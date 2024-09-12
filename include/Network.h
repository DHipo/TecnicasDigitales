#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

namespace Network
{

  const char *apSSID = "PEN-PLOTER";    // Nombre de la red WiFi para el punto de acceso
  const char *apPassword = "123456789"; // Contraseña de la red WiFi para el punto de acceso

  const char *ssid = "TU_SSID";
  const char *password = "TU_PASSWORD";

  WebServer server(80);

  void setupAccessPoint(const char *ssid, const char *password)
  {
    WiFi.softAP(ssid, password);

    // Obtener la dirección IP del Access Point
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Access Point IP Address: ");
    Serial.println(IP);
  }

  void handleStatus()
  {
    String status = "Impresora en línea"; // Aquí deberías obtener el estado real de la impresora
    server.send(200, "text/html", status);
  }

  void handleUpload()
  {
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START)
    {
      Serial.println("Subida iniciada");
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
      Serial.print("Escribiendo archivo...");
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
      Serial.println("Subida finalizada");
      server.send(200);
    }
  }

  void handlePause()
  {
    // Aquí implementarías la lógica para pausar o reanudar la impresión
    String message = "Impresión pausada"; // O "Impresión reanudada"
    server.send(200, "text/plain", message);
  }

  void Run()
  {
    server.handleClient();
  }

  void Init()
  {
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      Serial.println("Connecting to WiFi...");
    }
    Serial.println(WiFi.localIP());

    server.on("/status", HTTP_GET, handleStatus);
    server.on("/upload", HTTP_POST, []()
              { server.send(200); }, handleUpload);
    server.on("/pause", HTTP_GET, handlePause);

    server.begin();
    setupAccessPoint(apSSID, apPassword);
  }

};