# Dato sintetico de prueba

`senal_sintetica.csv` es **sintetico**: no contiene senales, fechas ni datos de
personas. Sus 22 columnas son las del analizador (`senal.FIELDS`). Se genera
con `python -B tests/datos/generar.py`, desde cero y sin leer capturas.

Para i = 0..59: tiempo = i * 500 ms, base = 338,
ADC = 338 + ((i * 7) % 21) - 10 y envolvente =
`(2, 7, 12, 2, 12, 2)[i // 10] + (i % 3) - 1`.
La fecha ficticia parte de 2000-01-01 UTC, sesion = 0, fronteras = 5/10,
referencia = 5 V y fallas/contadores = 0. Los voltajes son
`round(valor_adc * 5 / 1023, 6)` y los indicadores se derivan de flags = 0.
El generador fija UTF-8 sin BOM y LF; `manifiesto.json` identifica origen,
columnas, filas, periodo y SHA256 del CSV.

`PC-DATOS` verifica la lectura sin cambiar los valores; `EVIDENCIA` verifica
el manifiesto sintetico y reemplaza los hashes de capturas reales. Se retira
`scripts/verificar_mediciones.py` porque ya no hay mediciones versionadas.
`REPLAY-30S` mantiene cada fila durante 500 pasos ideales de 1 ms y ejercita
ThreeLevelControl y Motion durante 30 s simulados (ultima fila: 29.5 s).
Comprueba destinos y rampas en reposo/medio/alto/reposo/alto/reposo.
No reconstruye adquisicion, filtro ni muestras fisicas; no valida R1 ni R6.

Las capturas reales del AD8232 son datos biometricos/de salud y ya no se
versionan. Sus resultados se resumen en el informe del Avance 4. El historial
publicado permanece sin reescribir.
