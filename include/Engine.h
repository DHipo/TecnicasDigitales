#pragma once

#include <HardwareSerial.h>
#include <Arduino.h>
#include <AccelStepper.h>
#include <SPIFFS.h>
#include <vector>

#define PIN_STEP_X 16 // Pin STEP del driver
#define PIN_DIR_X 17  // Pin DIR del driver
#define PIN_STEP_Y 18 // Pin STEP del driver
#define PIN_DIR_Y 19  // Pin DIR del driver

#define MAX_DISTANCE_X 200 // 20 cm
#define MAX_DISTANCE_Y 250 // 25 cm

namespace Engine
{
    // Inicializamos el motor en modo DRIVER
    AccelStepper motorX(AccelStepper::DRIVER, PIN_STEP_X, PIN_DIR_X);
    AccelStepper motorY(AccelStepper::DRIVER, PIN_STEP_Y, PIN_DIR_Y);

    // Variables de estado
    struct fVec2
    {
        float x, y;
    };

    fVec2 position = {0.f, 0.f};
    float feedRate = 250;        // Velocidad predeterminada en pasos/segundo
    bool clockwise = false;      // Dirección del motor (sentido horario)
    unsigned long pauseStart;    // Momento de inicio de la pausa
    bool enPausa = false;        // Estado de pausa activa
    int currentCommandIndex = 0; // Índice del comando actual

    const float STEPS_PER_MM = 20; // Relación pasos por mm

    // Lista de comandos G-code para pruebas
    std::vector<String> gcode_rectangulo = {
        "G1 X0 Y0",    // Mover a punto inicial (0,0)
        "G1 X170 Y0",  // Mover a (100,0) - esquina inferior derecha
        "G1 X0 Y150",  // Mover a (100,100) - esquina superior derecha
        "G1 X-170 Y0", // Mover a (0,100) - esquina superior izquierda
        "G1 X0 Y-150"  // Volver al punto inicial (0,0)
    };

    // Lista de comandos G-code para dibujar un círculo en segmentos rectos, un eje a la vez
    std::vector<String> gcode_circulo = {
        "G1 X0 Y125",   // Mover a (10,0)
        "G1 X130 Y0",   // Mover a (10,10)
        "G1 X-30 Y30",  // Mover a (0,10)
        "G1 X-30 Y-30", // Mover a (-10,10)
        "G1 X30 Y-30",  // Mover a (-10,0)
        "G1 X30 Y30",   // Mover a (-10,-10)
    };

    std::vector<String> gcode_arco = {
        "G1 X0 Y100",
        "G1 X80 Y0",
        "G1 X100 Y80"};

    std::vector<String> circleGCode = {
        "G00 X0 Y0",
        "G01 Z-1 F100",
        "G02 X100 Y0 I50",
        "G00 Z0.5",
        "M30"};

    std::vector<String> m_currentGcode = gcode_rectangulo;

    String m_currentLine = "";
    File m_currentFile;
    String m_nameCurrentFile;

    HardwareSerial SerialX(1);
    HardwareSerial SerialY(2);

    static bool m_shouldPause = false;

    fVec2 currentPosition;

    void interpretarGcode(String);
    void backToOrigin();
    void detenerMotor(AccelStepper&);

    TaskHandle_t printingTaskHandle = NULL;
    bool taskShouldRun = false;

    String getCurrentLine()
    {
        return m_currentLine;
    }

    void StartPrinting()
    {
        Serial.printf("current size of gcode: %d", m_currentGcode.size());
        for (int i = 0; i < m_currentGcode.size(); i++){
            interpretarGcode(m_currentGcode[i]);
            delay(500);
        }

        backToOrigin();

        detenerMotor(motorX);
        detenerMotor(motorY);

        taskShouldRun = false;
    }

    void printCallback(void *pvParameters)
    {
        while (true)
        {
            if (taskShouldRun)
            {
                StartPrinting();
            }
            else
            {
                delay(10); // Espera un poco antes de volver a verificar
            }
        }
    }

    void createTask()
    {
        xTaskCreate(printCallback, "printing_task", 4096, NULL, 1, &printingTaskHandle);
    }

    void StartPrintingAsync()
    {
        taskShouldRun = true;
        Serial.printf("task: %d", taskShouldRun);
    }

    void setCurrentGcode(std::vector<String> _gcode)
    {
        m_currentGcode = _gcode;
    }

    void moverMotores(float posX, float posY)
    {
        // Mover ambos motores a las nuevas posiciones
        motorX.move( (posX - position.x) * STEPS_PER_MM);
        motorY.move( (posY - position.y) * STEPS_PER_MM);

        while (motorX.isRunning() || motorY.isRunning())
        {
            motorX.run();
            motorY.run();
        }

        position.y = motorY.currentPosition() / STEPS_PER_MM;
        position.x = motorX.currentPosition() / STEPS_PER_MM;
        Serial.printf("current pos: (%.2f, %.2f)\n", position.x, position.y);
    }

    void actualizarPosicionActual()
    {
        position.x = motorX.currentPosition();
        position.y = motorY.currentPosition();

        Serial.printf("current pos: (%.2f, %.2f)\n", position.x, position.y);
    }

    void interpretarCurva(const String &comando)
    {
        int iIndex = comando.indexOf('I');
        int jIndex = comando.indexOf('J');
        int pIndex = comando.indexOf('P');
        int qIndex = comando.indexOf('Q');

        float i = 0, j = 0, p = 0, q = 0;

        if (iIndex != -1)
            i = comando.substring(iIndex + 1, jIndex).toFloat();
        if (jIndex != -1)
            j = comando.substring(jIndex + 1, pIndex).toFloat();
        if (pIndex != -1)
            p = comando.substring(pIndex + 1, qIndex).toFloat();
        if (qIndex != -1)
            q = comando.substring(qIndex + 1).toFloat();

        Serial.printf("Ejecutando G5: Movimiento en arco desde (%.2f, %.2f) a (%.2f, %.2f), I=%.2f, J=%.2f, P=%.2f, Q=%.2f\n",
                      position.x, position.y, position.x + i, position.y + j, i, j, p, q);

        // Calcular los deltas entre el punto actual y el punto final
        float deltaX = i;
        float deltaY = j;

        // Calcular el punto de control para el arco
        float controlX = position.x + (deltaX / 2);
        float controlY = position.y + (deltaY / 2);

        // Interpolación cuadrática para calcular los puntos de control
        for (int t = 0; t <= 100; t++)
        { // 100 pasos por defecto
            float u = (float)t / 100.0f;

            float x = (1 - u) * (1 - u) * position.x + 2 * (1 - u) * u * controlX + u * u * (position.x + deltaX);
            float y = (1 - u) * (1 - u) * position.y + 2 * (1 - u) * u * controlY + u * u * (position.y + deltaY);

            // Aplicar tensiones P y Q
            float px = x + (p * (x - controlX));
            float py = y + (q * (y - controlY));

            // Mover los motores a la posición calculada
            moverMotores(px - position.x, py - position.y);

            delay(10); // Delay para controlar la velocidad de movimiento
        }
    }

    void interpretarDireccion(bool dir)
    {
        // Establecer la dirección de los motores
        digitalWrite(PIN_DIR_X, !dir); // Dirección del eje X
        digitalWrite(PIN_DIR_Y, dir);  // Dirección opuesta en el eje Y

        Serial.println(dir ? "Dirección: Horario (X adelante, Y abajo)" : "Dirección: Antihorario (X atrás, Y arriba)");
    }

    void interpretarDireccion(const String &comando)
    {
        int sentido = comando.substring(1).toInt();

        if (sentido != 0 && sentido != 1)
        {
            Serial.println("Error: Comando de dirección inválido (solo S1 o S0).");
            return;
        }

        // Establecer la dirección de los motores
        clockwise = (sentido == 1);
        digitalWrite(PIN_DIR_X, clockwise); // Dirección del eje X
        digitalWrite(PIN_DIR_Y, clockwise); // Dirección opuesta en el eje Y

        Serial.println(clockwise ? "Dirección: Horario (X adelante, Y abajo)" : "Dirección: Antihorario (X atrás, Y arriba)");
    }

    void interpretarMovimiento(const String &comando)
    {
        int posXIndex = comando.indexOf('X');
        int posYIndex = comando.indexOf('Y');
        // "G1X-10Y-10"
        float newX = position.x; // Valor actual como predeterminado
        float newY = position.y; // Valor actual como predeterminado

        // Extraer y validar la nueva posición en X, si está especificada
        if (posXIndex != -1)
        {
            // Obtener el valor de X desde la posición 'X'
            // X-160
            String xValue = comando.substring(posXIndex + 1);
            //-160
            // Verificar si hay un signo negativo
            if (xValue.charAt(0) == '-')
                newX = -xValue.substring(1).toFloat();
            else
                newX = xValue.toFloat();
            Serial.printf("newX value: %.2f\n", newX);
            if (position.x + newX < 0 || position.x + newX > MAX_DISTANCE_X)
            {
                Serial.printf("Error: Movimiento en X fuera de límites (0 - %d mm).\n", MAX_DISTANCE_X);
                return;
            }
        }

        // Extraer y validar la nueva posición en Y, si está especificada
        if (posYIndex != -1)
        {
            // Obtener el valor de X desde la posición 'X'
            String yValue = comando.substring(posYIndex + 1);
            // Verificar si hay un signo negativo
            if (yValue.charAt(0) == '-')
                newY = -yValue.substring(1).toFloat();
            else
                newY = yValue.toFloat();

            Serial.printf("newY value: %.2f\n", newY);
            if (position.y + newY < 0 || position.y + newY > MAX_DISTANCE_Y)
            {
                Serial.printf("Error: Movimiento en Y fuera de límites (0 - %d mm).\n", MAX_DISTANCE_Y);
                return;
            }
        }
        Serial.printf("moving to x: %.2f y: %.2f", newX, newY);
        moverMotores(newX, newY);
    }

    void pausa(int tiempoPausa)
    {
        Serial.printf("Pausa de %d ms\n", tiempoPausa);
        delay(tiempoPausa); // Pausar durante el tiempo especificado en milisegundos
    }

    // Función para interpretar comandos de pausa G4
    void interpretarPausa(const String &comando)
    {
        int tiempoIndex = comando.indexOf('S');
        if (tiempoIndex == -1)
        {
            Serial.println("Error: Comando de pausa sin tiempo especificado.");
            return;
        }

        int tiempoPausa = comando.substring(tiempoIndex + 1).toInt();
        if (tiempoPausa <= 0)
        {
            Serial.println("Error: Tiempo de pausa inválido.");
            return;
        }

        pausa(tiempoPausa);
    }
    void interpretarArco(const String &comando)
    {
        int iIndex = comando.indexOf('I');
        int jIndex = comando.indexOf('J');
        int pIndex = comando.indexOf('P');
        int qIndex = comando.indexOf('Q');

        float i = 0, j = 0, p = 0, q = 0;

        if (iIndex != -1)
            i = comando.substring(iIndex + 1, jIndex).toFloat();
        if (jIndex != -1)
            j = comando.substring(jIndex + 1, pIndex).toFloat();
        if (pIndex != -1)
            p = comando.substring(pIndex + 1, qIndex).toFloat();
        if (qIndex != -1)
            q = comando.substring(qIndex + 1).toFloat();

        Serial.printf("Ejecutando G02: Movimiento en arco desde (%.2f, %.2f) a (%.2f, %.2f), I=%.2f, J=%.2f, P=%.2f, Q=%.2f\n",
                      position.x, position.y, position.x + i, position.y + j, i, j, p, q);

        // Calcular los deltas entre el punto actual y el punto final
        float deltaX = i;
        float deltaY = j;

        // Calcular el punto de control para el arco
        float controlX = position.x + (deltaX / 2);
        float controlY = position.y + (deltaY / 2);

        // Interpolación cuadrática para calcular los puntos de control
        for (int t = 0; t <= 100; t++)
        { // 100 pasos por defecto
            float u = (float)t / 100.0f;

            float x = (1 - u) * (1 - u) * position.x + 2 * (1 - u) * u * controlX + u * u * (position.x + deltaX);
            float y = (1 - u) * (1 - u) * position.y + 2 * (1 - u) * u * controlY + u * u * (position.y + deltaY);

            // Aplicar tensiones P y Q
            float px = x + (p * (x - controlX));
            float py = y + (q * (y - controlY));

            // Mover los motores a la posición calculada
            moverMotores(px - position.x, py - position.y);

            delay(10); // Delay para controlar la velocidad de movimiento
        }
    }

    // Función para detener el motor
    void detenerMotor(AccelStepper &motor)
    {
        motor.stop();           // Detener el motor suavemente
        motor.disableOutputs(); // Deshabilitar las salidas para evitar vibracione
        Serial.println("Motor detenido");
    }

    void interpretarGcode(String comando)
    {
        comando.trim();

        if (comando.startsWith("G1") || comando.startsWith("G0"))
        {
            interpretarMovimiento(comando);
        }
        else if (comando.startsWith("G4"))
        {
            interpretarPausa(comando);
        }
        else if (comando.startsWith("G5"))
        {
            interpretarCurva(comando);
        }
        else if (comando.startsWith("G02"))
        {
            interpretarArco(comando);
        }
        else if (comando.startsWith("S"))
        {
            interpretarDireccion(comando);
        }
        else
        {
            Serial.println("Error: Comando desconocido.");
        }
    }

    void backToOrigin()
    {
        moverMotores((-position.x), (-position.y));
    }

    void initMotores()
    {
        createTask();
        motorX.setCurrentPosition(0);
        motorY.setCurrentPosition(0);

        motorX.setMaxSpeed(8000.0f);
        motorX.setSpeed(600);
        motorX.setAcceleration(2000);
        motorX.enableOutputs();

        motorY.setMaxSpeed(8000.0f);
        motorY.setSpeed(600);
        motorY.setAcceleration(2000);
        motorY.enableOutputs();

        // ni idea si funca
        motorX.setPinsInverted(true);
        moverMotores(10.f, 0.f);

        motorY.setPinsInverted(false);
        moverMotores(0.f, 10.f);

        backToOrigin();
    }
};
