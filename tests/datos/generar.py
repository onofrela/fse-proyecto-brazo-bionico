#!/usr/bin/env python3
"""Genera exclusivamente datos sinteticos deterministas; no lee capturas."""
import csv
import hashlib
import io
import json
from datetime import datetime, timedelta, timezone
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Inmoov_EMG/analizador_senal"))
from senal import FIELDS, Sample

ROW_COUNT = 60
STEP_MS = 500
ENVELOPES = (2, 7, 12, 2, 12, 2)


def main():
    origin = datetime(2000, 1, 1, tzinfo=timezone.utc)
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=FIELDS, lineterminator="\n")
    writer.writeheader()
    for index in range(ROW_COUNT):
        device_ms = index * STEP_MS
        raw = 338 + ((index * 7) % 21) - 10
        envelope = ENVELOPES[index // 10] + index % 3 - 1
        host = (origin + timedelta(milliseconds=device_ms)).isoformat(timespec="milliseconds")
        sample = Sample(host, 0, device_ms, device_ms / 1000, raw, 338,
                        envelope, 5, 10, 0, 0, 0, 5.0)
        writer.writerow(sample.row())
    data = stream.getvalue().encode("utf-8")
    directory = Path(__file__).resolve().parent
    (directory / "senal_sintetica.csv").write_bytes(data)
    manifest = {"tipo": "sintetico", "archivo": "senal_sintetica.csv",
                "filas": ROW_COUNT, "paso_ms": STEP_MS, "columnas": FIELDS,
                "sha256": hashlib.sha256(data).hexdigest()}
    (directory / "manifiesto.json").write_text(
        json.dumps(manifest, indent=2, ensure_ascii=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
