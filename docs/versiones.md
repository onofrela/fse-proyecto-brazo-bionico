# Historial de versiones

La rama principal es `main`. Parte del codigo base y distribuye por responsabilidad la extraccion, integracion, pruebas y documentacion. Las etiquetas permiten comparar las etapas y consultar las versiones preliminares.

| Etiqueta | Alcance |
| --- | --- |
| v0.1.0 | Codigo base del proyecto |
| v0.5.0-modulos | Apoyos de servo/ADC, configuracion y extraccion individual de los cinco modulos de logica |
| v0.5.0-integracion | Correcciones, drivers conectados uno a uno, cola ADC y coordinador Application |
| v0.5.0-pruebas | Pruebas separadas de persistencia, drivers e integracion; tabla de 27 casos y scripts reproducibles |
| v0.5.0 | Documentacion consolidada, diagrama, decisiones, requisitos, protocolo y registro detallado |

Los sketches de apoyo se incorporan como herramientas independientes para verificar servo y ADC. Los resultados fisicos se registran durante los ensayos del montaje.

## Commits por paso

| Paso | Cambio |
| --- | --- |
| 01 | docs: registrar el plan de extraccion desde el codigo base |
| 02 | test(hardware): incorporar prueba serial independiente de un servo |
| 03 | docs(servo): explicar conexion y uso del apoyo de actuadores |
| 04 | test(hardware): incorporar lectura cruda del ADC y contacto AD8232 |
| 05 | docs(adc): explicar lecturas crudas, escala y contactos del sensor |
| 06 | refactor(config): nombrar parametros de temporizacion y perifericos |
| 07 | refactor(signal): extraer senal AD8232 y media movil |
| 08 | refactor(decision): extraer umbrales e histeresis muscular |
| 09 | refactor(inputs): extraer antirrebote de entradas |
| 10 | refactor(motion): extraer rampa simultanea de dedos |
| 11 | refactor(calibration): extraer maquina de estados de calibracion |
| 12 | test(control): verificar regresion tras extraer los cinco modulos |
| 13 | fix(control): invalidar datos inseguros y permitir reiniciar el filtro |
| 14 | refactor(outputs): aislar GPIO y servos y conectar el driver al sketch |
| 15 | refactor(diagnostics): extraer comandos y telemetria UART del sketch |
| 16 | feat(storage): separar la transaccion incremental de calibracion |
| 17 | refactor(storage): integrar EEPROM verificada y guardado por pasos |
| 18 | feat(ipc): incorporar cola circular SPSC de muestras completas |
| 19 | refactor(acquisition): mover ISR y conectar la cola al procesamiento |
| 20 | refactor(application): integrar modulos y reducir el sketch a setup y loop |
| 21 | test(storage): cubrir guardado incremental y fallas de EEPROM |
| 22 | test(drivers): comprobar servos y UART con dobles de hardware |
| 23 | test(integration): cubrir flujo completo y recuperacion de adquisicion |
| 24 | test: automatizar ejecucion y publicar resultados esperados y obtenidos |
| 25 | build: fijar dependencias Arduino y separar la compilacion reproducible |
| 26 | build: integrar verificacion Bash y preservar finales LF |
| 27 | docs(arquitectura): conectar bloques y explicar el arranque autonomo |
| 28 | docs(bitacora): relacionar decisiones, prioridades y desviaciones |
| 29 | docs(requisitos): contrastar la implementacion con requisitos y limites |
| 30 | docs(pruebas): describir mediciones fisicas y el uso de los apoyos |
| 31 | docs(readme): consolidar modulos, apoyos y verificacion del proyecto |

El ultimo commit de documentacion se identifica con `v0.5.0`. Para recorrer la evolucion: `git log --reverse --oneline v0.1.0..v0.5.0`. Comparar etapas: `git diff v0.5.0-modulos..v0.5.0-integracion`.
