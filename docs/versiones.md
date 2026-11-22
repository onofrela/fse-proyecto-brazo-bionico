# Historial de versiones

La rama de entrega es `main` y la versión actual del repositorio es **v1.0.1**, registrada en [VERSION](../VERSION). El firmware conserva el identificador de protocolo **FW8**. `v1.0.0` corresponde al cierre de FW8 del 21 de noviembre; `v1.0.1` incorpora las precisiones de alimentación del 22 de noviembre y la consolidación del historial y sus referencias.

Los commits y las etiquetas anotadas del proyecto usan exclusivamente la identidad **Leonardo Arroyo Onofre <arroyoonofrel@gmail.com>** como autor, committer y etiquetador, según corresponda. La reconstrucción local conserva exactamente las fechas originales de autor y committer, las fechas de creación de las etiquetas, los mensajes descriptivos y los árboles de archivos de los commits existentes. Se retiran los trailers de coautoría; las notas de asistencia y las atribuciones del código de origen se conservan. Los identificadores de los commits y de las etiquetas cambian al corregir la identidad y los padres.

Los 14 commits posteriores al antiguo `main` se integraron manteniendo su orden; la rama temporal se retiró. La consolidación documental se registra en un commit adicional con su fecha real. Las versiones v0.2.0–v0.4.1 pertenecen a la línea histórica paralela recibida: se conservan sus etiquetas y su contenido sin introducirlos de nuevo en `main`. La base modular detallada se consulta como `v0.5.0`.

Estos cambios son locales; no se ha publicado el historial reconstruido en `origin`. El respaldo anterior y el mapa de identificadores se entregan por separado. Las fechas de la tabla son las fechas de autor de los commits de cada versión; las fechas de creación de las etiquetas también se preservan, aunque sean posteriores.

FW8 clasifica el promedio de 200 envolventes a 1 kHz con fronteras 5 y 10 y destinos 0°/90°/180°. Se retiraron calibración, control ponderado, persistencia de umbrales y LED RGB. EEPROM solo conserva el contador de arranque verificado. El montaje usa 4 celdas 18650 para los servos MG995 y una pila de 9 V para el Arduino; D7/D12/D13 quedan libres. El firmware mantiene el identificador de protocolo `FW=8`; `v1.0.0` identifica el cierre de FW8 y `v1.0.1` la revisión documental vigente; el número del protocolo es independiente del versionado del repositorio.

Las variantes `normal` (sin UART) y `depuracion` (UART habilitada, telemetría periódica apagada hasta `p`) se compilaron el 2026-11-21 con AVR 1.8.6 y Servo 1.2.2, ambas con D2 habilitado. Usan respectivamente 7128/11404 B de Flash y 508/1002 B de SRAM global. `python scripts/test.py` regeneró [los resultados](resultados-pruebas.md): 44 casos reportados y 30 pruebas Python sin fallos. LAT-FW8 obtuvo 161–360 ms para amplitud 12 ADC y 58–257 ms para 40 ADC hasta el primer destino/paso escrito en simulación. **R1 (≤50 ms) no cumple por diseño**; la validación física sigue pendiente. D11 documenta el motivo y el costo en [la bitácora](bitacora.md#retiro-de-calibración-y-control-ponderado-en-fw8).

| Etiqueta | Commit | Fecha del commit | Alcance |
| --- | --- | --- | --- |
| v0.1.0 | `9985eb6adc34` | 2026-11-03T18:55:36-06:00 | init: Código Base |
| v0.2.0 | `70f43e49ee7d` | 2026-11-08T23:15:23-06:00 | refactor: separar senal, calibracion, entradas y movimiento |
| v0.3.0 | `0ac280890530` | 2026-11-08T23:33:06-06:00 | feat: integrar cola ADC atomica y drivers con manejo de fallas |
| v0.4.0 | `7c8649674386` | 2026-11-08T23:34:20-06:00 | test: agregar 27 casos, fallas inducidas y compilacion reproducible |
| v0.4.1 | `859defc68af5` | 2026-11-08T23:35:56-06:00 | docs: documentar arquitectura, decisiones y validacion de requisitos |
| v0.5.0 | `5e91a3ee5494` | 2026-11-08T22:42:00-06:00 | docs(versiones): registrar commits por modulo y conservar el historial previo |
| v0.5.0-integracion | `01ef1a0ce955` | 2026-11-06T22:57:00-06:00 | refactor(application): integrar modulos y reducir el sketch a setup y loop |
| v0.5.0-modulos | `74972c170ff3` | 2026-11-05T22:50:00-06:00 | test(control): verificar regresion tras extraer los cinco modulos |
| v0.5.0-pruebas | `c809c04d5801` | 2026-11-07T21:47:00-06:00 | build: integrar verificacion Bash y preservar finales LF |
| v0.5.1-mediciones | `da2a3999427a` | 2026-11-08T23:21:43-06:00 | test(sensor): conservar mediciones reales y capturas de actividad |
| v0.6.0 | `0bebea5e70d2` | 2026-11-09T08:56:13-06:00 | docs(sensor): documentar inestabilidad y diagnostico pendiente |
| v0.6.0-calibracion | `571c94cee12b` | 2026-11-09T03:34:05-06:00 | fix(calibracion): guiar reposo y contracciones con percentiles robustos |
| v0.6.0-control | `50f1a25783bd` | 2026-11-09T05:03:34-06:00 | feat(decision): ponderar actividad y completar el cierre de la mano |
| v0.6.0-evidencia | `1e93a89a8a9a` | 2026-11-09T07:49:31-06:00 | fix(evidencia): conservar bytes originales y verificar archivos por SHA256 |
| v0.6.0-filtro | `64dcaa9d997e` | 2026-11-09T00:35:59-06:00 | feat(filtro): incorporar modulo digital verificable a 1 kHz |
| v0.6.0-senal | `7b5a8512c4c9` | 2026-11-09T02:12:31-06:00 | feat(senal): integrar filtrado sin ampliar las interrupciones ADC |
| v0.6.0-telemetria | `ce86b7be33bd` | 2026-11-09T06:51:34-06:00 | feat(diagnostico): conectar telemetria modular y analizador de senal |
| v1.0.0 | `a36ab6cb1816` | 2026-11-21T17:27:05-06:00 | docs(fw8): sincronizar arquitectura, requisitos, bitacora, resultados y versiones |
| v1.0.1 | `main` / `v1.0.1^{commit}` | Fecha real del commit de consolidación | Precisiones de alimentación y cierre documental del historial, autoría y versiones |

Para recorrer la serie histórica: `git log --reverse --oneline v0.5.0..v0.6.0`. Para comparar la base modular y el cierre de esa serie: `git diff v0.5.0..v0.6.0`. Para ver los cambios posteriores al cierre de FW8: `git log --oneline v1.0.0..v1.0.1`. Todas las etiquetas de la tabla son anotadas y permanecen locales.

Las capturas reales ya no se versionan en el árbol actual por contener datos biométricos/de salud; sus resultados se resumen en el informe del Avance 4. Las revisiones antiguas todavía las contienen y sus bytes se preservan en la reconstrucción. El analizador guarda nuevas sesiones fuera del repositorio, con consentimiento, cifrado por defecto e integridad autenticada. `python scripts/test.py --report build/resultados-pruebas.md` comprueba el manifiesto del [dato sintético](../tests/datos/README.md) sin reemplazar el informe histórico.

El estado del módulo del sensor y las hipótesis por comprobar se describen en [diagnóstico del sensor](diagnostico-sensor.md). La revisión no se considera validada físicamente mientras persista la inestabilidad.
