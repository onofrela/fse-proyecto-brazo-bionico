#!/usr/bin/env bash
# Bash para Linux/macOS/Git Bash. No se escribe ningun dispositivo.
set -Eeuo pipefail
trap 'printf "Error de verificacion en linea %s\n" "$LINENO" >&2' ERR
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
python_cmd="${PYTHON:-python3}"
command -v "$python_cmd" >/dev/null || { printf 'Falta Python 3\n' >&2; exit 1; }
"$python_cmd" scripts/test.py
"$python_cmd" scripts/build.py
"$python_cmd" scripts/build.py --no-debug --no-probe
