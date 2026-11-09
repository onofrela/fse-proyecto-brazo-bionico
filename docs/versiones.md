# Historial de versiones

La base modular recibida mediante pull es a37d208, conservada sin reescribir. La serie siguiente permanece en main y añade evidencia, filtrado, calibración, decisión y telemetría. No se crearon ramas adicionales.

| Etiqueta | Commit | Fecha | Alcance |
| --- | --- | --- | --- |
| v0.5.1-mediciones | 62e46f3 | 2026-11-08T23:21:43-06:00 | test(sensor): conservar mediciones reales y capturas de actividad |
| v0.6.0-filtro | e43b9c8 | 2026-11-09T00:35:59-06:00 | feat(filtro): incorporar modulo digital verificable a 1 kHz |
| v0.6.0-senal | 588bb7c | 2026-11-09T02:12:31-06:00 | feat(senal): integrar filtrado sin ampliar las interrupciones ADC |
| v0.6.0-calibracion | f903cf3 | 2026-11-09T03:34:05-06:00 | fix(calibracion): guiar reposo y contracciones con percentiles robustos |
| v0.6.0-control | f84ece1 | 2026-11-09T05:03:34-06:00 | feat(decision): ponderar actividad y completar el cierre de la mano |
| v0.6.0-telemetria | 0906c9b | 2026-11-09T06:51:34-06:00 | feat(diagnostico): conectar telemetria modular y analizador de senal |
| v0.6.0-evidencia | 349d613 | 2026-11-09T07:49:31-06:00 | fix(evidencia): conservar bytes originales y verificar archivos por SHA256 |
| v0.6.0 | Este commit | 2026-11-09T08:56:13-06:00 | docs(sensor): documentar inestabilidad y diagnostico pendiente |

Para recorrer la serie: `git log --reverse --oneline a37d208..main`. Para comparar la base modular y la revisión final: `git diff a37d208..v0.6.0`. Las etiquetas son anotadas y permanecen locales hasta un push explícito.

La normalización de texto de Git se desactiva para los CSV y JSON de evidencia, preservando BOM y finales de línea. Comprueba los blobs de la revisión final con `python scripts/verificar_mediciones.py --git-ref HEAD`.

El estado del módulo del sensor y las hipótesis por comprobar se describen en [diagnóstico del sensor](diagnostico-sensor.md). La revisión no se considera validada físicamente mientras persista la inestabilidad.
