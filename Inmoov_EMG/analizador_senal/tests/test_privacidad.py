"""Seguridad de registros con telemetría exclusivamente sintética."""
import csv
import io
import json
import os
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path
from unittest.mock import patch

from analizador import new_directory
from privacidad import (DEFAULT_OUTPUT, MAGIC, MANIFEST, RECORD_FILES, ask_passphrase,
                        canonical, decrypt_file, offer_cleanup, old_records, verify_directory)
from senal import Recorder, demo_lines, read_csv

PASSPHRASE = "contraseña de prueba con datos sintéticos"
ANALYZER = Path(__file__).resolve().parents[1] / "analizador.py"


class PrivacyTests(unittest.TestCase):
    def setUp(self):
        self.base = Path(self.enterContext(tempfile.TemporaryDirectory()))

    def record(self, **options):
        directory = new_directory(self.base)
        recorder = Recorder(directory, passphrase=PASSPHRASE, **options)
        for line in demo_lines(0.2):
            recorder.accept(line)
        report = recorder.close()
        return directory, recorder.samples, report

    def cli(self, *args, passphrase=PASSPHRASE):
        environment = os.environ.copy()
        environment["EMG_PASSPHRASE"] = passphrase
        environment["PYTHONIOENCODING"] = "utf-8"
        return subprocess.run([sys.executable, "-B", str(ANALYZER), *map(str, args)],
                              env=environment, capture_output=True, input=b"", timeout=30)

    def test_encrypted_roundtrip_and_minimization(self):
        directory, samples, report = self.record()
        self.assertEqual(DEFAULT_OUTPUT, Path.home() / "registros-emg")
        self.assertRegex(directory.name, r"^registro_[0-9a-f]{32}$")
        self.assertTrue(verify_directory(directory, PASSPHRASE))
        self.assertEqual(read_csv(directory / "senal.csv", PASSPHRASE), samples)
        self.assertTrue(all(s.host_utc == "" for s in samples))
        headers = []
        for name in RECORD_FILES:
            raw = (directory / name).read_bytes()
            self.assertTrue(raw.startswith(MAGIC))
            self.assertNotIn(PASSPHRASE.encode(), raw)
            headers.append(json.loads(raw[len(MAGIC):].split(b"\n", 1)[0]))
            clear = decrypt_file(directory / name, PASSPHRASE)
            self.assertNotIn(b"host_utc", clear)
            if name.endswith("csv"):
                self.assertNotIn(b"device_ms", raw)
                self.assertEqual(len(list(csv.DictReader(io.StringIO(clear.decode("utf-8-sig"))))),
                                 len(samples) if name == "senal.csv" else report["events"])
            else:
                self.assertEqual(json.loads(clear), report)
        self.assertEqual(len({h["salt"] for h in headers}), 3)
        self.assertEqual(len({h["nonce"] for h in headers}), 3)
        self.assertEqual({p.name for p in directory.iterdir()}, set(RECORD_FILES) | {MANIFEST})

    def test_wrong_password_fails(self):
        directory, _, _ = self.record()
        with self.assertRaises(ValueError):
            verify_directory(directory, "incorrecta")
        for name in RECORD_FILES:
            with self.assertRaises(ValueError):
                decrypt_file(directory / name, "incorrecta")
        self.assertNotEqual(self.cli("--verificar", directory, passphrase="incorrecta").returncode, 0)

    def test_each_tampered_file_fails_hmac_and_gcm(self):
        directory, _, _ = self.record()
        for name in RECORD_FILES:
            with self.subTest(name=name):
                path = directory / name
                original = path.read_bytes()
                corrupted = bytearray(original)
                corrupted[-17] ^= 1
                path.write_bytes(corrupted)
                with self.assertRaises(ValueError):
                    verify_directory(directory, PASSPHRASE)
                with self.assertRaises(ValueError):
                    decrypt_file(path, PASSPHRASE)
                result = self.cli("--descifrar", path)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")
                path.write_bytes(original)

    def test_manifest_changes_fail_including_forged_unkeyed_hash(self):
        directory, _, _ = self.record()
        path = directory / MANIFEST
        document = json.loads(path.read_text())
        for change in (lambda d: d["payload"].update(version=2),
                       lambda d: d["payload"]["files"].pop("eventos.csv"),
                       lambda d: d.update(hmac="0" * 64),
                       lambda d: d["payload"].update(salt="00" * 16)):
            changed = json.loads(json.dumps(document))
            change(changed)
            path.write_bytes(canonical(changed) + b"\n")
            with self.assertRaises(ValueError):
                verify_directory(directory, PASSPHRASE)
            self.assertNotEqual(self.cli("--verificar", directory).returncode, 0)

    def test_missing_or_truncated_file_and_missing_manifest_fail(self):
        directory, _, _ = self.record()
        signal = directory / "senal.csv"
        original = signal.read_bytes()
        signal.write_bytes(original[:-1])
        with self.assertRaises(ValueError):
            decrypt_file(signal, PASSPHRASE)
        signal.unlink()
        with self.assertRaises(ValueError):
            verify_directory(directory, PASSPHRASE)
        signal.write_bytes(original)
        (directory / MANIFEST).unlink()
        with self.assertRaises(ValueError):
            read_csv(signal, PASSPHRASE)

    def test_manifest_byte_change_and_added_field_fail(self):
        directory, _, _ = self.record()
        path = directory / MANIFEST
        original = path.read_bytes()
        path.write_bytes(original + b"\n")
        with self.assertRaises(ValueError):
            verify_directory(directory, PASSPHRASE)
        document = json.loads(original)
        document["otro"] = "alterado"
        path.write_bytes(canonical(document) + b"\n")
        with self.assertRaises(ValueError):
            verify_directory(directory, PASSPHRASE)

    def test_renaming_and_header_changes_fail_gcm(self):
        directory, _, _ = self.record()
        renamed = directory / "copia.csv"
        renamed.write_bytes((directory / "senal.csv").read_bytes())
        with self.assertRaises(ValueError):
            decrypt_file(renamed, PASSPHRASE)
        path = directory / "senal.csv"
        original = path.read_bytes()
        header_end = original.index(b"\n", len(MAGIC))
        metadata = json.loads(original[len(MAGIC):header_end])
        metadata["nonce"] = "00" * 12
        path.write_bytes(MAGIC + canonical(metadata) + b"\n" + original[header_end + 1:])
        with self.assertRaises(ValueError):
            decrypt_file(path, PASSPHRASE)

    def test_stream_is_encrypted_before_close(self):
        directory = new_directory(self.base)
        recorder = Recorder(directory, passphrase=PASSPHRASE)
        try:
            recorder.accept("D,0,338,338,4,5,10,0,0,0")
            self.assertTrue((directory / "senal.csv").read_bytes().startswith(MAGIC))
            self.assertNotIn(b"device_ms", (directory / "senal.csv").read_bytes())
            self.assertFalse((directory / MANIFEST).exists())
        finally:
            recorder.close()

    def test_plaintext_opt_in_keeps_integrity_and_optional_time(self):
        directory, samples, _ = self.record(encrypted=False, include_time=True)
        self.assertIn(b"host_utc", (directory / "senal.csv").read_bytes())
        self.assertTrue(samples[0].host_utc)
        self.assertEqual(read_csv(directory / "senal.csv", PASSPHRASE), samples)
        with (directory / "eventos.csv").open("ab") as stream:
            stream.write(b"alterado")
        with self.assertRaises(ValueError):
            verify_directory(directory, PASSPHRASE)

    def test_cli_consent_and_encrypted_commands(self):
        output = self.base / "output"
        result = self.cli("--headless", "--demo", "--seconds", ".1", "--output", output)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"--acepto-guardar", result.stderr)
        self.assertFalse(output.exists())
        result = self.cli("--headless", "--demo", "--seconds", ".1", "--output", output, "--acepto-guardar")
        self.assertEqual(result.returncode, 0, result.stderr)
        directory, = output.iterdir()
        self.assertFalse((directory / "grafica.png").exists())
        self.assertEqual(self.cli("--verificar", directory).returncode, 0)
        result = self.cli("--descifrar", directory / "senal.csv")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, decrypt_file(directory / "senal.csv", PASSPHRASE))
        result = self.cli("--headless", "--csv", directory / "senal.csv", "--output", output)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(len(list(output.iterdir())), 1)

    def test_cli_plaintext_warns_and_empty_password_cannot_record(self):
        output = self.base / "plain"
        args = ("--headless", "--demo", "--seconds", ".1", "--output", output, "--acepto-guardar", "--sin-cifrar")
        result = self.cli(*args, passphrase="")
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(output.exists())
        result = self.cli(*args)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(b"ADVERTENCIA --sin-cifrar", result.stderr)
        directory, = output.iterdir()
        self.assertNotIn(b"host_utc", (directory / "senal.csv").read_bytes())
        self.assertTrue(verify_directory(directory, PASSPHRASE))

    def test_retention_only_deletes_old_sessions_after_confirmation(self):
        directory, _, _ = self.record()
        past = time.time() - 31 * 86400
        for path in [*directory.iterdir(), directory]:
            os.utime(path, (past, past))
        recent, _, _ = self.record()
        unrelated = self.base / "registro_otro"
        unrelated.mkdir()
        (unrelated / "otro.txt").write_text("no es un registro")
        self.assertEqual(old_records(self.base), [directory])
        for accepted in (False, True):
            confirm, inform = unittest.mock.Mock(return_value=accepted), unittest.mock.Mock()
            offer_cleanup(self.base, confirm, inform)
            confirm.assert_called_once()
            self.assertIn(str(directory.resolve()), confirm.call_args.args[0])
            self.assertEqual(directory.exists(), not accepted)
        self.assertTrue(recent.exists())
        self.assertTrue(unrelated.exists())

    def test_passphrase_dialog_cancellation_and_confirmation(self):
        with patch.dict(os.environ, {}, clear=True):
            with self.assertRaises(ValueError):
                ask_passphrase(dialog=lambda prompt: None)
            with self.assertRaises(ValueError):
                ask_passphrase(new=True, dialog=unittest.mock.Mock(side_effect=["uno", "dos"]))
            self.assertEqual(ask_passphrase(new=True, dialog=lambda prompt: PASSPHRASE), PASSPHRASE)


if __name__ == "__main__":
    unittest.main()
