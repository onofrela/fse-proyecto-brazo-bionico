# Inestabilidad del módulo del sensor: diagnóstico pendiente

## Problema actual

El módulo AD8232 presenta una señal muy variable y el comportamiento del control todavía no resulta suficientemente repetible. La causa de esta inestabilidad **no se ha identificado**. Se está investigando el módulo, sus conexiones y las condiciones de adquisición; no se ha confirmado un componente averiado ni se considera resuelto el problema por aplicar filtros o cambiar los umbrales.

El operador reporta esta volatilidad durante las pruebas de la mano. Los CSV y capturas existentes muestran variaciones de envolvente y, en algunas tramas, umbrales 0,0. Parte de esos registros se obtuvo moviendo deliberadamente dedos y mano, por lo que sus cambios no se pueden atribuir automáticamente a una falla. Los umbrales nulos también obligan a comprobar la versión cargada y la calibración antes de explicar un movimiento únicamente por el hardware.

Las pruebas automatizadas verifican la lógica del programa con entradas controladas; no comprueban la calidad de una soldadura, un jumper ni el estado físico del AD8232. La validación del montaje sigue pendiente.

## Hipótesis de trabajo

Todas las hipótesis siguientes están **pendientes de comprobar**. Pueden coexistir varias causas.

| Hipótesis | Qué revisar | Cómo obtener evidencia |
| --- | --- | --- |
| Soldadura deficiente o unión intermitente | Pines del módulo, conectores, pads y uniones del montaje | Con el circuito apagado, inspección visual ampliada y comprobación de continuidad; documentar la unión antes y después de cualquier reparación |
| Módulo deteriorado o defectuoso | Estado físico y respuesta del AD8232 | Comparar con un módulo de funcionamiento conocido, manteniendo alimentación, conexiones, colocación y protocolo equivalentes; registrar modelo y revisión |
| Jumpers dañados o con mal contacto | Conductores, terminales y ajuste de los conectores | Comprobar continuidad sin alimentación y sustituir un cable a la vez por otro verificado; repetir la misma captura |
| Contactos de protoboard o conectores flojos | Firmeza y continuidad de cada unión | Probar conexiones verificadas sin cambiar simultáneamente otros elementos; conservar fotografías del montaje |
| Alimentación o referencia de tierra inestables | Alimentación del sensor, GND común, referencia ADC y relación con la fuente de servos | Registrar las condiciones eléctricas y comparar adquisición con los servos sin accionar y en las condiciones normales del ensayo; comprobar el montaje según el README |
| Contacto o movimiento de electrodos/cables | Colocación, adhesión y movimiento del cableado | Repetir reposo y contracción con colocación documentada y cables sujetos; separar los movimientos intencionales de los artefactos del montaje |
| Interferencia ambiental o acoplamiento eléctrico | Proximidad y recorrido del cableado respecto de otras fuentes de ruido | Comparar condiciones controladas cambiando una sola variable, sin asumir que todo pico es una contracción o un latido |
| Banda analógica del módulo inadecuada para la actividad buscada | Esquema, modelo y filtrado de la tarjeta AD8232 | Identificar la tarjeta y su acondicionamiento; el procesamiento digital no puede recuperar información eliminada por el circuito analógico |
| Configuración o calibración no coincidentes con el programa usado | Firmware informado por FW, modo, umbrales y referencia ADC | Guardar los eventos de arranque/calibración y comprobar umbrales válidos, bits de estado y contadores antes de interpretar el control |

No se concluye que haya una mala soldadura, un módulo inutilizable o un jumper averiado sólo por observar una señal variable. Cada hipótesis requiere una comparación repetible que la apoye o la descarte.

## Secuencia de investigación

1. Identificar y fotografiar módulo, conexiones y soldaduras. Registrar versión de firmware, configuración, alimentación, electrodos y elementos utilizados.
2. Con el montaje apagado, revisar visualmente y comprobar la continuidad de soldaduras, jumpers y conectores. Aplicar las condiciones de montaje y aislamiento del README antes de conectar instrumentos, especialmente si se utilizan electrodos sobre el cuerpo.
3. Verificar cables y contactos uno por uno. Repetir el ensayo tras cada cambio, conservando el resto de las condiciones.
4. Registrar varias repeticiones de reposo y contracción de toda la mano, con etiquetas REST/HAND/END y sin mover deliberadamente el brazo o los cables durante el reposo. Conservar los archivos completos, incluidos eventos de calibración y fallas.
5. Comparar la adquisición con los servos sin accionar y después bajo las condiciones normales previstas. Documentar cualquier relación entre la señal y el funcionamiento de los actuadores, sin atribuirla a la alimentación hasta medirla.
6. Si persiste la inestabilidad, comparar con un módulo verificado y revisar el acondicionamiento analógico. Evitar sustituir simultáneamente módulo, cables y colocación, porque impediría distinguir qué cambio tuvo efecto.
7. Repetir las condiciones iniciales cuando sea posible para comprobar si el resultado es reproducible. Sólo después actualizar el diagnóstico y la decisión de reparar o sustituir un elemento.

Una prueba favorable aislada no confirma la reparación. Que los bits de contacto o hardware sean cero tampoco certifica que todas las conexiones sean correctas.

## Registro de resultados

Para cada ensayo conservar: identificador, módulo/revisión, firmware, elemento cambiado, estado de soldaduras y cables, alimentación y referencia medidas cuando corresponda, colocación de electrodos, etiquetas de actividad, CSV, eventos, gráfica y fotografías. Registrar también frecuencia de telemetría, bits de falla, pérdidas/descartes, resultado de calibración y comparación de reposo/actividad.

Las instantáneas a 10 Hz permiten comparar niveles y estados, pero no demostrar por sí solas el origen espectral del ruido ni la eliminación de interferencia de red o ECG. Se deben distinguir las mediciones nuevas de los replays y de las pruebas sintéticas.

| Elemento | Estado actual |
| --- | --- |
| Causa principal de la volatilidad | Desconocida |
| Soldaduras, jumpers y contactos | Pendientes de comprobación documentada |
| Estado del módulo | No se ha confirmado si funciona correctamente o está deteriorado |
| Alimentación, electrodos e interferencias | Hipótesis por evaluar |
| Solución física validada | Pendiente |

Este documento registra el problema y el plan de investigación. No documenta reparaciones, sustituciones ni mediciones eléctricas que todavía no se hayan realizado.
