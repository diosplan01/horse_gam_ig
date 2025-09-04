#include <Servo.h>

/*

En esta programacion, hay que optimizar procesos sucios y que solo gastan espacio... como el exceso de llamadas al sistema, o la perdida de transferencia de datos
por mediocridad del codigo

Algo a mencionar es que el i2c no hace falta que funcione a 100khz... puede funcionar mas lento, ya que como tal... la informacion de los botones va directo al master
Luego la muestra de informacion viaja a travez de los i2c a cada lugar... si quieres puedes colocar que cada cierto tiempo se actualice la informacion... para asi no
sobrecargar todos los puertos, y siendo mas simple todo. Asi... no mezclando todo con el uart, sino que solo aplicando i2c

*/


// Configuración de pines (igual para ambos Megas)
const int LEDS_JUGADOR1[] = {22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42};
const int LEDS_JUGADOR2[] = {43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
const int SERVO_PIN1 = 2;
const int SERVO_PIN2 = 3;
const int NUM_LEDS = 21;

Servo servo1;
Servo servo2;

int progresoJ1 = 0;
int progresoJ2 = 0;

// Variables para verificación
unsigned long lastMessageTime = 0;
const unsigned long CONNECTION_TIMEOUT = 5000;

void setup() {
  Serial.begin(9600);
  
  // Inicializar LEDs - VERIFICAR ESTA PARTE
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LEDS_JUGADOR1[i], OUTPUT);
    pinMode(LEDS_JUGADOR2[i], OUTPUT);
    digitalWrite(LEDS_JUGADOR1[i], LOW);
    digitalWrite(LEDS_JUGADOR2[i], LOW);
  }
  
  // Test inicial de LEDs
  testLEDs();
  
  // Inicializar servos
  servo1.attach(SERVO_PIN1);
  servo2.attach(SERVO_PIN2);
  servo1.write(0);
  servo2.write(0);

  Serial.println("Mega inicializado - Esperando datos...");
}

void loop() {
  if (Serial.available()) {
    String mensaje = Serial.readStringUntil('\n');
    mensaje.trim(); // Eliminar espacios en blanco
    lastMessageTime = millis();
    
    Serial.print("Recibido: ");
    Serial.println(mensaje);
    
    procesarMensaje(mensaje);
  }
  
  // Verificar timeout de conexión
  if (millis() - lastMessageTime > CONNECTION_TIMEOUT && lastMessageTime > 0) {
    Serial.println("Timeout - No se reciben datos del master");
    lastMessageTime = millis(); // Reset para no spammear
  }
}

void testLEDs() {
  Serial.println("Testeando LEDs...");
  
  // Testear LEDs del jugador 1
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LEDS_JUGADOR1[i], HIGH);
    delay(50);
  }
  delay(500);
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LEDS_JUGADOR1[i], LOW);
  }
  
  // Testear LEDs del jugador 2
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LEDS_JUGADOR2[i], HIGH);
    delay(50);
  }
  delay(500);
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LEDS_JUGADOR2[i], LOW);
  }
  
  Serial.println("Test de LEDs completado");
}

void procesarMensaje(String mensaje) {
  // Comando RESET
  if (mensaje == "RESET") {
    Serial.println("Comando RESET recibido");
    reiniciarHardware();
    return;
  }
  
  // Formato: "COMANDO:VALOR:PJ"
  int separador1 = mensaje.indexOf(':');
  int separador2 = mensaje.indexOf(':', separador1 + 1);
  
  if (separador1 == -1 || separador2 == -1) {
    Serial.println("Formato de mensaje incorrecto");
    return;
  }
  
  String comando = mensaje.substring(0, separador1);
  String valorStr = mensaje.substring(separador1 + 1, separador2);
  String jugadorStr = mensaje.substring(separador2 + 1);
  
  // Verificar que el valor sea numérico
  for (unsigned int i = 0; i < valorStr.length(); i++) {
    if (!isDigit(valorStr.charAt(i))) {
      Serial.println("Valor no numérico");
      return;
    }
  }
  
  int valor = valorStr.toInt();
  int jugador = 0;
  
  // Verificar formato de jugador (P1, P2, etc.)
  if (jugadorStr.length() >= 2 && jugadorStr.charAt(0) == 'P') {
    jugador = jugadorStr.substring(1).toInt();
  } else {
    Serial.println("Formato de jugador incorrecto");
    return;
  }

  Serial.print("Procesando: ");
  Serial.print(comando);
  Serial.print(" Valor: ");
  Serial.print(valor);
  Serial.print(" Jugador: ");
  Serial.println(jugador);

  // Procesar comando
  if (comando == "BAR") {
    actualizarProgreso(jugador, valor);
  } else if (comando == "WIN") {
    celebrarVictoria(jugador);
  } else if (comando == "LOSE") {
    indicarDerrota(jugador);
  } else {
    Serial.println("Comando desconocido");
  }
}

void actualizarProgreso(int jugador, int valor) {
  if (jugador == 1 || jugador == 2) {
    Serial.print("Actualizando progreso jugador ");
    Serial.print(jugador);
    Serial.print(": ");
    Serial.println(valor);
    
    if (jugador == 1) {
      progresoJ1 = valor;
      int ledsEncendidos = map(valor, 0, 100, 0, NUM_LEDS);
      Serial.print("Encendiendo ");
      Serial.print(ledsEncendidos);
      Serial.println(" LEDs");
      
      for (int i = 0; i < NUM_LEDS; i++) {
        digitalWrite(LEDS_JUGADOR1[i], i < ledsEncendidos ? HIGH : LOW);
      }
      int angulo = map(valor, 0, 100, 0, 180);
      servo1.write(angulo);
    } else {
      progresoJ2 = valor;
      int ledsEncendidos = map(valor, 0, 100, 0, NUM_LEDS);
      Serial.print("Encendiendo ");
      Serial.print(ledsEncendidos);
      Serial.println(" LEDs");
      
      for (int i = 0; i < NUM_LEDS; i++) {
        digitalWrite(LEDS_JUGADOR2[i], i < ledsEncendidos ? HIGH : LOW);
      }
      int angulo = map(valor, 0, 100, 0, 180);
      servo2.write(angulo);
    }
  } else {
    Serial.print("Jugador no válido: ");
    Serial.println(jugador);
  }
}

void celebrarVictoria(int jugador) {
  Serial.print("Celebrando victoria jugador ");
  Serial.println(jugador);
  
  if (jugador == 1 || jugador == 2) {
    // Animación de victoria
    for (int i = 0; i < 5; i++) {
      if (jugador == 1) {
        for (int j = 0; j < NUM_LEDS; j++) digitalWrite(LEDS_JUGADOR1[j], HIGH);
        servo1.write(180);
      } else {
        for (int j = 0; j < NUM_LEDS; j++) digitalWrite(LEDS_JUGADOR2[j], HIGH);
        servo2.write(180);
      }
      delay(200);
      
      if (jugador == 1) {
        for (int j = 0; j < NUM_LEDS; j++) digitalWrite(LEDS_JUGADOR1[j], LOW);
        servo1.write(0);
      } else {
        for (int j = 0; j < NUM_LEDS; j++) digitalWrite(LEDS_JUGADOR2[j], LOW);
        servo2.write(0);
      }
      delay(200);
    }
    
    // Estado final
    if (jugador == 1) {
      for (int j = 0; j < NUM_LEDS; j++) digitalWrite(LEDS_JUGADOR1[j], HIGH);
      servo1.write(180);
    } else {
      for (int j = 0; j < NUM_LEDS; j++) digitalWrite(LEDS_JUGADOR2[j], HIGH);
      servo2.write(180);
    }
  }
}

void indicarDerrota(int jugador) {
  Serial.print("Indicando derrota jugador ");
  Serial.println(jugador);
  
  if (jugador == 1 || jugador == 2) {
    // Animación de derrota
    for (int i = 0; i < 3; i++) {
      if (jugador == 1) {
        for (int j = 0; j < 6; j++) digitalWrite(LEDS_JUGADOR1[j], HIGH);
      } else {
        for (int j = 0; j < 6; j++) digitalWrite(LEDS_JUGADOR2[j], HIGH);
      }
      delay(300);
      
      if (jugador == 1) {
        for (int j = 0; j < NUM_LEDS; j++) digitalWrite(LEDS_JUGADOR1[j], LOW);
      } else {
        for (int j = 0; j < NUM_LEDS; j++) digitalWrite(LEDS_JUGADOR2[j], LOW);
      }
      delay(300);
    }
  }
}

void reiniciarHardware() {
  Serial.println("Reiniciando hardware...");
  
  // Apagar todos los LEDs
  for (int i = 0; i < NUM_LEDS; i++) {
    digitalWrite(LEDS_JUGADOR1[i], LOW);
    digitalWrite(LEDS_JUGADOR2[i], LOW);
  }
  
  // Mover servos a posición neutral
  servo1.write(0);
  servo2.write(0);
  
  progresoJ1 = 0;
  progresoJ2 = 0;
  
  Serial.println("Hardware reiniciado");
}