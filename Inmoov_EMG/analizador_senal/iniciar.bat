@echo off
setlocal
cd /d "%~dp0"
if exist ".venv\Scripts\python.exe" (
  ".venv\Scripts\python.exe" analizador.py
) else (
  python analizador.py
)
if errorlevel 1 (
  echo.
  echo Si faltan bibliotecas, ejecuta: python -m pip install -r requirements.txt
  pause
)
