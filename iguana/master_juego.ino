/* 
Posible mejora para el UART

En la función enviarUART, agregar verificación
void enviarUART(const char* comando, int valor) {
  // Determinar a qué Mega enviar (Serial o SoftwareSerial)
  if (direccion == CONTROL_1 || direccion == CONTROL_2) {
    // Enviar a Mega A por Serial (jugadores 1-2)
    Serial.print(comando);
    Serial.print(valor);
    Serial.print(":P");
    Serial.println(direccion == CONTROL_1 ? "1" : "2");
    
    // Depuración
    Serial.print("Enviado a Mega A: ");
    Serial.print(comando);
    Serial.print(valor);
    Serial.print(":P");
    Serial.println(direccion == CONTROL_1 ? "1" : "2");
  } else {
    // Enviar a Mega B por SoftwareSerial (jugadores 3-4)
    megaSerial.print(comando);
    megaSerial.print(valor);
    megaSerial.print(":P");
    megaSerial.println(direccion == CONTROL_3 ? "3" : "4");
    
    // Depuración
    Serial.print("Enviado a Mega B: ");
    Serial.print(comando);
    Serial.print(valor);
    Serial.print(":P");
    Serial.println(direccion == CONTROL_3 ? "3" : "4");
  }
  
  // Pequeña pausa para evitar saturación
  delay(10);
}

*/

/*
Estructura inicial del puerto serial doble para meter mas de un arduino mega para controlar luces
Esto pensado para tener distintos UART para conectarse al mismo arduino master


Sin embargo. Esto sobrecarga demasiado al master, haciendo que no actualice de manera correcta la pantalla de los arduinos
Por ende mal funcionando por problemas de optimizacion

Ademas, el i2c se sobre carga... Ya que como debe de enviar informacion a los slaves para que enseñen en su pantalla la posicion y progreso del momento, acaba
relentizando toda la operacion, haciendo que vaya muy lento, considerando que debe de acabar todo muy rapido.

La estructura para el master es

0x08, 0x09, 0xA, 0x0B para los controladores (Controles, sin embargo el boton del control esta conectado al master)
0x10, 0x11 pensados para los mega, sin embargo... esto dejo de ser asi debido a que el master se comunica con uart con ellos. Siendo muchisimo mas simple todo
El problema de mezclar todo esto, es que el cable del i2c tiene bastante interferencia... por ende los datos se pierden mision... hayar una forma de solucionarlo

Me gustaria que al momento de arreglar el codigo hicieras una parte en la cual se comunica con el monitor serial para supervisar lo que esta pasando... o sea... Ver si el el 0x08
o el 0x09... y asi sucesivamente responden bien y escriben y leen correctamente. En caso de indicar que no, que este marque el estado de la linea. Con lo cual, deje por escrito eso
Sin embargo... que no me prohiba usarlo y que continue tal cual como empezo todo, no deseo que me bloque cosas por posibles errores fantasmas. Solo que los mencione

Esto facilmente puede ir en el setup para confirmar... Ademas, lo podrias dejar visual de manera fisica... o sea... 0x08 se comunica bien... entonces por el monitor serial
me enseña que se comunica bien y en la pantalla me dice Correcto, direccion X funcionando perfectamente y luego queda listo para el juego

idealmente prueba todo antes de empezar y compara todos los resultados con cosas ya hechas. Si no especifica que no funciona de dicha manera, entonces cambialo de inmediato.


#include <Wire.h>
#include <SoftwareSerial.h>

#define MAX_STEPS 200

// Puerto software para el segundo Mega
SoftwareSerial megaSerial(6, 7); // RX, TX

// ... resto de variables y configuración

void enviarUART(const char* comando, int valor, int jugador) {
  // Determinar a qué Mega enviar
  if (jugador == 1 || jugador == 2) {
    // Enviar al Mega A por Serial hardware
    Serial.print(comando);
    Serial.print(valor);
    Serial.print(":P");
    Serial.println(jugador);
  } else {
    // Enviar al Mega B por SoftwareSerial
    megaSerial.print(comando);
    megaSerial.print(valor);
    megaSerial.print(":P");
    megaSerial.println(jugador);
  }
}

void setup() {
  Wire.begin();
  Serial.begin(9600);    // Para Mega A
  megaSerial.begin(9600); // Para Mega B
  // ... resto del setup
}
*/

#include <Wire.h>
#include <SoftwareSerial.h>

#define MAX_STEPS 200

// Pines de botones (con resistencias pull-down externas)
const int J1_BUTTON = 2;
const int J2_BUTTON = 3;
const int J3_BUTTON = 4;
const int J4_BUTTON = 5;

// Direcciones I2C de los controles
const int CONTROL_1 = 0x08;
const int CONTROL_2 = 0x09;
const int CONTROL_3 = 0x0A;
const int CONTROL_4 = 0x0B;

// Configuración SoftwareSerial para el segundo Mega
SoftwareSerial megaSerial(6, 7); // RX, TX (no usamos RX)

class Jugador {
public:
  uint8_t direccion;
  uint8_t pinBoton;
  int progreso;
  bool haTerminado;

  Jugador(uint8_t addr, uint8_t btn) {
    direccion = addr;
    pinBoton = btn;
    progreso = 0;
    haTerminado = false;
    pinMode(pinBoton, INPUT);
  }

  void revisarBoton() {
    if (haTerminado) return;
    if (digitalRead(pinBoton) == HIGH) {
      delay(50); // Debounce
      if (digitalRead(pinBoton) == HIGH) {
        progreso++;
        
        // Enviar NUM al control via I2C
        enviarI2C("NUM:", progreso);
        
        // Enviar BAR a la Mega correspondiente via UART
        enviarUART("BAR:", progreso);
        
        while (digitalRead(pinBoton) == HIGH); // Esperar a que suelten el botón
      }
    }
    if (progreso >= MAX_STEPS) {
      haTerminado = true;
    }
  }

  void enviarI2C(const char* comando, int valor) {
    Wire.beginTransmission(direccion);
    Wire.print(comando);
    Wire.print(valor);
    Wire.endTransmission();
  }

  void enviarTextoI2C(const char* texto) {
    Wire.beginTransmission(direccion);
    Wire.print(texto);
    Wire.endTransmission();
  }

  void enviarUART(const char* comando, int valor) {
    // Determinar a qué Mega enviar (Serial o SoftwareSerial)
    if (direccion == CONTROL_1 || direccion == CONTROL_2) {
      // Enviar a Mega A por Serial (jugadores 1-2)
      Serial.print(comando);
      Serial.print(valor);
      Serial.print(":P");
      Serial.println(direccion == CONTROL_1 ? "1" : "2");
    } else {
      // Enviar a Mega B por SoftwareSerial (jugadores 3-4)
      megaSerial.print(comando);
      megaSerial.print(valor);
      megaSerial.print(":P");
      megaSerial.println(direccion == CONTROL_3 ? "3" : "4");
    }
  }

  void enviarTextoUART(const char* texto) {
    // Determinar a qué Mega enviar
    if (direccion == CONTROL_1 || direccion == CONTROL_2) {
      Serial.print(texto);
      Serial.print(":P");
      Serial.println(direccion == CONTROL_1 ? "1" : "2");
    } else {
      megaSerial.print(texto);
      megaSerial.print(":P");
      megaSerial.println(direccion == CONTROL_3 ? "3" : "4");
    }
  }
};

Jugador jugador1(CONTROL_1, J1_BUTTON);
Jugador jugador2(CONTROL_2, J2_BUTTON);
Jugador jugador3(CONTROL_3, J3_BUTTON);
Jugador jugador4(CONTROL_4, J4_BUTTON);

Jugador* jugadores[] = { &jugador1, &jugador2, &jugador3, &jugador4 };
const int numJugadores = sizeof(jugadores) / sizeof(jugadores[0]);

bool yaHayGanador = false;

void setup() {
  Wire.begin();
  Serial.begin(9600);     // Para Mega A
  megaSerial.begin(9600); // Para Mega B
  
  // Inicializar jugadores
  for (int i = 0; i < numJugadores; i++) {
    jugadores[i]->progreso = 0;
    jugadores[i]->haTerminado = false;
  }
  
  Serial.println("Sistema iniciado");
}

void loop() {
  for (int i = 0; i < numJugadores; i++) {
    jugadores[i]->revisarBoton();
  }

  if (!yaHayGanador) {
    int posiciones[numJugadores];
    for (int i = 0; i < numJugadores; i++) posiciones[i] = i;
    
    // Ordenar por progreso (de mayor a menor)
    for (int i = 0; i < numJugadores - 1; i++) {
      for (int j = i + 1; j < numJugadores; j++) {
        if (jugadores[posiciones[j]]->progreso > jugadores[posiciones[i]]->progreso) {
          int tmp = posiciones[i];
          posiciones[i] = posiciones[j];
          posiciones[j] = tmp;
        }
      }
    }

    // Enviar POS a cada control via I2C
    for (int i = 0; i < numJugadores; i++) {
      jugadores[posiciones[i]]->enviarI2C("POS:", i + 1);
    }

    // Verificar si hay ganador
    if (jugadores[posiciones[0]]->progreso >= MAX_STEPS) {
      yaHayGanador = true;
      
      // Enviar WIN/LOSE a controles via I2C
      jugadores[posiciones[0]]->enviarTextoI2C("WIN");
      for (int i = 1; i < numJugadores; i++) {
        jugadores[posiciones[i]]->enviarTextoI2C("LOSE");
      }
      
      // Enviar WIN/LOSE a Megas via UART
      jugadores[posiciones[0]]->enviarTextoUART("WIN");
      for (int i = 1; i < numJugadores; i++) {
        jugadores[posiciones[i]]->enviarTextoUART("LOSE");
      }
      
      Serial.println("Juego terminado - Reiniciando en 5 segundos...");
      delay(5000);
      
      // Reiniciar juego
      yaHayGanador = false;
      for (int i = 0; i < numJugadores; i++) {
        jugadores[i]->progreso = 0;
        jugadores[i]->haTerminado = false;
      }
      
      // Enviar RESET a todos
      for (int i = 0; i < numJugadores; i++) {
        jugadores[i]->enviarTextoI2C("RESET");
      }
      Serial.println("RESET");
      megaSerial.println("RESET");
      
      Serial.println("Juego reiniciado");
    }
  }
  
  delay(10);
}