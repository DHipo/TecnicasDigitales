#include <HardwareSerial.h>
#include <Arduino.h>
#include <SPIFFS.h>

#define RXD_X 16
#define TXD_X 17
#define RXD_Y 26
#define TXD_Y 27

struct Position
{
    float x;
    float y;
};

namespace Engine
{

    bool m_printing = false;
    int m_initialTime = 0;
    int m_endTime = 0;
    int m_elapseTime;

    File m_currentFile;
    String m_nameCurrentFile;

    HardwareSerial SerialX(1);
    HardwareSerial SerialY(2);

    Position currentPosition;

    void Init(const int _baudios)
    {
        SerialX.begin(_baudios, SERIAL_8N1, RXD_X, TXD_X);
        SerialY.begin(_baudios, SERIAL_8N1, RXD_Y, TXD_Y);

        // Configura los drivers aquí
        SerialX.print("$I=0\r\n"); // Deshabilita todas las respuestas de estado en X
        SerialX.print("M120\r\n"); // Inicia Grbl en X
        SerialX.print("M121\r\n"); // Inicia modo de espera en X

        SerialY.print("$I=0\r\n"); // Deshabilita todas las respuestas de estado en Y
        SerialY.print("M120\r\n"); // Inicia Grbl en Y
        SerialY.print("M121\r\n"); // Inicia modo de espera en Y
    }

    int SelectFile(const String _file)
    {
        if (!SPIFFS.exists(_file))
        {
            Serial.printf("No se encontró el archivo %s", _file);
            return -1;
        }

        m_currentFile = SPIFFS.open(_file, "r");

        if (!m_currentFile)
        {
            Serial.println("Error al abrir el archivo plotter.gcode");
            return -1;
        }

        m_nameCurrentFile = _file;
        Serial.print("Archivo seleccionado correctamente. ¡Listo para imprimir!");
        return 1;
    }

    void parseAndExecuteGCode(String line)
    {
        Position newPosition;

        if (line.startsWith("G21"))
        {
            float x = parseFloat(line);
            float y = parseFloat(line);

            Serial.print("Moviendo a (");
            Serial.print(x);
            Serial.print(", ");
            Serial.println(y);

            newPosition.x = x;
            newPosition.y = y;

            SerialX.print("G1 X");
            SerialX.print(x);
            SerialX.print(" F100\r\n");

            SerialY.print("G1 Y");
            SerialY.print(y);
            SerialY.print(" F100\r\n");
            return;
        }
        
        if (line.startsWith("G28"))
        {
            Serial.println("Detección de home en ambos ejes");
            SerialX.print("G28 X\r\n");
            SerialY.print("G28 Y\r\n");
        }
    }

    void StartPrinting() 
    {
        m_initialTime = millis();
        m_printing = true;
        
        while (m_currentFile.available())
        {
            parseAndExecuteGCode(m_currentFile.readStringUntil('\n'));
            delay(10); // Espera un poco entre líneas para evitar sobrecarga
        }

        m_printing = false;
        m_endTime = millis();

        m_elapseTime = m_endTime - m_initialTime;
    }

    float parseFloat(String line)
    {
        float result;
        sscanf(line.c_str(), "%f", &result);
        return result;
    }

#pragma region getters

    int getInitialTime()
    {
        return m_initialTime;
    }

    int getEndTime() 
    {
        return m_endTime;
    }

    String getFileName ()
    {
        return m_nameCurrentFile;
    }

    bool getState() 
    {
        return m_printing;
    }

    int getElapseTime() {
        return m_elapseTime;
    }
#pragma endregion

    TaskHandle_t printingTaskHandle = NULL;
    bool taskShouldRun = false;

    void printCallback(void *pvParameters) {
        while(true) {
            if(taskShouldRun) {
                StartPrinting();
                vTaskDelete(NULL); // Elimina esta tarea cuando termina
            } else {
                delay(10); // Espera un poco antes de volver a verificar
            }
        }
    }

    void StartPrintingAsync() {
        taskShouldRun = true;
        xTaskCreate(printCallback, "printing_task", 4096, NULL, 1, &printingTaskHandle);
    }

};
