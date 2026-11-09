#!/usr/bin/env bash
# Configuracion repetible, con errores de cada comando propagados al llamador.
set -Eeuo pipefail
trap 'printf "Error de configuracion en linea %s\n" "$LINENO" >&2' ERR
command -v arduino-cli >/dev/null || { printf 'Falta arduino-cli en PATH\n' >&2; exit 1; }
arduino-cli core update-index
arduino-cli core install arduino:avr@1.8.6
arduino-cli lib install 'Servo@1.2.2'
printf 'Listo: UNO, AVR 1.8.6 y Servo 1.2.2. Ejecuta python scripts/build.py\n'
