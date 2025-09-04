#include <Wire.h>
#include <Servo.h>

/*
  CONTROL DE LUCES Y SERVOS (ESCLAVO I2C)
  -----------------------------------------
  Este código está diseñado para un Arduino Mega que actúa como un esclavo I2C.
  Controla el hardware pesado para DOS jugadores:
  - 2 Tiras de 21 LEDs cada una.
  - 2 Servomotores.

  ARQUITECTURA:
  - Hay 2 de estos Megas en el sistema.
  - Mega 1 (Dirección 0x10): Controla Jugador 1 y Jugador 2.
  - Mega 2 (Dirección 0x11): Controla Jugador 3 y Jugador 4.
  - Recibe comandos del maestro por I2C para actualizar los componentes.

  MEJORAS:
  - Convertido de UART a I2C para una comunicación más robusta.
  - Implementado control de servo suavizado para eliminar vibraciones ("danza del servo").
  - Código reestructurado y comentado.
*/

// --- CONFIGURACIÓN ---
// ¡¡¡IMPORTANTE!!! CAMBIAR ESTA DIRECCIÓN PARA CADA MEGA
// Usar 0x10 para el Mega de los jugadores 1 y 2.
// Usar 0x11 para el Mega de los jugadores 3 y 4.
const byte I2C_ADDRESS = 0x10;

// --- PINES ---
// LEDs y Servos para los dos jugadores que controla este Mega.
const int LEDS_JUGADOR1[] = {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42};
const int LEDS_JUGADOR2[] = {43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
const int SERVO_PIN1 = 2;
const int SERVO_PIN2 = 3;
const int NUM_LEDS = 21;

// --- OBJETOS Y VARIABLES GLOBALES ---
Servo servo1;
Servo servo2;

// --- Variables para control de servo suavizado ---
int anguloActualServo1 = 0, anguloObjetivoServo1 = 0;
int anguloActualServo2 = 0, anguloObjetivoServo2 = 0;
unsigned long tiempoUltimoMovimientoServo = 0;
const int INTERVALO_MOVIMIENTO_SERVO = 15; // ms por grado.

// --- Variables para comunicación I2C ---
char mensajeBuffer[32];
volatile bool nuevoMensaje = false;

// --- Variables de Estado ---
// Flags para saber si el juego está activo para cada jugador local.
// Esto evita que se procesen comandos 'BAR' en momentos incorrectos.
bool juegoActivo[2] = {false, false};


// ------------------- SETUP -------------------
void setup() {
  Serial.begin(9600);
  Serial.print("Iniciando Mega esclavo en direccion 0x");
  Serial.println(I2C_ADDRESS, HEX);

  // Iniciar I2C y registrar la función de recepción de datos.
  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(recibirDatosI2C);

  // Inicializar LEDs
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LEDS_JUGADOR1[i], OUTPUT);
    pinMode(LEDS_JUGADOR2[i], OUTPUT);
  }

  // Inicializar servos
  servo1.attach(SERVO_PIN1);
  servo2.attach(SERVO_PIN2);

  // Estado inicial del hardware
  reiniciarHardware();
}

// ------------------- LOOP PRINCIPAL -------------------
void loop() {
  // Procesar mensaje si hay uno nuevo.
  if (nuevoMensaje) {
    procesarMensaje(String(mensajeBuffer));
    nuevoMensaje = false;
  }

  // Actualizar continuamente los servos de forma no bloqueante.
  actualizarServos();
}

// ------------------- FUNCIONES I2C -------------------
/*
  recibirDatosI2C(byteCount)
  --------------------------
  Función de interrupción que se ejecuta cuando llegan datos por I2C.
  Lee los datos y activa un flag para que sean procesados en el loop principal.
*/
void recibirDatosI2C(int byteCount) {
  if (byteCount > sizeof(mensajeBuffer) - 1) {
    while(Wire.available()) Wire.read();
    return;
  }
  byte index = 0;
  while (Wire.available()) {
    mensajeBuffer[index++] = (char)Wire.read();
  }
  mensajeBuffer[index] = '\0';
  nuevoMensaje = true;
}

/*
  procesarMensaje(mensaje)
  ------------------------
  Parsea el mensaje recibido del maestro y llama a la función correspondiente.
  Formato esperado: "JUGADOR:COMANDO:VALOR" (ej: "1:BAR:150")
  O "JUGADOR:COMANDO" (ej: "2:WIN")
  O "RESET"
*/
void procesarMensaje(String mensaje) {
  Serial.print("Recibido: ");
  Serial.println(mensaje);

  // Comando global de RESET
  if (mensaje == "RESET") {
    reiniciarHardware();
    return;
  }

  // Parseo del mensaje
  int separador1 = mensaje.indexOf(':');
  if (separador1 == -1) return; // Formato incorrecto

  int jugador = mensaje.substring(0, separador1).toInt();
  String comandoConValor = mensaje.substring(separador1 + 1);

  String comando = comandoConValor;
  int valor = 0;

  int separador2 = comandoConValor.indexOf(':');
  if (separador2 != -1) {
    comando = comandoConValor.substring(0, separador2);
    valor = comandoConValor.substring(separador2 + 1).toInt();
  }

  // Llamar a la función correspondiente
  if (comando == "BAR") {
    // La llegada de un comando BAR implica que el juego ha (re)comenzado.
    if (jugador == 1 || jugador == 2) {
      juegoActivo[jugador - 1] = true;
    }
    actualizarProgreso(jugador, valor);
  } else if (comando == "WIN") {
    if (jugador == 1 || jugador == 2) {
      juegoActivo[jugador - 1] = false; // El juego termina para este jugador
    }
    celebrarVictoria(jugador);
  } else if (comando == "LOSE") {
    if (jugador == 1 || jugador == 2) {
      juegoActivo[jugador - 1] = false; // El juego termina para este jugador
    }
    indicarDerrota(jugador);
  }
}

// ------------------- FUNCIONES DE CONTROL DE HARDWARE -------------------

/*
  actualizarProgreso(jugador, valor)
  ----------------------------------
  Actualiza los LEDs y el ángulo objetivo del servo para el jugador especificado.
  Solo actúa si el flag juegoActivo para ese jugador es true.
*/
void actualizarProgreso(int jugador, int valor) {
  // Validar jugador y estado del juego
  if ((jugador != 1 && jugador != 2) || !juegoActivo[jugador - 1]) {
    return; // No hacer nada si el jugador no es válido o su juego no está activo
  }

  int progreso = constrain(valor, 0, 200);
  int ledsEncendidos = map(progreso, 0, 200, 0, NUM_LEDS);
  int angulo = map(progreso, 0, 200, 0, 180);

  if (jugador == 1) {
    anguloObjetivoServo1 = angulo;
    for (int i = 0; i < NUM_LEDS; i++) {
      digitalWrite(LEDS_JUGADOR1[i], i < ledsEncendidos);
    }
  } else if (jugador == 2) {
    anguloObjetivoServo2 = angulo;
    for (int i = 0; i < NUM_LEDS; i++) {
      digitalWrite(LEDS_JUGADOR2[i], i < ledsEncendidos);
    }
  }
}

/*
  actualizarServos()
  ------------------
  Mueve los servos gradualmente hacia su ángulo objetivo.
  Esta función no es bloqueante y se llama en cada ciclo del loop.
*/
void actualizarServos() {
  if (millis() - tiempoUltimoMovimientoServo > INTERVALO_MOVIMIENTO_SERVO) {
    // Servo 1
    if (anguloActualServo1 < anguloObjetivoServo1) {
      anguloActualServo1++;
      servo1.write(anguloActualServo1);
    } else if (anguloActualServo1 > anguloObjetivoServo1) {
      anguloActualServo1--;
      servo1.write(anguloActualServo1);
    }
    // Servo 2
    if (anguloActualServo2 < anguloObjetivoServo2) {
      anguloActualServo2++;
      servo2.write(anguloActualServo2);
    } else if (anguloActualServo2 > anguloObjetivoServo2) {
      anguloActualServo2--;
      servo2.write(anguloActualServo2);
    }
    tiempoUltimoMovimientoServo = millis();
  }
}

/*
  celebrarVictoria(jugador)
  -------------------------
  Realiza una animación de victoria para el jugador especificado.
  NOTA: Esta función usa delay() y es bloqueante.
*/
void celebrarVictoria(int jugador) {
  const int* leds = (jugador == 1) ? LEDS_JUGADOR1 : LEDS_JUGADOR2;
  Servo& servo = (jugador == 1) ? servo1 : servo2;

  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < NUM_LEDS; j++) digitalWrite(leds[j], HIGH);
    servo.write(180);
    delay(200);
    for (int j = 0; j < NUM_LEDS; j++) digitalWrite(leds[j], LOW);
    servo.write(0);
    delay(200);
  }
  // Dejarlo en estado de victoria
  for (int j = 0; j < NUM_LEDS; j++) digitalWrite(leds[j], HIGH);
  servo.write(180);
  if (jugador == 1) {
    anguloActualServo1 = anguloObjetivoServo1 = 180; // Sincronizar estado
  } else {
    anguloActualServo2 = anguloObjetivoServo2 = 180;
  }
}

/*
  indicarDerrota(jugador)
  -----------------------
  Realiza una animación de derrota.
  NOTA: Usa delay() y es bloqueante.
*/
void indicarDerrota(int jugador) {
  if (jugador != 1 && jugador != 2) return;

  const int* leds = (jugador == 1) ? LEDS_JUGADOR1 : LEDS_JUGADOR2;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 6; j++) digitalWrite(leds[j], HIGH);
    delay(300);
    for (int j = 0; j < NUM_LEDS; j++) digitalWrite(leds[j], LOW);
    delay(300);
  }
}

/*
  reiniciarHardware()
  -------------------
  Resetea todos los LEDs y servos a su estado inicial.
*/
void reiniciarHardware() {
  Serial.println("Reiniciando hardware...");

  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LEDS_JUGADOR1[i], LOW);
    digitalWrite(LEDS_JUGADOR2[i], LOW);
  }

  anguloObjetivoServo1 = 0;
  anguloObjetivoServo2 = 0;
  // Los servos se moverán suavemente a 0 en el loop.

  // Desactivar el juego para ambos jugadores locales.
  juegoActivo[0] = false;
  juegoActivo[1] = false;

  Serial.println("Hardware reiniciado.");
}
