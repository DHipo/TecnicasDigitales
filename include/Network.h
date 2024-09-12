#include <Arduino.h>
#include <WiFi.h>

namespace Network {

    const char* apSSID = "PEN-PLOTER";      // Nombre de la red WiFi para el punto de acceso
    const char* apPassword = "123456789"; // Contraseña de la red WiFi para el punto de acceso

    void setupAccessPoint(const char* ssid, const char* password) {
    WiFi.softAP(ssid, password);

    // Obtener la dirección IP del Access Point
    IPAddress IP = WiFi.softAPIP();
        Serial.print("Access Point IP Address: ");
        Serial.println(IP);

    }
    void Init() 
    {
        setupAccessPoint(apSSID, apPassword);
    }
};