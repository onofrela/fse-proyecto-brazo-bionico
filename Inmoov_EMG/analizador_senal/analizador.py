"""Aplicacion de escritorio y CLI para la telemetria de Inmoov_EMG."""
from __future__ import annotations

import argparse
import json
import queue
import threading
import time
from datetime import datetime
from pathlib import Path

from senal import (Recorder, Sample, demo_lines, draw_plot, make_figure,
                   read_csv, save_plot, serial_lines, status_text, summarize)

HERE = Path(__file__).resolve().parent


def new_directory(base):
    return Path(base) / datetime.now().strftime("registro_%Y%m%d_%H%M%S_%f")


def acquire(directory, stop, *, demo=False, port=None, seconds=0, reference=5.0,
            commands=None,
            notify=lambda kind, payload: None):
    recorder = Recorder(directory, reference, "demo_sintetica" if demo else f"serial:{port}")
    start = time.monotonic()
    source = demo_lines(seconds if seconds else 86400) if demo else serial_lines(port, stop, commands=commands)
    deadline = threading.Timer(seconds, stop.set) if seconds and not demo else None
    if deadline:
        deadline.start()
    try:
        for line in source:
            if stop.is_set() or (seconds and not demo and time.monotonic() - start >= seconds):
                break
            item = recorder.accept(line)
            if item is not None:
                notify("sample" if isinstance(item, Sample) else "event", item)
            if demo and isinstance(item, Sample):
                if stop.wait(0.1):
                    break
    finally:
        if deadline:
            deadline.cancel()
        source.close()
        recorder.close()
    return recorder.samples


def run_gui(args):
    import tkinter as tk
    from tkinter import filedialog, messagebox, ttk
    from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
    from serial.tools import list_ports

    class App:
        def __init__(self, root):
            self.root = root
            root.title("InMoov · Abrir y cerrar mano")
            width = min(1180, root.winfo_screenwidth() - 60)
            height = min(920, root.winfo_screenheight() - 80)
            root.geometry(f"{width}x{height}")
            root.minsize(min(960, width), min(620, height))
            self.samples = []
            self.messages = queue.Queue()
            self.stop = threading.Event()
            self.thread = None
            self.directory = None
            self.closing = False
            self.dirty = True
            self.commands = queue.Queue()
            self.firmware_version = 0
            self.capture_started = None
            self.calibrating = False
            self.prompt = "Conecta para consultar firmware y calibración."
            self.prompt_until = None
            self.calibration_status = tk.StringVar(value=self.prompt)
            self.activity_status = tk.StringVar(value="Actividad ponderada: — · Contracción: cerrar · Relajación: abrir")
            self.label = tk.StringVar(value="REST")
            self.mode = tk.StringVar(value="Sensor USB")
            self.port = tk.StringVar(value=args.port or "")
            self.reference = tk.StringVar(value=str(args.reference))
            self.status = tk.StringVar(value="Elige Sensor USB o Demo y pulsa Iniciar.")
            self.metrics = tk.StringVar(value="Sin muestras")
            self.path_label = tk.StringVar(value="Los registros se guardan automáticamente al iniciar.")
            wrapper = ttk.Frame(root, padding=12)
            wrapper.pack(fill="both", expand=True)
            ttk.Label(wrapper, text="InMoov · Control de mano completa", font=("Segoe UI", 18, "bold")).pack(anchor="w")
            ttk.Label(wrapper, text="Un canal AD8232 · USB 115200 · Arduino: 1 kHz interno / 10 Hz enviados").pack(anchor="w", pady=(0, 8))
            controls = ttk.Frame(wrapper)
            controls.pack(fill="x")
            self.mode_box = ttk.Combobox(controls, textvariable=self.mode, values=["Sensor USB", "Demo sintética"], state="readonly", width=17)
            self.mode_box.pack(side="left", padx=(0, 8))
            ttk.Label(controls, text="Puerto:").pack(side="left")
            self.port_box = ttk.Combobox(controls, textvariable=self.port, width=12)
            self.port_box.pack(side="left", padx=5)
            self.refresh_button = ttk.Button(controls, text="Buscar puertos", command=self.refresh)
            self.refresh_button.pack(side="left")
            ttk.Label(controls, text="Referencia ADC (V):").pack(side="left", padx=(12, 4))
            self.reference_box = ttk.Entry(controls, textvariable=self.reference, width=6)
            self.reference_box.pack(side="left")
            self.start_button = ttk.Button(controls, text="Iniciar / grabar CSV", command=self.start)
            self.start_button.pack(side="left", padx=(12, 5))
            self.stop_button = ttk.Button(controls, text="Detener", command=self.stop.set, state="disabled")
            self.stop_button.pack(side="left")
            actions = ttk.Frame(wrapper)
            actions.pack(fill="x", pady=8)
            self.open_button = ttk.Button(actions, text="Abrir CSV…", command=self.open_csv)
            self.open_button.pack(side="left")
            ttk.Button(actions, text="Exportar gráfica completa PNG…", command=self.export).pack(side="left", padx=8)
            ttk.Label(actions, text="En vivo: últimos 30 s. El CSV conserva todas las muestras recibidas.").pack(side="left")
            calibration_controls = ttk.Frame(wrapper)
            calibration_controls.pack(fill="x", pady=(0, 4))
            self.cal_button = ttk.Button(calibration_controls, text="Calibrar (22 s)", command=lambda: self.send("c"), state="disabled")
            self.cal_button.pack(side="left")
            self.cancel_button = ttk.Button(calibration_controls, text="Cancelar / abrir", command=lambda: self.send("x"), state="disabled")
            self.cancel_button.pack(side="left", padx=5)
            ttk.Label(calibration_controls, textvariable=self.activity_status).pack(side="left", padx=12)
            markers = ttk.Frame(wrapper)
            markers.pack(fill="x", pady=(0, 4))
            ttk.Label(markers, text="Etiqueta del ensayo:").pack(side="left")
            ttk.Combobox(markers, textvariable=self.label, values=["REST", "HAND", "END"], state="readonly", width=10).pack(side="left", padx=5)
            self.mark_button = ttk.Button(markers, text="Marcar inicio", command=lambda: self.send("@" + self.label.get()), state="disabled")
            self.mark_button.pack(side="left")
            ttk.Label(markers, text="Sólo añade una marca local al CSV de eventos.").pack(side="left", padx=8)
            ttk.Label(wrapper, textvariable=self.calibration_status, wraplength=1080, foreground="#1d4ed8").pack(anchor="w", pady=3)
            ttk.Label(wrapper, textvariable=self.status, font=("Segoe UI", 10, "bold")).pack(anchor="w")
            ttk.Label(wrapper, textvariable=self.metrics).pack(anchor="w", pady=3)
            ttk.Label(wrapper, textvariable=self.path_label, wraplength=1100).pack(anchor="w")
            self.figure, self.axes = make_figure()
            self.canvas = FigureCanvasTkAgg(self.figure, master=wrapper)
            self.log = tk.Text(wrapper, height=2, state="disabled", font=("Consolas", 9))
            self.log.pack(side="bottom", fill="x", pady=(6, 0))
            NavigationToolbar2Tk(self.canvas, wrapper, pack_toolbar=False).pack(side="bottom", fill="x")
            self.canvas.get_tk_widget().pack(fill="both", expand=True, pady=6)
            self.refresh()
            root.protocol("WM_DELETE_WINDOW", self.close)
            if args.csv:
                self.load(args.csv)
            root.after(100, self.tick)

        def refresh(self):
            ports = [p.device for p in list_ports.comports()]
            self.port_box["values"] = ports
            if not self.port.get() and ports:
                self.port.set(ports[0])

        def write_log(self, text):
            self.log.configure(state="normal")
            self.log.insert("end", text + "\n")
            if int(self.log.index("end-1c").split(".")[0]) > 250:
                self.log.delete("1.0", "50.0")
            self.log.see("end")
            self.log.configure(state="disabled")

        def set_running(self, running):
            for widget in (self.start_button, self.open_button, self.refresh_button, self.reference_box):
                widget.configure(state="disabled" if running else "normal")
            self.mode_box.configure(state="disabled" if running else "readonly")
            self.port_box.configure(state="disabled" if running else "normal")
            self.stop_button.configure(state="normal" if running else "disabled")
            self.update_controls()

        def update_controls(self):
            recording = self.thread is not None and self.mode.get() == "Sensor USB"
            capable = recording and self.firmware_version == 6
            self.cal_button.configure(state="normal" if capable and not self.calibrating else "disabled")
            self.cancel_button.configure(state="normal" if capable else "disabled")
            self.mark_button.configure(state="normal" if recording else "disabled")

        def send(self, command):
            if self.thread and self.mode.get() == "Sensor USB":
                self.commands.put(command)
                if command == "c":
                    self.calibrating = True
                    self.prompt = "Calibración solicitada. Espera las instrucciones; la mano permanecerá abierta."
                    self.prompt_until = None
                    self.update_controls()

        def handle_event(self, event):
            name, value = event["name"], event["value"]
            instructions = {"CAL_PENDING": "Esperando señal estable y mano abierta.",
                "CAL_PREP_REST": "Prepárate: relaja la mano y mantén quieto el brazo.",
                "CAL_REST": "REPOSO: mantén la mano relajada y el brazo quieto.",
                "CAL_PREP_CONTRACT": "Prepárate para contraer y relajar toda la mano repetidamente.",
                "CAL_CONTRACT": "ACTIVIDAD: alterna una contracción cómoda de toda la mano y relajación."}
            if name == "BOOT":
                self.firmware_version = 0
                self.calibrating = False
                self.prompt = "Placa reiniciada. Consultando firmware y calibración…"
                self.prompt_until = None
            elif name == "FW":
                self.firmware_version = value
                if value != 6:
                    self.prompt = "Carga el firmware con ponderación (v6) para habilitar la calibración."
                    self.prompt_until = None
            elif name == "ACTIVITY":
                self.activity_status.set(f"Actividad ponderada: {value / 10:.1f}% · Contracción: cerrar · Relajación: abrir")
            elif name in instructions:
                self.calibrating = True
                self.prompt = instructions[name]
                self.prompt_until = time.monotonic() + value / 1000 if value else None
            elif name == "CAL_FAILED":
                reasons = {1: "Se perdió la señal.", 2: "Faltaron muestras.",
                           3: "Reposo y actividad se superponen: repite el ensayo o revisa los electrodos/banda del módulo.",
                           4: "La envolvente excede el rango del histograma; revisa ganancia/ruido."}
                self.calibrating = False
                self.prompt = "CALIBRACIÓN RECHAZADA: " + reasons.get(value, "Firmware anterior: la calibración no separó las mediciones.")
                self.prompt_until = None
            elif name in ("TH_CLOSE", "CAL_CANCEL"):
                self.calibrating = False
                self.prompt = "Calibración aceptada. Relaja para rearmar el control." if name == "TH_CLOSE" else "Calibración cancelada / apertura solicitada."
                self.prompt_until = None
            elif name == "CAL_VALID" and self.firmware_version == 6 and not self.calibrating and "RECHAZADA" not in self.prompt:
                self.prompt = "Calibración disponible. Relaja para rearmar." if value else "SIN CALIBRAR: pulsa Calibrar cuando estés preparado."
                self.prompt_until = None
            self.update_controls()

        def start(self):
            demo = self.mode.get() == "Demo sintética"
            try:
                reference = float(self.reference.get())
                if not 0 < reference <= 10:
                    raise ValueError
            except ValueError:
                messagebox.showerror("Referencia ADC", "Introduce un voltaje mayor que 0 y hasta 10 V. En Config.h es 5.0 V por defecto.")
                return
            port = self.port.get().strip()
            if not demo and not port:
                messagebox.showerror("Puerto", "Selecciona un puerto COM o elige Demo sintética.")
                return
            self.samples = []
            self.commands = queue.Queue()
            self.firmware_version = 0
            self.capture_started = time.monotonic()
            self.activity_status.set("Actividad ponderada: — · Contracción: cerrar · Relajación: abrir")
            self.calibrating = False
            self.prompt = "Consultando versión del firmware…" if not demo else "Demo sintética: los controles de actuadores están deshabilitados."
            self.prompt_until = None
            self.directory = new_directory(args.output)
            self.stop.clear()
            self.set_running(True)
            self.dirty = True
            self.status.set("DEMO: datos sintéticos" if demo else "Conectando… cierra el monitor serie de Arduino.")
            self.path_label.set(str(self.directory))
            self.write_log("Inicio DEMO (datos sintéticos)." if demo else f"Inicio {port}. Abrir USB puede reiniciar el UNO; se conserva su control actual.")

            def worker():
                error = None
                try:
                    acquire(self.directory, self.stop, demo=demo, port=port, reference=reference,
                            commands=self.commands,
                            notify=lambda kind, payload: self.messages.put((kind, payload)))
                except Exception as exc:
                    error = str(exc)
                finally:
                    self.messages.put(("done", error))
            self.thread = threading.Thread(target=worker, daemon=True)
            self.thread.start()
            self.update_controls()

        def load(self, path):
            try:
                samples = read_csv(path)
            except (OSError, ValueError) as exc:
                messagebox.showerror("No se pudo abrir", str(exc))
                return
            self.samples = samples
            self.directory = None
            self.prompt = "CSV cargado: se muestra la envolvente guardada, sin aplicar de nuevo el filtrado."
            self.prompt_until = None
            self.path_label.set(str(Path(path).resolve()))
            self.status.set("Registro cargado · " + status_text(samples[-1].flags))
            self.dirty = True
            self.show_metrics()

        def open_csv(self):
            path = filedialog.askopenfilename(title="Abrir senal.csv", filetypes=[("CSV del analizador", "*.csv")])
            if path:
                self.load(path)

        def export(self):
            if not self.samples:
                messagebox.showinfo("Sin datos", "Inicia una captura o abre un CSV primero.")
                return
            path = filedialog.asksaveasfilename(title="Guardar gráfica", defaultextension=".png", initialfile="grafica.png", filetypes=[("Imagen PNG", "*.png")])
            if path:
                try:
                    save_plot(list(self.samples), path, self.plot_title())
                    self.write_log("Gráfica exportada: " + path)
                except OSError as exc:
                    messagebox.showerror("Error al exportar", str(exc))

        def plot_title(self):
            if self.directory and self.mode.get() == "Demo sintética":
                return "DEMO · datos sintéticos · AD8232"
            return "Registro AD8232 · OUTPUT y actividad calculada"

        def show_metrics(self):
            # El resumen completo se genera al detener; la vista en vivo usa la ventana reciente.
            report = summarize(self.samples[-300:] if self.thread else self.samples)
            rate = report["telemetry_hz_median"]
            rms = report["raw_minus_baseline_rms_adc_valid"]
            self.metrics.set(f"Muestras guardadas/cargadas: {len(self.samples)}  |  Telemetría: {rate if rate is not None else '—'} Hz  |  "
                             f"Válidas en vista: {report['valid_samples']}/{report['samples']}  |  "
                             f"RMS (OUTPUT − base), ADC: {rms if rms is not None else '—'}")

        def tick(self):
            try:
                while True:
                    kind, payload = self.messages.get_nowait()
                    if kind == "sample":
                        self.samples.append(payload)
                        self.status.set(("DEMO · " if self.mode.get() == "Demo sintética" else "Grabando · ") + status_text(payload.flags))
                        self.dirty = True
                    elif kind == "event":
                        self.handle_event(payload)
                        self.write_log(f"{payload['time_s']:8.3f} s  {payload['name']} = {payload['value']}")
                    elif kind == "done":
                        self.thread = None
                        self.calibrating = False
                        self.set_running(False)
                        self.status.set("Captura terminada" if payload is None else "Captura detenida: " + payload)
                        self.write_log(self.status.get())
                        if self.samples:
                            try:
                                save_plot(self.samples, self.directory / "grafica.png", self.plot_title())
                                self.write_log("Guardados senal.csv, eventos.csv, resumen.json y grafica.png.")
                            except OSError as exc:
                                self.write_log("CSV conservado; no se pudo guardar PNG: " + str(exc))
                        if payload and not self.closing:
                            messagebox.showerror("Captura", payload)
                        self.dirty = True
            except queue.Empty:
                pass
            if self.closing and self.thread is None:
                self.root.destroy()
                return
            if (self.thread and self.mode.get() == "Sensor USB" and not self.firmware_version
                    and self.capture_started and time.monotonic()-self.capture_started>6
                    and self.prompt in ("Consultando versión del firmware…", "Placa reiniciada. Consultando firmware y calibración…")):
                self.prompt = "Carga el firmware con ponderación (v6) para habilitar la calibración guiada."
            remaining = max(0, int(self.prompt_until - time.monotonic()) + 1) if self.prompt_until else None
            self.calibration_status.set(self.prompt + (f" ({remaining} s)" if remaining is not None else ""))
            if self.dirty:
                draw_plot(self.figure, self.axes, self.samples, self.plot_title(), window=30 if self.thread else None)
                self.canvas.draw_idle()
                self.show_metrics()
                self.dirty = False
            self.root.after(250, self.tick)

        def close(self):
            self.closing = True
            self.stop.set()
            self.status.set("Terminando registro y guardando…")

    root = tk.Tk()
    App(root)
    root.mainloop()


def main():
    parser = argparse.ArgumentParser(description="Graficar y guardar la telemetría AD8232 de Inmoov_EMG.")
    parser.add_argument("--port", help="Puerto serie, por ejemplo COM3")
    parser.add_argument("--demo", action="store_true", help="Datos sintéticos (CLI)")
    parser.add_argument("--headless", action="store_true", help="Captura sin ventana; Ctrl+C para terminar")
    parser.add_argument("--seconds", type=float, default=0, help="Duración CLI; 0 = hasta Ctrl+C (demo: 20 s por defecto)")
    parser.add_argument("--csv", type=Path, help="Abrir un senal.csv existente; con --headless genera PNG y resumen")
    parser.add_argument("--reference", type=float, default=5.0, help="Referencia ADC AVcc en voltios, igual a Config.h")
    parser.add_argument("--output", type=Path, default=HERE / "registros", help="Directorio base de salida")
    args = parser.parse_args()
    if not 0 < args.reference <= 10 or not 0 <= args.seconds < 86400:
        parser.error("Referencia fuera de rango o duración inválida (0 a <86400 s).")
    if not args.headless:
        run_gui(args)
        return
    if not args.csv and not args.demo and not args.port:
        parser.error("Usa --port COM3, --demo o --csv senal.csv.")
    if args.csv:
        samples = read_csv(args.csv)
        target = new_directory(args.output)
        target.mkdir(parents=True)
        (target / "resumen.json").write_text(json.dumps(summarize(samples), indent=2, ensure_ascii=False), encoding="utf-8")
    else:
        target = new_directory(args.output)
        print("Guardando en:", target, flush=True)
        stop = threading.Event()
        try:
            acquire(target, stop, demo=args.demo, port=args.port,
                    seconds=args.seconds or (20 if args.demo else 0), reference=args.reference)
        except KeyboardInterrupt:
            print("Captura detenida por el usuario.")
        except Exception as exc:
            print("Captura detenida:", exc)
            # Si hubo datos antes de la desconexión, también genera la gráfica.
            if (target / "senal.csv").exists():
                try:
                    samples = read_csv(target / "senal.csv")
                    save_plot(samples, target / "grafica.png", "Captura interrumpida · AD8232")
                except ValueError:
                    pass
            raise SystemExit(1) from exc
        try:
            samples = read_csv(target / "senal.csv")
        except ValueError:
            raise SystemExit("No se recibieron muestras. Revisa puerto, firmware y conexión.")
    title = "DEMO · datos sintéticos · AD8232" if args.demo else "Registro AD8232 · telemetría 10 Hz"
    save_plot(samples, target / "grafica.png", title)
    print(json.dumps(summarize(samples), indent=2, ensure_ascii=False))
    print("Archivos:", target)


if __name__ == "__main__":
    main()
