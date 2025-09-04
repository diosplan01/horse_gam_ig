#include <Wire.h>
#include <LiquidCrystal.h>

/*
  CONTROLADOR DE JUGADOR (ESCLAVO I2C)
  ------------------------------------
  Este código es para los Arduinos (posiblemente Nanos o Unos) que
  actúan como la "consola" de cada jugador.

  COMPONENTES CONTROLADOS:
  - Una pantalla LCD 16x2.
  - Dos LEDs de estado para indicar la posición en la carrera.

  ARQUITECTURA:
  - Hay 4 de estos controladores en el sistema, cada uno con una
    dirección I2C única (0x08, 0x09, 0x0A, 0x0B).
  - Recibe comandos del maestro para actualizar la pantalla y los LEDs.
*/

// --- PINES Y CONFIGURACIÓN ---
// Pines para la pantalla LCD
const int RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5;
// Pines para los LEDs de estado
#define LED_VERDE 6
#define LED_ROJO 7
// ¡¡¡IMPORTANTE!!! CAMBIAR ESTA DIRECCIÓN PARA CADA CONTROLADOR
const byte I2C_ADDRESS = 0x08; // 0x08, 0x09, 0x0A, 0x0B

// --- OBJETOS Y VARIABLES GLOBALES ---
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// Variables de estado del juego
int progreso = 0;
int posicion = 4; // Empezar en 4to lugar por defecto
bool gano = false;
bool perdio = false;
bool juegoActivo = false;

// Variables para el parpadeo de LEDs
unsigned long tiempoUltimoParpadeo = 0;
bool estadoLedParpadeo = false;

// Variables para comunicación I2C
char mensajeBuffer[32];
volatile bool nuevoMensaje = false;


// ------------------- SETUP -------------------
void setup() {
  // Configurar pines de LEDs
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);

  // Inicializar LCD
  lcd.begin(16, 2);

  // Inicializar I2C como esclavo
  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(recibirDatosI2C);

  // Iniciar Serial para depuración
  Serial.begin(9600);
  Serial.print("Control inicializado - Direccion: 0x");
  Serial.println(I2C_ADDRESS, HEX);

  // Estado inicial de los componentes
  reiniciarEstado();
}

// ------------------- LOOP PRINCIPAL -------------------
void loop() {
  // Procesar mensaje si hay uno nuevo
  if (nuevoMensaje) {
    procesarMensaje(String(mensajeBuffer));
    nuevoMensaje = false;
  }

  // Controlar los LEDs de posición si el juego está activo
  if (juegoActivo) {
    controlarLuces();
  }

  delay(10);
}

// ------------------- FUNCIONES I2C -------------------
/*
  recibirDatosI2C(byteCount)
  --------------------------
  Función de interrupción que se ejecuta al recibir datos por I2C.
  Lee los datos y activa un flag para que sean procesados en el loop.
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
  Interpreta el comando recibido del maestro y actualiza el estado.
*/
void procesarMensaje(String mensaje) {
  Serial.print("Mensaje recibido: ");
  Serial.println(mensaje);

  if (mensaje.startsWith("NUM:")) {
    progreso = mensaje.substring(4).toInt();
    if (juegoActivo) {
      mostrarEnLCD();
    }
  }
  else if (mensaje.startsWith("POS:")) {
    posicion = mensaje.substring(4).toInt();
  }
  else if (mensaje == "WIN") {
    juegoActivo = false;
    gano = true;
    mostrarEnLCD();
  }
  else if (mensaje == "LOSE") {
    juegoActivo = false;
    perdio = true;
    mostrarEnLCD();
  }
  else if (mensaje == "RESET") {
    reiniciarEstado();
  }
  else if (mensaje == "WAIT") {
    juegoActivo = false;
    lcd.clear();
    lcd.print("Esperando...");
  }
  else if (mensaje == "START") {
    juegoActivo = true;
    mostrarEnLCD();
  }
}

// ------------------- CONTROL DE COMPONENTES -------------------
/*
  controlarLuces()
  ----------------
  Controla los LEDs de estado (verde/rojo) según la posición
  del jugador en la carrera.
*/
void controlarLuces() {
  unsigned long ahora = millis();

  switch (posicion) {
    case 1: // Primer lugar: verde fijo
      digitalWrite(LED_VERDE, HIGH);
      digitalWrite(LED_ROJO, LOW);
      break;

    case 2: // Segundo lugar: verde parpadeante
      if (ahora - tiempoUltimoParpadeo > 500) {
        estadoLedParpadeo = !estadoLedParpadeo;
        digitalWrite(LED_VERDE, estadoLedParpadeo);
        digitalWrite(LED_ROJO, LOW);
        tiempoUltimoParpadeo = ahora;
      }
      break;

    case 3: // Tercer lugar: rojo parpadeante
      if (ahora - tiempoUltimoParpadeo > 500) {
        estadoLedParpadeo = !estadoLedParpadeo;
        digitalWrite(LED_ROJO, estadoLedParpadeo);
        digitalWrite(LED_VERDE, LOW);
        tiempoUltimoParpadeo = ahora;
      }
      break;

    default: // Último lugar (o sin posición): rojo fijo
      digitalWrite(LED_VERDE, LOW);
      digitalWrite(LED_ROJO, HIGH);
      break;
  }
}

/*
  mostrarEnLCD()
  --------------
  Refresca la pantalla LCD con la información adecuada.
*/
void mostrarEnLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);

  if (gano) {
    lcd.print("¡GANASTE!");
  }
  else if (perdio) {
    lcd.print("PERDISTE");
  }
  else {
    lcd.print("Progreso: ");
    lcd.print(progreso);
  }

  // Dibuja la barra de progreso en la segunda línea del LCD
  lcd.setCursor(0, 1);
  int barras = map(constrain(progreso, 0, 200), 0, 200, 0, 16);
  for (int i = 0; i < barras; i++) {
    lcd.write(byte(255)); // Carácter de bloque lleno
  }
}

/*
  reiniciarEstado()
  -----------------
  Resetea todas las variables y componentes a su estado inicial.
*/
void reiniciarEstado() {
  progreso = 0;
  posicion = 4;
  gano = false;
  perdio = false;
  juegoActivo = false;

  // Apagar LEDs (o dejarlos en estado por defecto)
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, HIGH);

  // Mensaje de bienvenida
  lcd.clear();
  lcd.print("Control ");
  lcd.print(I2C_ADDRESS, HEX);
  lcd.setCursor(0, 1);
  lcd.print("Listo");
}
