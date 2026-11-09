"""Protocolo de Inmoov_EMG, registro, simulacion y analisis sin interfaz grafica."""
from __future__ import annotations

import csv
import json
import math
import queue
import statistics
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path


FIELDS = ["host_utc", "session", "device_ms", "time_s", "raw_adc", "baseline_adc",
          "envelope_adc", "threshold_open_adc", "threshold_close_adc", "flags",
          "adc_overruns", "dropped_logs", "adc_reference_v", "raw_v", "baseline_v",
          "envelope_v", "leads_off", "clipped", "not_ready", "stale"]
LEGACY_FIELDS = tuple(FIELDS)
FIELDS += ["data_loss", "hardware_fault"]
EVENT_FIELDS = ["host_utc", "session", "device_ms", "time_s", "name", "value"]
FLAG_NAMES = {1: "Electrodos sin contacto", 2: "Recorte", 4: "Estabilizando", 8: "ADC sin muestras recientes", 16: "Datos perdidos", 32: "Falla de hardware"}


def status_text(flags: int) -> str:
    return " | ".join(name for bit, name in FLAG_NAMES.items() if flags & bit) or "Señal lista"


def parse_line(line: str):
    """Rechaza tramas incompletas, rangos imposibles y texto del arranque."""
    parts = line.strip().split(",")
    try:
        if len(parts) == 10 and parts[0] == "D":
            values = [int(x) for x in parts[1:]]
            limits = [2**32 - 1, 1023, 1023, 1023, 1023, 1023, 63, 65535, 2**32 - 1]
            if all(0 <= x <= limit for x, limit in zip(values, limits)):
                return "D", values
        if len(parts) == 4 and parts[0] == "E" and parts[2]:
            ms, value = int(parts[1]), int(parts[3])
            if 0 <= ms < 2**32 and 0 <= value < 2**32:
                return "E", (ms, parts[2], value)
    except ValueError:
        pass
    return None


class DeviceClock:
    """Tiempo relativo continuo; conserva reinicios y rollover de millis()."""
    def __init__(self):
        self.last = None
        self.elapsed = 0.0
        self.session = 0

    def observe(self, ms, boot=False):
        if self.last is not None:
            delta = ms - self.last
            if boot or (-2**31 < delta < 0):
                self.session += 1
                self.elapsed += 0.001
            else:
                self.elapsed += (delta % 2**32) / 1000
        self.last = ms
        return self.session, round(self.elapsed, 6)


@dataclass
class Sample:
    host_utc: str
    session: int
    device_ms: int
    time_s: float
    raw_adc: int
    baseline_adc: int
    envelope_adc: int
    threshold_open_adc: int
    threshold_close_adc: int
    flags: int
    adc_overruns: int
    dropped_logs: int
    adc_reference_v: float = 5.0

    def row(self):
        result = dict(self.__dict__)
        for name in ("raw", "baseline", "envelope"):
            result[name + "_v"] = round(result[name + "_adc"] * self.adc_reference_v / 1023, 6)
        for name, bit in zip(("leads_off", "clipped", "not_ready", "stale", "data_loss", "hardware_fault"), (1, 2, 4, 8, 16, 32)):
            result[name] = int(bool(self.flags & bit))
        return result


class Recorder:
    """Escribe cada trama antes de entregarla a la grafica. CSV UTF-8 para Excel."""
    def __init__(self, directory, reference=5.0, source="serial"):
        self.directory = Path(directory)
        self.directory.mkdir(parents=True, exist_ok=False)
        self.reference = reference
        self.source = source
        self.clock = DeviceClock()
        self.invalid = 0
        self.samples = []
        self.events = []
        self._files = []
        try:
            for name in ("senal", "eventos"):
                self._files.append((self.directory / f"{name}.csv").open("w", newline="", encoding="utf-8-sig"))
            self.data_writer = csv.DictWriter(self._files[0], fieldnames=FIELDS)
            self.event_writer = csv.DictWriter(self._files[1], fieldnames=EVENT_FIELDS)
            self.data_writer.writeheader()
            self.event_writer.writeheader()
            for stream in self._files:
                stream.flush()
        except Exception:
            for stream in self._files:
                stream.close()
            raise

    def accept(self, line):
        parsed = parse_line(line)
        if parsed is None:
            if line.strip():
                self.invalid += 1
            return None
        kind, payload = parsed
        ms = payload[0]
        session, relative = self.clock.observe(ms, kind == "E" and payload[1] == "BOOT")
        host = datetime.now(timezone.utc).isoformat(timespec="milliseconds")
        if kind == "D":
            sample = Sample(host, session, ms, relative, *payload[1:], self.reference)
            self.data_writer.writerow(sample.row())
            self._files[0].flush()
            self.samples.append(sample)
            return sample
        event = dict(zip(EVENT_FIELDS, (host, session, ms, relative, payload[1], payload[2])))
        self.event_writer.writerow(event)
        self._files[1].flush()
        self.events.append(event)
        return event

    def close(self):
        for stream in self._files:
            stream.close()
        report = summarize(self.samples)
        report.update(source=self.source, invalid_lines=self.invalid,
                      events=len(self.events), close_commands=sum(e["name"] == "CLOSE" for e in self.events))
        (self.directory / "resumen.json").write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
        return report


def read_csv(path):
    samples = []
    with Path(path).open(encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        if not set(LEGACY_FIELDS).issubset(reader.fieldnames or []):
            raise ValueError("El CSV debe ser senal.csv generado por este analizador.")
        for number, row in enumerate(reader, 2):
            try:
                ints = [int(row[name]) for name in FIELDS[1:3] + FIELDS[4:12]]
                sample = Sample(row["host_utc"], ints[0], ints[1], float(row["time_s"]), *ints[2:], float(row["adc_reference_v"]))
                if (not math.isfinite(sample.time_s) or sample.time_s < 0 or
                        not math.isfinite(sample.adc_reference_v) or sample.adc_reference_v <= 0 or
                        sample.session < 0 or sample.device_ms < 0 or sample.device_ms >= 2**32 or
                        any(not 0 <= getattr(sample, name) <= 1023 for name in FIELDS[4:9]) or
                        not 0 <= sample.flags <= 63 or not 0 <= sample.adc_overruns <= 65535 or
                        not 0 <= sample.dropped_logs < 2**32 or
                        (samples and sample.time_s < samples[-1].time_s)):
                    raise ValueError("Rangos o tiempos no válidos")
                samples.append(sample)
            except (ValueError, TypeError, KeyError) as exc:
                raise ValueError(f"CSV inválido en fila {number}: {exc}") from exc
    if not samples:
        raise ValueError("El CSV no contiene muestras.")
    return samples


def summarize(samples):
    valid = [s for s in samples if s.flags == 0]
    steps = [b.time_s - a.time_s for a, b in zip(samples, samples[1:])
             if a.session == b.session and b.time_s > a.time_s]
    rms = math.sqrt(statistics.mean((s.raw_adc - s.baseline_adc)**2 for s in valid)) if valid else None
    counters = {}
    for name, modulus in (("adc_overruns", 65536), ("dropped_logs", 2**32)):
        counters[name + "_increment_observed"] = sum(
            (getattr(b, name) - getattr(a, name)) % modulus
            for a, b in zip(samples, samples[1:]) if a.session == b.session)
    return {"samples": len(samples), "valid_samples": len(valid),
            "duration_s": round(samples[-1].time_s - samples[0].time_s, 3) if samples else 0,
            "telemetry_hz_median": round(1 / statistics.median(steps), 3) if steps else None,
            "gaps_over_250ms": sum(x > 0.250 for x in steps),
            "sessions": len({s.session for s in samples}),
            "raw_minus_baseline_rms_adc_valid": round(rms, 3) if rms is not None else None,
            "envelope_mean_adc_valid": round(statistics.mean(s.envelope_adc for s in valid), 3) if valid else None,
            "envelope_max_adc_valid": max((s.envelope_adc for s in valid), default=None),
            "flag_counts": {name: sum(bool(s.flags & bit) for s in samples) for bit, name in FLAG_NAMES.items()},
            **counters,
            "note": "Estadísticas de telemetría a ~10 Hz, no de las 1000 muestras/s internas. RMS de valores recibidos; no es RMS EMG de banda completa. Los voltajes corresponden a OUTPUT, no al músculo. CLOSE es una orden, no movimiento medido."}


def demo_lines(seconds=20):
    """Señales sintéticas a 10 Hz: reposo, actividad, pérdida de contacto y recorte."""
    yield "E,0,BOOT,1"
    yield "E,0,MODE,0"
    previous_active = False
    for i in range(max(1, int(seconds * 10))):
        t = i / 10
        active = 3 <= t % 10 < 6
        baseline = 338 + int(3 * math.sin(t * 0.4))
        amplitude = 65 if active else 6
        raw = baseline + int(amplitude * math.sin(i * 1.37))
        envelope = (38 if active else 4) + int(2 * math.sin(i * 0.8))
        flags = 4 if t < 2 else 0
        if 12 <= t < 13:
            flags, raw, envelope = 1, 338, 0
        elif 13 <= t < 15:
            flags, envelope = 4, 0
        elif 17 <= t < 17.5:
            flags, raw, envelope = 2, 675, 0
        elif 17.5 <= t < 19.5:
            flags, envelope = 4, 0
        if active != previous_active:
            yield f"E,{i * 100},{'CLOSE' if active else 'OPEN'},25"
            previous_active = active
        yield f"D,{i * 100},{raw},{baseline},{envelope},12,24,{flags},0,0"


def serial_lines(port, stop, baud=115200, commands=None):
    """Detecta p activo. Sólo transmite c/x al recibir la orden explícita de la UI."""
    import serial
    with serial.Serial(port, baudrate=baud, timeout=0.1, write_timeout=1) as device:
        # Abrir UNO suele reiniciarlo. Damos tiempo al bootloader sin descartar eventos.
        start = time.monotonic()
        last_data = start
        toggled = False
        saw_data = False
        pending = bytearray()
        queried = False
        latest_ms = 0
        while not stop.is_set():
            if commands is not None:
                try:
                    while True:
                        command = commands.get_nowait()
                        if command in ("@REST", "@HAND", "@END"):
                            # Marca local en eventos.csv; no se transmite al Arduino.
                            yield f"E,{latest_ms},MARK_{command[1:]},0"
                        elif command in ("c", "x", "?"):
                            device.write(command.encode("ascii"))
                except queue.Empty:
                    pass
            chunk = device.read(max(1, min(device.in_waiting, 4096)))
            pending.extend(chunk)
            if len(pending) > 8192:
                raise RuntimeError("Trama serie demasiado larga. Revisa firmware y baudrate.")
            while b"\n" in pending:
                raw, _, pending = pending.partition(b"\n")
                line = raw.decode("ascii", errors="replace").strip()
                parsed = parse_line(line)
                if parsed:
                    latest_ms = parsed[1][0]
                if parsed and parsed[0] == "D":
                    saw_data = True
                    last_data = time.monotonic()
                    if not queried:
                        device.write(b"?")
                        queried = True
                if line:
                    yield line
            now = time.monotonic()
            if not saw_data and not toggled and now - start >= 3:
                device.write(b"p")
                toggled = True
                last_data = now
            if now - last_data >= (6 if not saw_data else 5):
                raise RuntimeError("Sin datos D. Revisa conexión, 115200 baud y EMG_SERIAL_DEBUG=1. Cierra el monitor serie; si la placa se reinició, vuelve a conectar.")


def make_figure():
    from matplotlib.figure import Figure
    figure = Figure(figsize=(11, 8), dpi=110, layout="constrained", facecolor="#f8fafc")
    axes = figure.subplots(3, 1, sharex=True, gridspec_kw={"height_ratios": [2, 2, 2.5]})
    return figure, axes


def draw_plot(figure, axes, samples, title="Señal AD8232", window=None):
    for ax in axes:
        ax.clear()
        ax.grid(True, alpha=0.2)
        ax.tick_params(axis="both", labelsize=8)
    figure.suptitle(title, fontsize=14, fontweight="bold")
    if not samples:
        axes[0].text(0.5, 0.5, "Esperando datos…", transform=axes[0].transAxes, ha="center")
        return
    if window:
        start = samples[-1].time_s - window
        samples = [s for s in samples if s.time_s >= start]
    # Conserva todos los puntos guardados; reduce sólo el dibujo de registros grandes.
    stride = max(1, len(samples) // 20000)
    plotted = samples[::stride]
    if plotted[-1] is not samples[-1]:
        plotted.append(samples[-1])
    t = [s.time_s for s in plotted]
    axes[0].plot(t, [s.raw_adc for s in plotted], label="OUTPUT recibido", color="#2563eb", linewidth=1)
    axes[0].plot(t, [s.baseline_adc for s in plotted], label="Base DC (Arduino)", color="#64748b")
    axes[0].set_ylabel("ADC", fontsize=9)
    for field, label, color, style in (
        ("envelope_adc", "Envolvente (Arduino)", "#059669", "-"),
        ("threshold_open_adc", "Umbral abrir", "#f59e0b", "--"),
        ("threshold_close_adc", "Umbral cerrar", "#dc2626", "--")):
        axes[1].plot(t, [getattr(s, field) for s in plotted], label=label, color=color, linestyle=style)
    axes[1].set_ylabel("Actividad", fontsize=9)
    colors = ("#dc2626", "#ea580c", "#9333ea", "#475569", "#be185d", "#0f766e")
    # Los estados se dibujan con todos los puntos para no perder fallas breves.
    all_t = [s.time_s for s in samples]
    for index, ((bit, label), color) in enumerate(zip(FLAG_NAMES.items(), colors)):
        axes[2].step(all_t, [index + (0.65 if s.flags & bit else 0) for s in samples],
                     where="post", label=label, color=color, linewidth=1.1)
    axes[2].set_yticks(range(6), ["LO", "Recorte", "Espera", "ADC", "Pérdida", "HW"])
    axes[2].tick_params(axis="y", labelsize=7)
    axes[2].set_ylim(-0.2, 6)
    axes[2].set_xlabel("Tiempo relativo (s) · telemetría nominal 10 Hz", fontsize=10)
    for ax in axes[:2]:
        ax.legend(loc="upper right", fontsize=8)
    if len(t) == 1:
        axes[0].set_xlim(t[0] - 0.5, t[0] + 0.5)


def save_plot(samples, path, title="Señal AD8232"):
    from matplotlib.backends.backend_agg import FigureCanvasAgg
    figure, axes = make_figure()
    FigureCanvasAgg(figure)
    draw_plot(figure, axes, samples, title)
    figure.savefig(path, dpi=150)
