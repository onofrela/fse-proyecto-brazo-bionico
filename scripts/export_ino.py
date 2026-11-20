#!/usr/bin/env python3
"""Genera Inmoov_EMG.ino autocontenido y su copia para Arduino IDE."""
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Inmoov_EMG"
DESTINATIONS = [
    SOURCE / "Inmoov_EMG.ino",
    ROOT / "Arduino_IDE/Inmoov_EMG_Servos_Invertidos/Inmoov_EMG_Servos_Invertidos.ino",
]
LOCAL_INCLUDE = re.compile(r'^\s*#include "([^"\n]+)"\s*$', re.MULTILINE)
MODULE_START = "#ifndef ARDUINO // INMOOV_NATIVE_MODULE\n"
MODULE_END = "#endif // INMOOV_NATIVE_MODULE"


def main():
    headers = []
    visited = set()

    def read(path):
        content = path.read_text(encoding="utf-8")
        if path.suffix == ".cpp":
            if not content.startswith(MODULE_START) or not content.rstrip().endswith(MODULE_END):
                raise ValueError(f"Falta guarda de modulo nativo en {path}")
            content = content[len(MODULE_START):].rstrip()[:-len(MODULE_END)].rstrip()
        for name in LOCAL_INCLUDE.findall(content):
            header = path.parent / name
            if header not in visited:
                visited.add(header)
                headers.append((header, read(header)))
        return LOCAL_INCLUDE.sub("", content).strip()

    implementations = [(path, read(path)) for path in sorted(SOURCE.glob("*.cpp"))]
    sketch = """namespace { Hand::Application application; }
void setup() { application.begin(millis()); }
void loop() { application.tick(millis()); }
"""
    banner = """/*
 * Mano InMoov - version autocontenida para Arduino IDE / Arduino Uno R3.
 * Medio (D6), anular (D10) y menique (D11): 0 -> 180, 90 -> 90, 180 -> 0.
 * Pulgar (D3) e indice (D5) conservan su direccion.
 * Abrir este archivo y seleccionar Arduino Uno y su puerto.
 * Bibliotecas: Servo; EEPROM incluida en Arduino AVR Boards.
 * Generado con python scripts/export_ino.py desde los modulos .h/.cpp de Inmoov_EMG/.
 * Toda la configuracion esta incluida abajo; no requiere archivos del proyecto.
 * Monitor Serie a 115200: enviar ? y comprobar SERVO_REVERSE_MASK,28.
 * Los .cpp de esta carpeta se reservan para pruebas nativas, evitando duplicados.
 */
#include <Arduino.h>
"""
    sections = [banner]
    for path, content in headers + implementations:
        sections.append(f"// ===== {path.name} =====\n{content}\n")
    sections.append("// ===== Entrada Arduino =====\n" + sketch)
    for destination in DESTINATIONS:
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text("\n".join(sections), encoding="utf-8")
        print(destination.relative_to(ROOT))


if __name__ == "__main__":
    main()
