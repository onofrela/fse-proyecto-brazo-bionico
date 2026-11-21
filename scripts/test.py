#!/usr/bin/env python3
"""Pruebas C++ nativas y tabla de evidencia; no conectan ni accionan hardware."""
from pathlib import Path
import argparse
import csv
import hashlib
import json
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build" / "native"
DATA = ROOT / "tests" / "datos"
SYNTHETIC_CSV = DATA / "senal_sintetica.csv"
SUITES = {
    "control": ["tests/control_test.cpp"],
    "levels": ["tests/levels_test.cpp"],
    "replay": ["tests/replay_test.cpp"],
    "filter": ["tests/filter_test.cpp"],
    "integration": ["tests/integration_test.cpp", "Inmoov_EMG/Application.cpp"],
    "storage": ["tests/storage_test.cpp", "Inmoov_EMG/Storage.cpp"],
    "drivers": ["tests/drivers_test.cpp", "Inmoov_EMG/Outputs.cpp", "Inmoov_EMG/Diagnostics.cpp"],
}


def run(command, **kwargs):
    completed = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, **kwargs)
    if completed.returncode:
        raise RuntimeError(f"Fallo ({completed.returncode}): {command}\n{completed.stdout}")
    return completed.stdout


def compiler_environment():
    if shutil.which("g++"):
        return "g++", os.environ.copy()
    if shutil.which("clang++"):
        return "clang++", os.environ.copy()
    if shutil.which("cl"):
        return "cl", os.environ.copy()
    if os.name == "nt":
        vswhere = Path(os.environ.get("ProgramFiles(x86)", "C:/Program Files (x86)")) / "Microsoft Visual Studio/Installer/vswhere.exe"
        if vswhere.is_file():
            installation = run([str(vswhere), "-latest", "-products", "*", "-requires",
                                "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"]).strip()
            developer = Path(installation) / "Common7/Tools/VsDevCmd.bat"
            if installation and developer.is_file():
                batch = BUILD / "compiler_env.cmd"
                batch.write_text(f'@echo off\ncall "{developer}" -arch=x64 -no_logo\nif errorlevel 1 exit /b 1\nset\n')
                environment = os.environ.copy()
                for line in run(["cmd.exe", "/d", "/c", str(batch)]).splitlines():
                    if "=" in line and not line.startswith("="):
                        key, value = line.split("=", 1)
                        environment[key.upper()] = value
                return "cl", environment
    raise RuntimeError("Instala g++, clang++ o las herramientas C++ de Visual Studio.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", default="docs/resultados-pruebas.md")
    parser.add_argument("--native-only", action="store_true", help="Omitir pruebas del analizador Python")
    args = parser.parse_args()
    BUILD.mkdir(parents=True, exist_ok=True)
    compiler, environment = compiler_environment()
    records = []
    for name, sources in SUITES.items():
        executable = BUILD / (name + (".exe" if os.name == "nt" else ""))
        if compiler == "cl":
            command = [compiler, "/nologo", "/EHsc", "/std:c++14", "/W4", "/WX", "/UNDEBUG",
                       "/Itests/fakes", f"/Fe:{executable}", f"/Fo:{BUILD}{os.sep}", *sources]
        else:
            command = [compiler, "-std=c++14", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                       "-Itests/fakes", *sources, "-o", str(executable)]
        command[0] = shutil.which(compiler, path=environment.get("PATH", environment.get("Path"))) or compiler
        run(command, env=environment)
        arguments = [str(SYNTHETIC_CSV)] if name == "replay" else []
        output = run([str(executable), *arguments], env=environment)
        print(output, end="")
        (BUILD / (name + ".txt")).write_text(output, encoding="utf-8")
        if name == "control":
            records.append(["UNIT-01", "regresion con aserciones activas",
                            "senal, respuesta del filtro, rampa y antirrebote", "PASA"])
        for line in output.splitlines():
            fields = line.split("|")
            if len(fields) == 4:
                records.append(fields)
    if not args.native_only:
        python_environment = os.environ.copy()
        python_environment["PYTHONPATH"] = str(ROOT / "Inmoov_EMG/analizador_senal")
        temporary = BUILD / "python-temp"
        temporary.mkdir(exist_ok=True)
        for key in ("TMPDIR", "TMP", "TEMP"):
            python_environment[key] = str(temporary)
        output = run([sys.executable, "-B", "-m", "unittest", "discover", "-s",
                      "Inmoov_EMG/analizador_senal/tests", "-v"], env=python_environment)
        print(output, end="")
        (BUILD / "python.txt").write_text(output, encoding="utf-8")
        records.append(["PC-01", "protocolo, comandos, CSV antiguo y fallas 16/32", "suite Python sin fallos", "PASA"])
        records.append(["PC-SEG", "ida y vuelta AES-256-GCM, rechazo de contrasena incorrecta, archivos y manifiesto alterados",
                        "suite de privacidad, consentimiento y retencion sin fallos", "PASA"])
        # Comparar todas las columnas permite detectar cambios de valores al leer.
        run([sys.executable, "-B", "-c",
             "import csv, sys; from senal import FIELDS, read_csv; "
             "rows=list(csv.DictReader(open(sys.argv[1], encoding='utf-8', newline=''))); "
             "samples=read_csv(sys.argv[1]); "
             "assert len(samples)==len(rows)==60; "
             "assert list(rows[0])==FIELDS; "
             "assert all(str(s.row()[k])==r[k] for s,r in zip(samples,rows) for k in FIELDS)",
             str(SYNTHETIC_CSV)], env=python_environment)
        records.append(["PC-DATOS", "leer CSV sintetico sin reinterpretar valores",
                        "60 filas y 22 columnas conservadas", "PASA"])
    manifest = json.loads((DATA / "manifiesto.json").read_text(encoding="utf-8"))
    with SYNTHETIC_CSV.open(encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream)
        columns, rows = reader.fieldnames, list(reader)
    if not (manifest["tipo"] == "sintetico" and manifest["archivo"] == SYNTHETIC_CSV.name
            and manifest["filas"] == len(rows) == 60 and manifest["columnas"] == columns
            and manifest["paso_ms"] == 500
            and all(int(row["device_ms"]) == index * manifest["paso_ms"]
                    for index, row in enumerate(rows))
            and manifest["sha256"] == hashlib.sha256(SYNTHETIC_CSV.read_bytes()).hexdigest()):
        raise RuntimeError("Manifiesto sintetico distinto del CSV de prueba")
    records.append(["EVIDENCIA", "manifiesto del CSV sintetico, sin datos biometricos",
                    "origen, 60 filas, columnas, periodo y SHA256 verificados", "PASA"])
    report = ROOT / args.report
    report.parent.mkdir(parents=True, exist_ok=True)
    header = ("# Resultados de pruebas automatizadas\n\n"
              f"Generado con `python scripts/test.py`, compilador `{compiler}`, "
              "aserciones activas y advertencias como errores.\n\n"
              "Las pruebas C++ usan entradas sinteticas. Los drivers GPIO/Servo/UART usan dobles; "
              "Application, cola, filtros y persistencia por bytes ejecutan el codigo del proyecto. "
              "La ISR AVR se verifica por compilacion; estas pruebas no simulan interrupciones de hardware "
              "ni demuestran ausencia de jitter o carreras en una placa. FW8 usa ThreeLevelControl, sin calibracion.\n\n"
              "LAT-SW termina al ordenar el pulso (ver LAT-SW; no demuestra R1 de 50 ms); R2-SW mide el recorrido ordenado; R9-SW usa "
              "un bucle ideal de 1 ms. Son comparaciones de software contra los requisitos del proyecto, "
              "no mediciones de PWM, movimiento o consumo. REPLAY-30S retiene cada fila sintetica "
              "500 ms en un bucle ideal de 1 ms, sin reconstruir adquisicion ni filtro. PC-DATOS "
              "lee ese mismo CSV y EVIDENCIA verifica su manifiesto; no validan fisicamente el firmware.\n\n"
              "| Caso | Resultado esperado | Resultado obtenido | Estado |\n"
              "| --- | --- | --- | --- |\n")
    report.write_text(header + "".join("| " + " | ".join(row) + " |\n" for row in records), encoding="utf-8")
    print(f"{len(records)} casos reportados. Tabla: {report.relative_to(ROOT)}")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
