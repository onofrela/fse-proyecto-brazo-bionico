#!/usr/bin/env python3
"""Compila UNO con versiones verificadas. No instala ni carga firmware."""
from pathlib import Path
import argparse
import json
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
CORE = "1.8.6"
SERVO = "1.2.2"


def run(command):
    result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError(f"Comando fallo ({result.returncode}): {command}\n{result.stdout}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cli", help="Ruta a arduino-cli")
    parser.add_argument("--config", help="Configuracion YAML de Arduino CLI")
    parser.add_argument("--servo-library", type=Path, help="Carpeta Servo con library.properties")
    parser.add_argument("--no-debug", action="store_true")
    parser.add_argument("--no-probe", action="store_true")
    args = parser.parse_args()
    cli = args.cli or shutil.which("arduino-cli")
    local = Path(os.environ.get("LOCALAPPDATA", ""))
    if not cli and os.name == "nt":
        candidate = local / "Programs/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe"
        if candidate.is_file():
            cli = str(candidate)
    if not cli:
        raise RuntimeError("No se encontro arduino-cli; usa --cli o instalalo y agregalo a PATH.")
    build = ROOT / "build"
    build.mkdir(exist_ok=True)
    base = [cli]
    data = local / "Arduino15"
    if args.config:
        base += ["--config-file", str(Path(args.config).resolve())]
    elif os.name == "nt" and (data / "package_index.json").is_file():
        # Las rutas se descubren por equipo; no se guardan rutas de un usuario en Git.
        configuration = build / "arduino-local.yaml"
        directories = {"data": str(data), "downloads": str(data / "staging"),
                       "user": str(Path.home() / "Documents/Arduino")}
        configuration.write_text(json.dumps({"directories": directories}), encoding="utf-8")
        base += ["--config-file", str(configuration)]
    platforms = json.loads(run(base + ["core", "list", "--format", "json"]))["platforms"]
    if not any(p["id"] == "arduino:avr" and p.get("installed_version") == CORE for p in platforms):
        raise RuntimeError(f"Se requiere arduino:avr@{CORE}. Ejecuta scripts/bootstrap.sh o el equivalente del README.")
    servo = args.servo_library
    if servo is None:
        installed = json.loads(run(base + ["lib", "list", "--all", "--format", "json"]))["installed_libraries"]
        candidates = [Path(item["library"]["install_dir"]) for item in installed
                      if item["library"]["name"] == "Servo" and item["library"]["version"] == SERVO]
        # Algunas instalaciones antiguas de IDE usan Arduino15/libraries.
        candidates.append(data / "libraries/Servo")
        servo = next((path for path in candidates if (path / "library.properties").is_file()), None)
    if servo is None:
        raise RuntimeError(f"No se encontro Servo@{SERVO}; instala la version fijada.")
    properties = dict(line.split("=", 1) for line in (servo / "library.properties").read_text(encoding="utf-8").splitlines() if "=" in line)
    if properties.get("name") != "Servo" or properties.get("version") != SERVO:
        raise RuntimeError(f"La biblioteca seleccionada no es Servo@{SERVO}: {servo}")
    variant = "uno" + ("-sin-uart" if args.no_debug else "") + ("-sin-sonda" if args.no_probe else "")
    command = base + ["compile", "--fqbn", "arduino:avr:uno", "--warnings", "all", "--library", str(servo.resolve()),
                      "--build-path", str(build / variant)]
    flags = []
    if args.no_debug:
        flags.append("-DEMG_SERIAL_DEBUG=0")
    if args.no_probe:
        flags.append("-DEMG_SAMPLE_PROBE=0")
    if flags:
        command += ["--build-property", "compiler.cpp.extra_flags=" + " ".join(flags)]
    command.append("Inmoov_EMG")
    output = run(command)
    (build / (variant + ".txt")).write_text(output, encoding="utf-8")
    print(output)


if __name__ == "__main__":
    try:
        main()
    except (OSError, KeyError, ValueError, RuntimeError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
