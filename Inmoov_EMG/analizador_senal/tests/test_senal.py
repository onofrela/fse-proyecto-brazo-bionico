import csv
import queue
import tempfile
import threading
import unittest
from pathlib import Path
from unittest.mock import patch

from privacidad import MANIFEST

from senal import FIELDS, DeviceClock, Recorder as SecureRecorder, demo_lines, parse_line, read_csv as read_record_csv, serial_lines, summarize


def Recorder(*args, **kwargs):
    # Estos casos prueban el protocolo y la compatibilidad con CSV históricos.
    return SecureRecorder(*args, **kwargs, passphrase="prueba-sintetica", encrypted=False, include_time=True)


def read_csv(path):
    return read_record_csv(path, "prueba-sintetica")


class ProtocolTests(unittest.TestCase):
    def test_actual_firmware_and_combined_flags(self):
        kind, values = parse_line("D,12345,338,337,19,5,10,3,2,4\r\n")
        self.assertEqual((kind, values[6:]), ("D", [3, 2, 4]))
        for bad in ("D,1,338", "D,1,1024,338,9,12,24,0,0,0", "D,1,1,1,1,1,1,64,0,0",
                    "D,1,1,1,1,1,1,0,0,0,9", "D,NaN,1,1,1,1,1,0,0,0", "BOOT", "E,-1,BOOT,1"):
            self.assertIsNone(parse_line(bad), bad)

    def test_fixed_control_events_and_close_count(self):
        with tempfile.TemporaryDirectory() as temp:
            recorder = Recorder(Path(temp) / "r")
            for line in ("E,10,FW,8", "E,11,MEAN_X10,67", "E,12,TARGET_DEG,90", "E,13,TARGET_DEG,180", "E,14,CLOSE,180"):
                self.assertIsNotNone(recorder.accept(line))
            report = recorder.close()
            self.assertEqual(report["close_commands"], 1)
            self.assertEqual(report["events"], 5)

    def test_fault_bits_and_optional_derived_csv_columns(self):
        parsed=parse_line("D,100,338,338,5,5,10,48,0,0")
        self.assertEqual(parsed[1][6],48)
        with tempfile.TemporaryDirectory() as temp:
            recorder=Recorder(Path(temp)/"r")
            sample=recorder.accept("D,100,338,338,5,5,10,48,0,0")
            recorder.close()
            self.assertEqual((sample.row()["data_loss"],sample.row()["hardware_fault"]),(1,1))
            path=Path(temp)/"r"/"senal.csv"
            with path.open(encoding="utf-8-sig",newline="") as f:
                reader=csv.DictReader(f)
                self.assertEqual(reader.fieldnames, FIELDS)
                rows=list(reader)
            self.assertEqual(read_csv(path), [sample])
            fields=[key for key in rows[0] if key not in ("data_loss","hardware_fault")]
            (path.parent / MANIFEST).unlink()
            with path.open("w",encoding="utf-8-sig",newline="") as f:
                writer=csv.DictWriter(f,fields,extrasaction="ignore"); writer.writeheader(); writer.writerows(rows)
            self.assertEqual(read_csv(path)[0].flags,48)

    def test_demo_uses_fixed_levels_and_events(self):
        frames = [parse_line(line) for line in demo_lines(20)]
        self.assertTrue(all(frames))
        events = [payload for kind, payload in frames if kind == "E"]
        self.assertIn((0, "FW", 8), events)
        self.assertEqual({value for _, name, value in events if name == "TARGET_DEG"}, {0, 90, 180})
        self.assertEqual({name for _, name, _ in events}, {"BOOT", "FW", "MODE", "MEAN_X10", "TARGET_DEG"})
        for kind, payload in frames:
            if kind == "D":
                self.assertEqual(payload[4:6], [5, 10])

    def test_rollover_and_reboot(self):
        clock = DeviceClock()
        self.assertEqual(clock.observe(2**32 - 100), (0, 0))
        self.assertEqual(clock.observe(0), (0, 0.1))
        self.assertEqual(clock.observe(100), (0, 0.2))
        self.assertEqual(clock.observe(0, boot=True), (1, 0.201))
        self.assertEqual(clock.observe(100), (1, 0.301))
        # Un BOOT también marca reinicio si el nuevo tiempo supera al anterior.
        self.assertEqual(clock.observe(200, boot=True), (2, 0.302))

    def test_csv_roundtrip_and_valid_only_statistics(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp) / "registro"
            recorder = Recorder(directory, reference=4.95, source="demo")
            for line in demo_lines(20):
                recorder.accept(line)
            recorder.accept("trama rota")
            report = recorder.close()
            loaded = read_csv(directory / "senal.csv")
            self.assertEqual(len(loaded), 200)
            self.assertEqual(report["invalid_lines"], 1)
            self.assertEqual(report["telemetry_hz_median"], 10)
            self.assertGreater(report["flag_counts"]["Recorte"], 0)
            self.assertEqual(loaded[0].adc_reference_v, 4.95)
            self.assertEqual(loaded, recorder.samples)
            with (directory / "senal.csv").open(encoding="utf-8-sig", newline="") as stream:
                rows = list(csv.DictReader(stream))
            self.assertAlmostEqual(float(rows[0]["raw_v"]), loaded[0].raw_adc * 4.95 / 1023, places=5)
            self.assertEqual(summarize(loaded[:1])["raw_minus_baseline_rms_adc_valid"], None)

    def test_counter_wrap_and_reset_are_separate(self):
        with tempfile.TemporaryDirectory() as temp:
            recorder = Recorder(Path(temp) / "r")
            for line in ["D,100,338,338,2,12,24,0,65535,4294967295",
                         "D,200,338,338,2,12,24,0,0,0", "E,0,BOOT,2",
                         "D,100,338,338,2,12,24,0,0,0"]:
                recorder.accept(line)
            report = recorder.close()
            self.assertEqual(report["adc_overruns_increment_observed"], 1)
            self.assertEqual(report["dropped_logs_increment_observed"], 1)
            self.assertEqual(report["sessions"], 2)

    def test_invalid_csv_reports_line(self):
        with tempfile.TemporaryDirectory() as temp:
            recorder = Recorder(Path(temp) / "r")
            recorder.accept("D,0,338,338,4,12,24,0,0,0")
            recorder.close()
            path = Path(temp) / "r" / "senal.csv"
            (path.parent / MANIFEST).unlink()
            path.write_text(path.read_text(encoding="utf-8-sig").replace(",338,", ",NaN,", 1), encoding="utf-8-sig")
            with self.assertRaisesRegex(ValueError, "fila 2"):
                read_csv(path)


class SerialTests(unittest.TestCase):
    def run_fake(self, chunks, commands=None):
        stop = threading.Event()

        class FakeDevice:
            def __init__(self):
                self.now = 0
                self.writes = []
                self.chunks = list(chunks)
                self.in_waiting = 0
            def __enter__(self):
                return self
            def __exit__(self, *args):
                pass
            def read(self, size):
                self.now += 1
                if not self.chunks:
                    stop.set()
                    return b""
                return self.chunks.pop(0)
            def write(self, value):
                self.writes.append(value)

        device = FakeDevice()
        with patch("serial.Serial", return_value=device), patch("senal.time.monotonic", side_effect=lambda: device.now):
            lines = list(serial_lines("FAKE", stop, commands=commands))
        return device, lines

    def test_fragmented_reads_and_active_stream_does_not_toggle(self):
        device, lines = self.run_fake([b"D,100,338,", b"338,4,12,24,0,0,0\r\n",
                                       b"D,200,338,338,4,12,24,0,0,0\n",
                                       b"E,201,MODE,0\nD,300,338,338,4,12,24,0,0,0\n"])
        self.assertEqual(len(lines), 4)
        self.assertEqual(device.writes, [b"?"])
        self.assertTrue(all(parse_line(line) for line in lines))

    def test_inactive_stream_is_enabled_once(self):
        device, lines = self.run_fake([b"", b"", b"", b"", b"D,100,338,338,4,12,24,0,0,0\n"])
        self.assertEqual(device.writes, [b"p", b"?"])
        self.assertEqual(len(lines), 1)

    def test_silent_device_times_out(self):
        with self.assertRaisesRegex(RuntimeError, "Sin datos D"):
            self.run_fake([b""] * 20)

    def test_commands_and_local_labels(self):
        commands = queue.Queue()
        for command in ("c", "x", "?", "2", "@REST", "@HAND", "@END", "@INDEX", "unexpected"):
            commands.put(command)
        device, lines = self.run_fake([b"D,100,338,338,4,12,24,0,0,0\n"], commands)
        self.assertEqual(device.writes, [b"c", b"x", b"?", b"?"])
        self.assertEqual([parse_line(line) for line in lines[:3]],
                         [("E", (0, f"MARK_{name}", 0)) for name in ("REST", "HAND", "END")])


if __name__ == "__main__":
    unittest.main()
