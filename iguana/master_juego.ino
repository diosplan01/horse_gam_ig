/*
  MASTER JUEGO (ARQUITECTURA 6 ESCLAVOS)
  ---------------------------------------
  Este es el Arduino maestro que controla la lógica del juego.
  Se comunica con 6 esclavos I2C:
  - 4 Controles de Jugador (LCDs y LEDs de estado).
  - 2 Megas (barras de 21 LEDs y servos).

  MEJORAS:
  - Comunicación 100% I2C, eliminando UART.
  - Lógica de comunicación adaptada a la arquitectura de 6 esclavos.
  - Chequeo de salud en el arranque para los 6 esclavos.
  - Optimización de envío de datos de posición (POS).
  - Código reestructurado y comentado extensamente.
*/

#include <Wire.h>

// --- CONSTANTES Y PINES ---
#define MAX_STEPS 200
const int J1_BUTTON = 2, J2_BUTTON = 3, J3_BUTTON = 4, J4_BUTTON = 5;

// --- DIRECCIONES I2C ---
// 4 Esclavos para los controles de los jugadores (LCDs)
const int CONTROL_1_ADDR = 0x08, CONTROL_2_ADDR = 0x09, CONTROL_3_ADDR = 0x0A, CONTROL_4_ADDR = 0x0B;
// 2 Esclavos para el hardware pesado (LEDs y Servos)
const int MEGA_1_ADDR = 0x10; // Para jugadores 1 y 2
const int MEGA_2_ADDR = 0x11; // Para jugadores 3 y 4

/*
  CLASE JUGADOR
  -------------
  Almacena el estado de cada jugador y sus direcciones I2C asociadas.
*/
class Jugador {
public:
  uint8_t pinBoton;
  uint8_t direccionControl; // Dirección del control con el LCD
  uint8_t direccionMega;    // Dirección del Mega con los LEDs/Servo
  uint8_t idEnMega;         // Identificador en el Mega (1 o 2)
  int progreso;
  bool haTerminado;
  bool juegoIniciado;

  Jugador(uint8_t btn, uint8_t ctrlAddr, uint8_t megaAddr, uint8_t megaId) {
    pinBoton = btn;
    direccionControl = ctrlAddr;
    direccionMega = megaAddr;
    idEnMega = megaId;
    progreso = 0;
    haTerminado = false;
    juegoIniciado = false;
    pinMode(pinBoton, INPUT);
  }

  /*
    revisarBoton()
    --------------
    Verifica si se ha presionado el botón. Si es así, incrementa el progreso
    y envía los datos a los esclavos correspondientes por I2C.
  */
  void revisarBoton() {
    if (!juegoIniciado || haTerminado) return;

    if (digitalRead(pinBoton) == HIGH) {
      delay(50); // Debounce
      if (digitalRead(pinBoton) == HIGH) {
        progreso++;

        // 1. Enviar progreso al control del jugador (para el LCD)
        enviarTextoI2C(direccionControl, "NUM:" + String(progreso));

        // 2. Enviar progreso al Mega correspondiente (para LEDs y Servo)
        String comandoMega = String(idEnMega) + ":BAR:" + String(progreso);
        enviarTextoI2C(direccionMega, comandoMega);

        while (digitalRead(pinBoton) == HIGH);
      }
    }
    if (progreso >= MAX_STEPS) {
      haTerminado = true;
    }
  }

  /*
    enviarTextoI2C(direccion, texto)
    ---------------------------------
    Función genérica para enviar un string a una dirección I2C.
  */
  void enviarTextoI2C(uint8_t direccion, String texto) {
    Wire.beginTransmission(direccion);
    Wire.print(texto);
    Wire.endTransmission();
  }
};

// --- OBJETOS GLOBALES ---
Jugador jugador1(J1_BUTTON, CONTROL_1_ADDR, MEGA_1_ADDR, 1);
Jugador jugador2(J2_BUTTON, CONTROL_2_ADDR, MEGA_1_ADDR, 2);
Jugador jugador3(J3_BUTTON, CONTROL_3_ADDR, MEGA_2_ADDR, 1);
Jugador jugador4(J4_BUTTON, CONTROL_4_ADDR, MEGA_2_ADDR, 2);

Jugador* jugadores[] = { &jugador1, &jugador2, &jugador3, &jugador4 };
const int numJugadores = 4;

// --- VARIABLES DE ESTADO ---
bool yaHayGanador = false;
int ultimasPosiciones[numJugadores] = {0, 0, 0, 0};

// ------------------- SETUP -------------------
void setup() {
  Wire.begin();
  Serial.begin(9600);
  Serial.println("--- Master (6 Esclavos) Iniciado ---");

  // Chequeo de salud para los 6 esclavos
  chequearEsclavos();

  // Preparar y empezar el juego
  reiniciarJuego(false);

  Serial.println("Enviando comando WAIT a todos los esclavos...");
  for (int i = 0; i < numJugadores; i++) {
    jugadores[i]->enviarTextoI2C(jugadores[i]->direccionControl, "WAIT");
  }

  Serial.println("El juego comenzara en 3 segundos...");
  delay(3000);

  Serial.println("¡JUEGO INICIADO!");
  for (int i = 0; i < numJugadores; i++) {
    jugadores[i]->enviarTextoI2C(jugadores[i]->direccionControl, "START");
    jugadores[i]->juegoIniciado = true;
  }
}

// ------------------- LOOP PRINCIPAL -------------------
void loop() {
  for (int i = 0; i < numJugadores; i++) {
    jugadores[i]->revisarBoton();
  }

  if (!yaHayGanador) {
    actualizarPosiciones();
    verificarGanador();
  }

  delay(10);
}

// ------------------- FUNCIONES AUXILIARES -------------------

/*
  chequearEsclavos()
  ------------------
  Verifica la conexión con los 4 controles y los 2 Megas.
*/
void chequearEsclavos() {
  Serial.println("Chequeando Controles (LCDs)...");
  for (int i = 0; i < numJugadores; i++) {
    chequearDispositivo(jugadores[i]->direccionControl, "Control " + String(i + 1));
  }
  Serial.println("Chequeando Megas (LEDs/Servos)...");
  chequearDispositivo(MEGA_1_ADDR, "Mega 1 (J1/J2)");
  chequearDispositivo(MEGA_2_ADDR, "Mega 2 (J3/J4)");
}

void chequearDispositivo(uint8_t direccion, String nombre) {
  Wire.beginTransmission(direccion);
  byte error = Wire.endTransmission();
  Serial.print(nombre + " (0x" + String(direccion, HEX) + "): ");
  if (error == 0) {
    Serial.println("Conectado.");
  } else {
    Serial.println("No encontrado (Error: " + String(error) + ")");
  }
}

/*
  actualizarPosiciones()
  ----------------------
  Calcula y envía el ranking de los jugadores a sus controles.
  Optimizado para enviar solo si la posición cambia.
*/
void actualizarPosiciones() {
  int indices[numJugadores];
  for(int i=0; i<numJugadores; i++) indices[i] = i;

  for(int i=0; i<numJugadores-1; i++){
    for(int j=i+1; j<numJugadores; j++){
      if(jugadores[indices[j]]->progreso > jugadores[indices[i]]->progreso){
        int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
      }
    }
  }

  for (int i = 0; i < numJugadores; i++) {
    int jugadorIdx = indices[i];
    int nuevaPosicion = i + 1;
    if (ultimasPosiciones[jugadorIdx] != nuevaPosicion) {
      jugadores[jugadorIdx]->enviarTextoI2C(jugadores[jugadorIdx]->direccionControl, "POS:" + String(nuevaPosicion));
      ultimasPosiciones[jugadorIdx] = nuevaPosicion;
    }
  }
}

/*
  verificarGanador()
  ------------------
  Comprueba si hay un ganador y gestiona el fin de la partida.
*/
void verificarGanador() {
  int ganadorIdx = -1;
  for (int i = 0; i < numJugadores; i++) {
    if (jugadores[i]->progreso >= MAX_STEPS) {
      ganadorIdx = i;
      break;
    }
  }

  if (ganadorIdx != -1) {
    yaHayGanador = true;

    // Enviar WIN/LOSE a todos los esclavos (controles y Megas)
    for (int i = 0; i < numJugadores; i++) {
      String msg = (i == ganadorIdx) ? "WIN" : "LOSE";
      jugadores[i]->enviarTextoI2C(jugadores[i]->direccionControl, msg);
      jugadores[i]->enviarTextoI2C(jugadores[i]->direccionMega, String(jugadores[i]->idEnMega) + ":" + msg);
    }

    Serial.println("Juego terminado. Reiniciando en 10 segundos...");
    delay(10000);
    reiniciarJuego(true);
  }
}

/*
  reiniciarJuego(anunciar)
  ------------------------
  Resetea el estado del juego para una nueva partida.
*/
void reiniciarJuego(bool anunciar) {
  yaHayGanador = false;
  for (int i = 0; i < numJugadores; i++) {
    jugadores[i]->progreso = 0;
    jugadores[i]->haTerminado = false;
    ultimasPosiciones[i] = 0;

    if (anunciar) {
      jugadores[i]->enviarTextoI2C(jugadores[i]->direccionControl, "RESET");
      jugadores[i]->juegoIniciado = true;
    } else {
      jugadores[i]->juegoIniciado = false;
    }
  }

  if (anunciar) {
    // Enviar RESET a los Megas
    jugador1.enviarTextoI2C(MEGA_1_ADDR, "RESET");
    jugador3.enviarTextoI2C(MEGA_2_ADDR, "RESET");
    Serial.println("Juego reiniciado.");
  }
}
