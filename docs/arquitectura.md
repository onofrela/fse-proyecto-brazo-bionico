# Arquitectura y flujo de extremo a extremo

La plataforma sigue siendo Arduino UNO R3 a 16 MHz, sin SO. Cada bloque del sistema tiene una conexión en este diagrama. Las conexiones físicas se describen en el README; el diagrama representa el montaje previsto y el firmware implementado, no una certificación de ensamblaje.

```mermaid
flowchart TD
    BAT[LiPo 3S 1000 mAh] --> SW[Interruptor principal]
    SW --> BUCK[Buck ajustado a 5 V]
    BUCK --> MCU[Arduino UNO: alimentación regulada]
    BUCK --> POWER[Alimentación externa de cinco servos]
    MCU --> REG[Salida 3.3 V del UNO]
    REG --> SENSOR[AD8232: 3.3 V]
    ELECT[Electrodos RA / LA / RL] --> SENSOR
    SENSOR -->|OUTPUT A0 y LO+/LO- A1/A2| ADC[Acquisition: Timer2 + ADC ISR]
    ADC -->|muestra cruda + contacto + tiempo| QUEUE[SampleQueue: 32 posiciones, 31 útiles]
    QUEUE -->|copia atómica| DC[Signal: centrado DC y salud]
    DC --> FILTER[SignalFilter: HP30 + notch60 + LP200]
    FILTER --> ENV[Signal: rectificación + media de 32 muestras]
    ENV -->|envolvente y salud| APP[Application: coordinación y seguridad]
    BUTTON[Botón D8 / selector D9] --> INPUT[Outputs: GPIO + Inputs: antirrebote]
    INPUT --> APP
    APP --> CAL[Calibration: P95 de reposo y P70 de actividad]
    CAL -->|umbrales| APP
    APP -->|guardar| STORE[Storage: EEPROM verificada]
    STORE -->|recuperar al encender| APP
    APP --> DEC[Decision: ponderación y retención de cierre]
    DEC --> MOTION[Motion: rampa de cinco dedos]
    MOTION --> OUT[Outputs: Servo / Timer1]
    OUT -->|D3 D5 D6 D10 D11| SERVOS[Cinco servomotores]
    POWER --> SERVOS
    SERVOS --> TENDON[Tensores]
    TENDON --> HAND[Dedos InMoov]
    APP --> RGB[Outputs: RGB D7 D12 D13]
    APP --> DIAG[Diagnostics: UART 115200]
    DIAG -->|registro opcional| PC[Analizador: CSV y gráficas]
    PC -->|c / p / x / consulta| DIAG
    DIAG -->|comando por vuelta| APP
    ADC --> PROBE[D2: un flanco por conversión]
```

Todos los circuitos de alimentación y señal comparten GND según el cableado. El AD8232 recibe 3.3 V; la alimentación de servos debe soportar sus corrientes sin pasar por USB ni el regulador del UNO. No se debe alimentar el AD8232 a 5 V.

## Propiedad de datos y sincronización

`Application` posee los estados de modo, calibración, filtro, rampa y antirrebote. Solo `loop()` los modifica. La ISR ADC posee el extremo productor de la cola; el bucle posee el consumidor. Timer2 y ADC modifican la alarma y el contador de pérdida. No hay asignaciones dinámicas en el firmware del proyecto.

La publicación de una muestra ocurre después de copiar sus campos. `Acquisition::take()` copia la muestra y avanza el consumidor dentro de `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)`; lo mismo se hace con la lectura/limpieza de alarmas y el contador de 16 bits. Así se evita leer una marca de tiempo o dato de varios bytes a mitad de actualización en el AVR de 8 bits. Las pruebas FIFO verifican orden y límites; la compilación AVR incluye las secciones críticas reales. Falta medir su duración y el jitter en placa.

La cola llena rechaza la muestra nueva y registra una alarma. Al recibirla, `Application` descarta pendientes, invalida el filtro y ordena apertura en EMG. Una muestra vieja (>10 ms) tampoco puede renovar la salud. La adquisición se recupera con 2000 muestras sanas y reposo antes del cierre; una calibración interrumpida debe repetirse. La apertura es una rampa, no un corte de potencia.

## Secuencia de arranque

1. Arduino llama a `setup()` tras alimentación/reset y ejecución del bootloader.
2. Se configuran GPIO y servos abiertos; se valida `attach` y se deshace un arranque parcial si falla.
3. Se recuperan umbrales y se escribe/verifica el contador de arranque antes de habilitar ADC.
4. Se configura ADC con espera acotada y Timer2 con comprobación de registros.
5. Se entra al bucle sin esperar USB. Con calibración válida opera tras estabilización y reposo. Sin ella permanece abierto hasta una solicitud explícita; D8 permite calibrar incluso con UART deshabilitada.

Un fallo al iniciar ADC mantiene EMG inhibido hasta reiniciar; manual sigue disponible si los servos iniciaron. Un fallo de servos inhibe sus órdenes y se señala en RGB/UART. Una falla persistente de hardware no puede repararse por software.

## Contrato del filtro y de los dos relojes

SignalFilter tiene estado propio y operaciones `addQ6`/`clear`; no conoce GPIO, servos, EEPROM ni UART. Signal lo reinicia tras pérdida de salud. Sus coeficientes están diseñados para 1000 Hz; la compilación impide cambiar esa frecuencia sin revisar el filtro. El consumidor procesa todas las muestras aceptadas, manteniendo la ISR breve.

Calibration y Decision avanzan con el tiempo de adquisición. Motion avanza con el reloj del bucle y nunca recupera pasos en ráfaga. IPC-TIME verifica ponderación idéntica con una muestra o lotes de cinco. La retención normal no bloquea la vía de falla que solicita apertura. No se añadieron estados por dedo ni se modificó la persistencia incremental.
