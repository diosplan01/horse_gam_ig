# Proyecto de Juego de Carreras (Arquitectura de 6 Esclavos)

Este documento detalla la arquitectura, montaje y funcionamiento del proyecto refactorizado. El sistema ahora utiliza una robusta arquitectura de 6 esclavos I2C controlados por un maestro central.

## 1. Arquitectura del Sistema

El sistema se compone de 7 Arduinos en total:

-   **1 x Arduino Maestro (`master_juego.ino`)**: Es el cerebro del juego. Gestiona la lógica, lee los botones y envía comandos a todos los esclavos.

-   **4 x Controles de Jugador (`controles.ino`)**: Son Arduinos pequeños (como Nano o Uno) que sirven de interfaz para cada jugador.
    -   **Direcciones I2C**: `0x08`, `0x09`, `0x0A`, `0x0B`.
    -   **Función**: Controlan una pantalla LCD y LEDs de estado para un solo jugador.

-   **2 x Megas de Hardware (`control_luces_caballos.ino`)**: Son Arduinos Mega que manejan el hardware más demandante.
    -   **Direcciones I2C**: `0x10`, `0x11`.
    -   **Función**:
        -   El Mega en `0x10` controla los 21 LEDs y el servo del Jugador 1, y los 21 LEDs y el servo del Jugador 2.
        -   El Mega en `0x11` controla los 21 LEDs y el servo del Jugador 3, y los 21 LEDs y el servo del Jugador 4.

Esta arquitectura distribuye la carga de trabajo, permitiendo que el maestro se enfoque en la lógica y los esclavos en controlar el hardware específico.

---

## 2. Montaje y Cableado

### Bus I2C

-   Conecta los pines `SDA` y `SCL` de los 7 Arduinos (1 Maestro + 6 Esclavos) en paralelo.
-   **IMPORTANTE**: Coloca una resistencia de pull-up de 4.7kΩ en la línea SDA y otra en la línea SCL. Conéctalas desde la línea respectiva (SDA o SCL) a 5V. Esto es esencial para la estabilidad del bus I2C con tantos dispositivos.

### Maestro (`master_juego.ino`)

-   **Botones**: Conecta los botones de los jugadores a los pines `2, 3, 4, 5`. Usa resistencias de 10kΩ en modo pull-down.

### Controles de Jugador (`controles.ino`) - Repetir 4 veces

-   **Sketch**: Sube `controles.ino` a cada uno de los 4 Arduinos.
-   **Configuración**: **En cada uno**, modifica la línea `const byte I2C_ADDRESS` para que sea única: `0x08` para J1, `0x09` para J2, `0x0A` para J3, y `0x0B` para J4.
-   **Conexiones por control**:
    -   LCD: A los pines `11, 12, 2, 3, 4, 5`.
    -   LED Verde: Al pin `6`.
    -   LED Rojo: Al pin `7`.

### Megas de Hardware (`control_luces_caballos.ino`) - Repetir 2 veces

-   **Sketch**: Sube `control_luces_caballos.ino` a cada uno de los 2 Arduinos Mega.
-   **Configuración**:
    -   Para el Mega que controlará a los jugadores 1 y 2, establece `const byte I2C_ADDRESS = 0x10;`.
    -   Para el Mega que controlará a los jugadores 3 y 4, establece `const byte I2C_ADDRESS = 0x11;`.
-   **Conexiones por Mega**:
    -   **Jugador "1" del Mega (J1 o J3 del juego)**:
        -   21 LEDs: A los pines `22` al `42`.
        -   Servo: Al pin `2`.
    -   **Jugador "2" del Mega (J2 o J4 del juego)**:
        -   21 LEDs: A los pines `43-53` y `4-13`.
        -   Servo: Al pin `3`.

---

## 3. Flujo de Funcionamiento

1.  **Arranque**: Al encender, el maestro testea la conexión con los 6 esclavos y reporta su estado en el Monitor Serial. Luego, envía un comando `WAIT` a los 4 controles, que mostrarán "Esperando...".
2.  **Inicio**: Tras una pausa, el maestro envía `START` a los controles. El juego comienza.
3.  **Juego**:
    -   Cuando un jugador pulsa su botón, el maestro actualiza su progreso.
    -   El maestro envía dos comandos I2C:
        1.  Un comando `NUM:[progreso]` al **control** del jugador para actualizar el LCD.
        2.  Un comando `JUGADOR:BAR:[progreso]` al **Mega** correspondiente para actualizar la barra de LEDs y el servo.
    -   El maestro comprueba constantemente el ranking y envía `POS:[posicion]` a los controles si hay cambios.
4.  **Fin de Partida**:
    -   Al detectar un ganador, el maestro envía `WIN` o `LOSE` a todos los dispositivos relevantes (tanto controles como Megas).
    -   Tras 10 segundos, el maestro envía un comando `RESET` global a todos los esclavos para prepararlos para la siguiente partida.

---

## 4. Mejoras Clave Implementadas

-   **Arquitectura Correcta**: El sistema ahora respeta la arquitectura de hardware original con 6 esclavos, distribuyendo la carga de trabajo de manera eficiente.
-   **Conversión a I2C**: Se eliminó completamente la comunicación UART, que era menos fiable y más compleja de cablear.
-   **Servo Suavizado**: El "baile" del servo se ha solucionado implementando un algoritmo de movimiento no bloqueante en los Megas.
-   **Código Documentado**: Todos los archivos `.ino` han sido comentados extensamente para explicar su lógica. Este README sirve como guía central del proyecto.
-   **Diagnóstico Mejorado**: El chequeo de salud en el arranque ahora verifica los 6 esclavos, facilitando la detección de errores de conexión.
