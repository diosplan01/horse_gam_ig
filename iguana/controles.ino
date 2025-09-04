#include <Wire.h>
#include <LiquidCrystal.h>
 
/*

Mejorar la robustez del codigo y añadir en paralelo maneras en las cuales se comunique mejor con el master
Por lo tanto En lo posible añadir solo el esperando cuando el master dicte que tiene que esperar o cuando aun no 
da instrucciones de empezar, asi que añade ese apartado aqui y en el master

*/
// Pines LCD
const int RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

#define LED_VERDE 6
#define LED_ROJO 7

// Variables del juego
int progreso = 0;
int posicion = 0;
bool gano = false;
bool perdio = false;

// Variables para el control de parpadeo
unsigned long lastToggle = 0;
bool ledState = false;

// Buffer para recibir datos I2C
char mensajeBuffer[32];
byte bufferIndex = 0;
bool nuevoMensaje = false;

// Dirección I2C (CAMBIAR SEGÚN CADA CONTROL)
const byte I2C_ADDRESS = 0x08; // 0x08, 0x09, 0x0A, 0x0B

void setup() {
  // Configurar pines de LEDs
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  
  // Inicializar LCD
  lcd.begin(16, 2);
  
  // Inicializar I2C como slave
  Wire.begin(I2C_ADDRESS);
  Wire.onReceive(recibirDatos);
  
  // Estado inicial
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, HIGH);
  
  // Mostrar mensaje inicial
  lcd.clear();
  lcd.print("Control ");
  lcd.print(I2C_ADDRESS, HEX);
  lcd.setCursor(0, 1);
  lcd.print("Listo");
  
  Serial.begin(9600);
  Serial.print("Control inicializado - Direccion: 0x");
  Serial.println(I2C_ADDRESS, HEX);
}

void loop() {
  // Procesar mensaje si hay uno nuevo
  if (nuevoMensaje) {
    String mensaje = String(mensajeBuffer);
    nuevoMensaje = false;
    bufferIndex = 0;
    
    procesarMensaje(mensaje);
  }
  
  // Control de luces según posición
  controlarLuces();
  
  delay(10);
}

void recibirDatos(int byteCount) {
  while (Wire.available()) {
    char c = Wire.read();
    if (c != '\n' && c != '\r' && bufferIndex < sizeof(mensajeBuffer) - 1) {
      mensajeBuffer[bufferIndex++] = c;
    } else {
      mensajeBuffer[bufferIndex] = '\0';
      nuevoMensaje = true;
      bufferIndex = 0;
    }
  }
}

void procesarMensaje(String mensaje) {
  Serial.print("Mensaje recibido: ");
  Serial.println(mensaje);
  
  // Procesar comandos válidos
  if (mensaje.startsWith("NUM:")) {
    String valorStr = mensaje.substring(4);
    if (esNumerico(valorStr)) {
      progreso = valorStr.toInt();
      Serial.print("Progreso actualizado: ");
      Serial.println(progreso);
      mostrarEnLCD();
    }
  } 
  else if (mensaje.startsWith("POS:")) {
    String valorStr = mensaje.substring(4);
    if (esNumerico(valorStr)) {
      posicion = valorStr.toInt();
      Serial.print("Posicion actualizada: ");
      Serial.println(posicion);
    }
  } 
  else if (mensaje == "WIN") {
    gano = true;
    perdio = false;
    Serial.println("¡Victoria!");
    mostrarEnLCD();
  } 
  else if (mensaje == "LOSE") {
    perdio = true;
    gano = false;
    Serial.println("Derrota");
    mostrarEnLCD();
  } 
  else if (mensaje == "RESET") {
    reiniciarEstado();
    Serial.println("Reset completo");
  }
}

void controlarLuces() {
  unsigned long ahora = millis();
  
  switch (posicion) {
    case 1: // Primer lugar: verde fijo
      digitalWrite(LED_VERDE, HIGH);
      digitalWrite(LED_ROJO, LOW);
      break;
      
    case 2: // Segundo lugar: verde parpadeante
      if (ahora - lastToggle > 500) {
        ledState = !ledState;
        digitalWrite(LED_VERDE, ledState);
        digitalWrite(LED_ROJO, LOW);
        lastToggle = ahora;
      }
      break;
      
    case 3: // Tercer lugar: rojo parpadeante
      if (ahora - lastToggle > 500) {
        ledState = !ledState;
        digitalWrite(LED_ROJO, ledState);
        digitalWrite(LED_VERDE, LOW);
        lastToggle = ahora;
      }
      break;
      
    default: // Último lugar: rojo fijo
      digitalWrite(LED_VERDE, LOW);
      digitalWrite(LED_ROJO, HIGH);
      break;
  }
}

void mostrarEnLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  
  if (gano) {
    lcd.print("GANASTE!");
  } 
  else if (perdio) {
    lcd.print("PERDISTE");
  } 
  else {
    lcd.print("Progreso: ");
    lcd.print(progreso);
  }

  lcd.setCursor(0, 1);
  int barras = map(constrain(progreso, 0, 200), 0, 200, 0, 16);
  for (int i = 0; i < barras; i++) {
    lcd.write(byte(255)); // Carácter de bloque lleno
  }
  for (int i = barras; i < 16; i++) {
    lcd.print(" ");
  }
}

void reiniciarEstado() {
  progreso = 0;
  posicion = 0;
  gano = false;
  perdio = false;
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_ROJO, HIGH);
  
  lcd.clear();
  lcd.print("Reiniciado");
  delay(1000);
  mostrarEnLCD();
}

bool esNumerico(String str) {
  if (str.length() == 0) return false;
  for (unsigned int i = 0; i < str.length(); i++) {
    if (!isdigit(str.charAt(i))) {
      return false;
    }
  }
  return true;
}