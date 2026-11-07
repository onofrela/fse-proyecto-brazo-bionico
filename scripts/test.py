#!/usr/bin/env python3
"""Pruebas C++ nativas y tabla de evidencia; no conectan ni accionan hardware."""
from pathlib import Path
import argparse
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build" / "native"
SUITES = {
    "control": ["tests/control_test.cpp"],
    "integration": ["tests/integration_test.cpp", "Inmoov_EMG/Application.cpp"],
    "storage": ["tests/storage_test.cpp"],
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
            command = [compiler, "-std=c++11", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                       "-Itests/fakes", *sources, "-o", str(executable)]
        command[0] = shutil.which(compiler, path=environment.get("PATH", environment.get("Path"))) or compiler
        run(command, env=environment)
        output = run([str(executable)], env=environment)
        print(output, end="")
        (BUILD / (name + ".txt")).write_text(output, encoding="utf-8")
        if name == "control":
            records.append(["UNIT-01", "8 grupos de regresion sin aserciones fallidas",
                            "8 grupos completados (filtro, umbrales, rampa, UI, calibracion, AD8232 e integracion)", "PASA"])
        for line in output.splitlines():
            fields = line.split("|")
            if len(fields) == 4:
                records.append(fields)
    report = ROOT / args.report
    report.parent.mkdir(parents=True, exist_ok=True)
    header = ("# Resultados de pruebas automatizadas\n\n"
              f"Generado con `python scripts/test.py`, compilador `{compiler}`, "
              "aserciones activas y advertencias como errores.\n\n"
              "Todas las entradas son sinteticas. Los drivers GPIO/Servo/UART usan dobles; "
              "Application, cola, filtros y persistencia por bytes ejecutan el codigo del proyecto. "
              "La ISR AVR se verifica por compilacion; estas pruebas no simulan interrupciones de hardware "
              "ni demuestran ausencia de jitter o carreras en una placa.\n\n"
              "R1-SW termina al ordenar el pulso; R2-SW mide el recorrido ordenado; R9-SW usa "
              "un bucle ideal de 1 ms. Son comparaciones de software contra los requisitos del proyecto, "
              "no mediciones de PWM, movimiento o consumo.\n\n"
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
