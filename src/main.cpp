#include <Arduino.h>
#include <AccelStepper.h>

#define PIN_STEP_X 16 // Pin STEP del driver
#define PIN_DIR_X 17  // Pin DIR del driver

#define PIN_STEP_Y 18 // Pin STEP del driver
#define PIN_DIR_Y 19 // Pin DIR del driver

#define MAX_DISTANCE_X 200 // 20 cm
#define MAX_DISTANCE_Y 250 // 25 cm

// Inicializamos el motor en modo DRIVER
AccelStepper motorX(AccelStepper::DRIVER, PIN_STEP_X, PIN_DIR_X);
AccelStepper motorY(AccelStepper::DRIVER, PIN_STEP_Y, PIN_DIR_Y);

// Variables de estado
float currentX = 0;          // Posición actual en mm
float currentY = 0;          // Posición actual en mm
float feedRate = 250;        // Velocidad predeterminada en pasos/segundo
bool clockwise = true;       // Dirección del motor (sentido horario)
unsigned long pauseStart;    // Momento de inicio de la pausa
bool enPausa = false;        // Estado de pausa activa
int currentCommandIndex = 0; // Índice del comando actual

const float STEPS_PER_MM = 20; // Relación pasos por mm

// Lista de comandos G-code para pruebas
String gcode[] = {
    "G1 X0 Y0",       // Mover a punto inicial (0,0)
    "G1 X170 Y0",     // Mover a (100,0) - esquina inferior derecha
    "G1 X0 Y150",   // Mover a (100,100) - esquina superior derecha
    "G1 X-170 Y0",     // Mover a (0,100) - esquina superior izquierda
    "G1 X0 Y-150"        // Volver al punto inicial (0,0)
};

void interpretarDireccion(bool dir) {
  // Establecer la dirección de los motores
  digitalWrite(PIN_DIR_X, !dir);       // Dirección del eje X
  digitalWrite(PIN_DIR_Y, dir);      // Dirección opuesta en el eje Y

  Serial.println(dir ? "Dirección: Horario (X adelante, Y abajo)" : "Dirección: Antihorario (X atrás, Y arriba)");
}

void interpretarDireccion(const String& comando) {
  int sentido = comando.substring(1).toInt();

  if (sentido != 0 && sentido != 1) {
    Serial.println("Error: Comando de dirección inválido (solo S1 o S0).");
    return;
  }

  // Establecer la dirección de los motores
  clockwise = (sentido == 1);
  digitalWrite(PIN_DIR_X, clockwise);       // Dirección del eje X
  digitalWrite(PIN_DIR_Y, !clockwise);      // Dirección opuesta en el eje Y

  Serial.println(clockwise ? "Dirección: Horario (X adelante, Y abajo)" : "Dirección: Antihorario (X atrás, Y arriba)");
}

// Función para mover el motor a una nueva posición
void moverMotor(AccelStepper& motor, float newX)
{
  int pasosX = newX * STEPS_PER_MM; // Convertir mm a pasos
  motor.move(pasosX);
  motor.run();
  while (motor.isRunning())
  {
    motor.run();
  }

  currentX = newX; // Actualizar la posición actual
  Serial.printf("Motor movido a %.2f mm\n", currentX);
}

void interpretarMovimiento(const String& comando) {
  int posXIndex = comando.indexOf('X');
  int posYIndex = comando.indexOf('Y');

  float newX = currentX; // Valor actual como predeterminado
  float newY = currentY; // Valor actual como predeterminado

  // Extraer y validar la nueva posición en X, si está especificada
  if (posXIndex != -1) {
    // Obtener el valor de X desde la posición 'X'
      // X-160
      String xValue = comando.substring(posXIndex + 1);
      //-160
      // Verificar si hay un signo negativo
      if (xValue.charAt(0) == '-') {
          // -160
          newX = - xValue.substring(1).toFloat();
      }
      else {
        newX = xValue.toFloat();
      }
      
    if (newX > MAX_DISTANCE_X) {
      Serial.printf("Error: Movimiento en X fuera de límites (0 - %d mm).\n", MAX_DISTANCE_X);
      return;
    }
    
    newX = -newX; // Invertir para movimiento positivo en X
  }

  // Extraer y validar la nueva posición en Y, si está especificada
  if (posYIndex != -1) {
    // Obtener el valor de X desde la posición 'X'
      String yValue = comando.substring(posYIndex + 1);
      // Verificar si hay un signo negativo
      if (yValue.charAt(0) == '-') {
          newY = - yValue.substring(1).toFloat();
      }
      else {
        newY = yValue.toFloat();
      }
    if (newY > MAX_DISTANCE_Y) {
      Serial.printf("Error: Movimiento en Y fuera de límites (0 - %d mm).\n", MAX_DISTANCE_Y);
      return;
    }
  }

  // Mover ambos motores a las nuevas posiciones
  moverMotor(motorX, newX);
  moverMotor(motorY, newY);
}


void pausa(int tiempoPausa) {
  Serial.printf("Pausa de %d ms\n", tiempoPausa);
  delay(tiempoPausa); // Pausar durante el tiempo especificado en milisegundos
}

// Función para interpretar comandos de pausa G4
void interpretarPausa(const String& comando) {
  int tiempoIndex = comando.indexOf('S');
  if (tiempoIndex == -1) {
    Serial.println("Error: Comando de pausa sin tiempo especificado.");
    return;
  }

  int tiempoPausa = comando.substring(tiempoIndex + 1).toInt();
  if (tiempoPausa <= 0) {
    Serial.println("Error: Tiempo de pausa inválido.");
    return;
  }

  pausa(tiempoPausa);
}

// Función para detener el motor
void detenerMotor(AccelStepper& motor)
{
  motor.stop();           // Detener el motor suavemente
  motor.disableOutputs(); // Deshabilitar las salidas para evitar vibraciones
  Serial.println("Motor detenido");
}

// Función principal de interpretación de G-code
void interpretarGcode(String comando) {
  comando.trim(); // Elimina espacios en blanco al inicio y al final

  if (comando.startsWith("G1") || comando.startsWith("G0")) {
    interpretarMovimiento(comando);
  } else if (comando.startsWith("G4")) {
    interpretarPausa(comando);
  } else if (comando.startsWith("S")) {
    interpretarDireccion(comando);
  } else {
    Serial.println("Error: Comando desconocido.");
  }
}

void setup()
{
  motorX.setMaxSpeed(8000.0f);
  motorX.setSpeed(600);
  motorX.setAcceleration(2000);
  motorX.enableOutputs();
  
  motorY.setMaxSpeed(8000.0f);
  motorY.setSpeed(600);
  motorY.setAcceleration(2000);
  motorY.enableOutputs();
  delay(100);
  
  // se regula la corriente
  moverMotor(motorX, -200);
  moverMotor(motorX, 200);

  moverMotor(motorY, 250);
  moverMotor(motorY, -250);

  /* do stuff */

  for (int i = 0; i < 5; i++)
    interpretarGcode(gcode[i]);
  

  detenerMotor(motorX);
  detenerMotor(motorY);
}

void loop()
{
}
