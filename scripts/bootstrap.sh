#!/usr/bin/env bash
# Configuracion repetible, con errores de cada comando propagados al llamador.
set -Eeuo pipefail
trap 'printf "Error de configuracion en linea %s\n" "$LINENO" >&2' ERR
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
python_cmd="${PYTHON:-python3}"
command -v "$python_cmd" >/dev/null || { printf 'Falta Python 3 (o define PYTHON)\n' >&2; exit 1; }
command -v arduino-cli >/dev/null || { printf 'Falta arduino-cli en PATH\n' >&2; exit 1; }
arduino-cli core update-index
arduino-cli core install arduino:avr@1.8.6
arduino-cli lib install 'Servo@1.2.2'
"$python_cmd" -m pip install -r Inmoov_EMG/analizador_senal/requirements.txt
printf 'Listo: UNO, AVR 1.8.6, Servo 1.2.2 y dependencias Python. Ejecuta python scripts/build.py\n'
