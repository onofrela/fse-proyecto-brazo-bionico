# Historial de versiones

Las etiquetas anotadas permiten comparar las versiones del firmware. Los programas `Prueba_AD8232_Serial/` y `Prueba_Servo_Serial/` son herramientas independientes de diagnóstico de hardware.

| Etiqueta | Cambio verificable |
| --- | --- |
| v0.1.0 | Codigo base `74de538` |
| v0.2.0 | separar lógica en Signal, Decision, Inputs, Motion y Calibration; mantener fachada Control; nombrar parámetros; regresión y compilación UNO verificadas |
| v0.3.0 | Application, Acquisition, cola circular atómica, Outputs, Storage y Diagnostics; timeout ADC, errores de servo/UART/EEPROM y recuperación de adquisición |
| v0.4.0 | pruebas de integración/drivers/persistencia con 27 casos reportados; scripts de configuración, compilación y prueba; tabla esperado/obtenido |
| v0.4.1 | Documentación de arquitectura, decisiones, alcance de requisitos y protocolo de medición; política LF para scripts Bash |

Consultar `git log --oneline --decorate` y `git show v0.3.0`. Se puede revisar cada paso con `git diff v0.1.0..v0.2.0`, `git diff v0.2.0..v0.3.0`, etc.
