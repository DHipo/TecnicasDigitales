#include <Arduino.h>
#include <AccelStepper.h>

#define PIN_STEP_X 16 // Pin STEP del driver
#define PIN_DIR_X 17  // Pin DIR del driver

#define PIN_STEP_Y 18 // Pin STEP del driver
#define PIN_DIR_Y 19 // Pin DIR del driver


// Inicializamos el motor en modo DRIVER
AccelStepper motorX(AccelStepper::DRIVER, PIN_STEP_X, PIN_DIR_X);
AccelStepper motorY(AccelStepper::DRIVER, PIN_STEP_Y, PIN_DIR_Y);

// Variables de estado
float currentX = 0;          // Posición actual en mm
float feedRate = 250;        // Velocidad predeterminada en pasos/segundo
bool clockwise = true;       // Dirección del motor (sentido horario)
unsigned long pauseStart;    // Momento de inicio de la pausa
bool enPausa = false;        // Estado de pausa activa
int currentCommandIndex = 0; // Índice del comando actual

const float STEPS_PER_MM = 20; // Relación pasos por mm

// Lista de comandos G-code para pruebas
String gcode[] = {
    "G1 X50 F2000", // Mover a X=50 con velocidad 300 pasos/s
    "G0 X100",      // Movimiento rápido a X=100
    "G4 S3000",     // Pausar 3 segundos
    "S1",           // Cambiar dirección a horario
    "G1 X0 F2000",  // Mover a X=0 con velocidad 300 pasos/s
    "S0",           // Cambiar dirección a antihorario
    "G1 X100 F2000" // Mover a X=100 con nueva dirección
};

// Función para mover el motor a una nueva posición
void moverMotor(AccelStepper& motor, float newX)
{
  int pasosX = newX * STEPS_PER_MM; // Convertir mm a pasos
  digitalWrite(PIN_DIR_X, clockwise);  // Establecer dirección true = izq false = derecha
  digitalWrite(PIN_DIR_Y, !clockwise);  // Establecer dirección false = arriba true = abajo
  motor.move(pasosX);
  motor.run();
  while (motor.isRunning())
  {
    motor.run();
  }

  currentX = newX; // Actualizar la posición actual
  Serial.printf("Motor movido a %.2f mm\n", currentX);
}

// Función para establecer la velocidad del motor
void establecerVelocidad(float velocidad)
{
  feedRate = velocidad;
  motorX.setMaxSpeed(feedRate);
  Serial.printf("Velocidad establecida: %.2f pasos/s\n", feedRate);
}

// Función para cambiar la dirección del motor
void cambiarDireccion(bool sentidoHorario)
{
  clockwise = sentidoHorario;
  Serial.printf("Dirección cambiada: %s\n", clockwise ? "Horario" : "Antihorario");
  enPausa = true;
}

// Función para detener el motor
void detenerMotor(AccelStepper& motor)
{
  motor.stop();           // Detener el motor suavemente
  motor.disableOutputs(); // Deshabilitar las salidas para evitar vibraciones
  Serial.println("Motor detenido");
}

// Función para iniciar una pausa no bloqueante
void iniciarPausa(int tiempo)
{
  detenerMotor(motorX); // Asegurar que el motor se detenga
  enPausa = true;
  pauseStart = millis(); // Registrar el momento de inicio de la pausa
  Serial.printf("Pausa iniciada por %d ms\n", tiempo);
}

// Verificar si la pausa ha terminado
bool pausaCompletada(int tiempo)
{
  if (millis() - pauseStart >= tiempo)
  {
    enPausa = false;
    Serial.println("Pausa completada");
    return true;
  }
  return false;
}

// Interpretar y ejecutar comandos G-code
void interpretarGcode(String comando)
{
  String tipoComando = comando.substring(0, 2); // Extraer tipo de comando

  switch (tipoComando.toInt())
  {
  case 1:
  { // G1: Movimiento controlado
    float newX = comando.substring(comando.indexOf('X') + 1).toFloat();
    // if (comando.indexOf('F') >= 0) {
    //    float velocidad = comando.substring(comando.indexOf('F') + 1).toFloat();
    //    establecerVelocidad(velocidad);
    //  }
    moverMotor(motorX, newX);
    break;
  }
  case 0:
  { // G0: Movimiento rápido
    float newX = comando.substring(comando.indexOf('X') + 1).toFloat();
    moverMotor(motorX, newX);
    break;
  }
  case 4:
  { // G4: Pausa
    int tiempo = comando.substring(comando.indexOf('S') + 1).toInt();
    iniciarPausa(tiempo);
    break;
  }
  default:
  { // Cambiar dirección (S)
    if (comando.startsWith("S"))
    {
      bool sentidoHorario = comando.substring(1).toInt() == 1;
      cambiarDireccion(sentidoHorario);
    }
    else
    {
      Serial.println("Comando no reconocido");
    }
    break;
  }
  }
}

// Ejecutar el siguiente comando G-code en la lista
void ejecutarComandos()
{
  // Si no estamos en pausa, ejecutar el siguiente comando
  if (currentCommandIndex < sizeof(gcode) / sizeof(gcode[0]))
  {
    Serial.println(gcode[currentCommandIndex]);
    interpretarGcode(gcode[currentCommandIndex]);
    currentCommandIndex++; // Avanzar al siguiente comando
  }
  else
  {
    Serial.println("Todos los comandos ejecutados.");
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

  detenerMotor(motorX);
  detenerMotor(motorY);
}

void loop()
{
}
