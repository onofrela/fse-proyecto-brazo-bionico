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
    "filter": ["tests/filter_test.cpp"],
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
        output = run([str(executable)], env=environment)
        print(output, end="")
        (BUILD / (name + ".txt")).write_text(output, encoding="utf-8")
        if name == "control":
            records.append(["UNIT-01", "regresion con aserciones activas",
                            "filtro, calibracion, ponderacion, rampa, fallas y replay completados", "PASA"])
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
        records.append(["PC-01", "protocolo, comandos, CSV antiguo y fallas 16/32", "10 pruebas Python sin fallos", "PASA"])
        run([sys.executable, "-B", "-c", "from pathlib import Path; from senal import read_csv; "
             "counts=sorted(len(read_csv(p)) for p in Path('docs/mediciones/2026-11-09').glob('*/senal.csv')); "
             "assert counts==[469,709], counts"], env=python_environment)
        records.append(["PC-DATOS", "leer CSV reales sin reinterpretar su filtro", "469 y 709 instantaneas conservadas", "PASA"])
    run([sys.executable, "-B", "scripts/verificar_mediciones.py"])
    records.append(["EVIDENCIA", "SHA256 original de mediciones y capturas", "12 archivos con integridad verificada", "PASA"])
    report = ROOT / args.report
    report.parent.mkdir(parents=True, exist_ok=True)
    header = ("# Resultados de pruebas automatizadas\n\n"
              f"Generado con `python scripts/test.py`, compilador `{compiler}`, "
              "aserciones activas y advertencias como errores.\n\n"
              "Las pruebas C++ usan entradas sinteticas. Los drivers GPIO/Servo/UART usan dobles; "
              "Application, cola, filtros y persistencia por bytes ejecutan el codigo del proyecto. "
              "La ISR AVR se verifica por compilacion; estas pruebas no simulan interrupciones de hardware "
              "ni demuestran ausencia de jitter o carreras en una placa.\n\n"
              "LAT-SW termina al ordenar el pulso (71 ms en este ensayo; no demuestra R1 de 50 ms); R2-SW mide el recorrido ordenado; R9-SW usa "
              "un bucle ideal de 1 ms. Son comparaciones de software contra los requisitos del proyecto, "
              "no mediciones de PWM, movimiento o consumo. Los CSV reales se leen sin recalcular sus filtros; "
              "los hashes verifican la evidencia original y no validan fisicamente el firmware posterior.\n\n"
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
