# Historial de versiones

La base modular recibida mediante pull es a37d208, conservada sin reescribir. La serie histórica siguiente añade evidencia, filtrado, calibración, decisión y telemetría. Las etiquetas ya fueron creadas localmente; sus commits se verificaron con `git cat-file -t` y corresponden a la tabla.

La entrega final es **FW8**, con control fijo por tres niveles y sin calibración, en la rama `limpieza-fw8`. La versión de entrega prevista es **v1.0.0**. El orquestador debe crear la etiqueta anotada local en `HEAD` después del commit final del paso 12, sin publicarla. Este paso deja `build/commit-msg.txt` preparado; el sandbox no permite escribir en `.git`, por lo que el commit y la etiqueta siguen pendientes. La versión `v0.6.2` citada anteriormente no existe.

FW8 clasifica el promedio de 200 envolventes a 1 kHz con fronteras 5 y 10 y destinos 0°/90°/180°. Se retiraron calibración, control ponderado, persistencia de umbrales y LED RGB. EEPROM solo conserva el contador de arranque verificado. El montaje usa 4 celdas 18650 para los servos MG995 y una pila de 9 V para el Arduino; D7/D12/D13 quedan libres. El firmware mantiene el identificador de protocolo `FW=8`; `v1.0.0` identifica la entrega del repositorio.

Las variantes `normal` (sin UART) y `depuracion` (UART habilitada, telemetría periódica apagada hasta `p`) se compilaron el 2026-11-21 con AVR 1.8.6 y Servo 1.2.2, ambas con D2 habilitado. Usan respectivamente 7128/11404 B de Flash y 508/1002 B de SRAM global. `python scripts/test.py` regeneró [los resultados](resultados-pruebas.md): 44 casos reportados y 30 pruebas Python sin fallos. LAT-FW8 obtuvo 161–360 ms para amplitud 12 ADC y 58–257 ms para 40 ADC hasta el primer destino/paso escrito en simulación. **R1 (≤50 ms) no cumple por diseño**; la validación física sigue pendiente. D11 documenta el motivo y el costo en [la bitácora](bitacora.md#retiro-de-calibración-y-control-ponderado-en-fw8).

| Etiqueta | Commit | Fecha | Alcance |
| --- | --- | --- | --- |
| v0.5.1-mediciones | 62e46f3 | 2026-11-08T23:21:43-06:00 | test(sensor): conservar mediciones reales y capturas de actividad |
| v0.6.0-filtro | e43b9c8 | 2026-11-09T00:35:59-06:00 | feat(filtro): incorporar modulo digital verificable a 1 kHz |
| v0.6.0-senal | 588bb7c | 2026-11-09T02:12:31-06:00 | feat(senal): integrar filtrado sin ampliar las interrupciones ADC |
| v0.6.0-calibracion | f903cf3 | 2026-11-09T03:34:05-06:00 | fix(calibracion): guiar reposo y contracciones con percentiles robustos |
| v0.6.0-control | f84ece1 | 2026-11-09T05:03:34-06:00 | feat(decision): ponderar actividad y completar el cierre de la mano |
| v0.6.0-telemetria | 0906c9b | 2026-11-09T06:51:34-06:00 | feat(diagnostico): conectar telemetria modular y analizador de senal |
| v0.6.0-evidencia | 349d613 | 2026-11-09T07:49:31-06:00 | fix(evidencia): conservar bytes originales y verificar archivos por SHA256 |
| v0.6.0 | 3a56b3c | 2026-11-09T08:56:13-06:00 | docs(sensor): documentar inestabilidad y diagnostico pendiente |

Para recorrer la serie histórica: `git log --reverse --oneline a37d208..v0.6.0`. Para comparar la base modular y el cierre de esa serie: `git diff a37d208..v0.6.0`. Las etiquetas son anotadas y permanecen locales, sin publicar.

En esas revisiones se desactivó la normalización de los CSV y JSON de evidencia para preservar sus bytes. Las capturas reales ya no se versionan en el árbol actual por contener datos biométricos/de salud; sus resultados se resumen en el informe del Avance 4. Se retiraron aquellas reglas y el verificador de mediciones, sin reescribir el historial: las revisiones antiguas aún contienen las capturas. El analizador guarda nuevas sesiones fuera del repositorio, con consentimiento, cifrado por defecto e integridad autenticada. `python scripts/test.py` verifica el manifiesto del [dato sintético](../tests/datos/README.md) y regenera el informe actual sin capturas reales.

El estado del módulo del sensor y las hipótesis por comprobar se describen en [diagnóstico del sensor](diagnostico-sensor.md). La revisión no se considera validada físicamente mientras persista la inestabilidad.
