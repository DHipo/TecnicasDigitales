#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "Engine.h"
#include "SPIFFS.h"

#define SSID "UA-Alumnos"
#define PASSWORD "41umn05WLC"

#define LOG(x) Serial.print(x)

namespace Network
{
  String htmlContent();
  const char *apSSID = "PEN-PLOTER"; // Nombre de la red WiFi para el punto de acceso
  const char *apPassword = "";       // Contraseña de la red WiFi para el punto de acceso

  WebServer server(80);
  RequestHandler request;

  void setupAccessPoint(const char *_ssid, const char *_password)
  {
    WiFi.softAP(_ssid, _password);

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
    static File uploadFile; // Archivo donde se guardará la subida

    if (upload.status == UPLOAD_FILE_START)
    {
      // Construir el nombre completo del archivo con la ruta en SPIFFS
      String filename = "/" + upload.filename;
      Serial.println("Subida iniciada: " + filename);

      // Abrir el archivo en modo escritura
      uploadFile = SPIFFS.open(filename, FILE_WRITE);
      if (!uploadFile)
      {
        Serial.println("Error al abrir el archivo para escritura");
        return;
      }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
      // Mientras se recibe el archivo, se va escribiendo en SPIFFS
      if (uploadFile)
      {
        uploadFile.write(upload.buf, upload.currentSize);
        Serial.print("Escribiendo archivo...");
      }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
      // Cuando termina la subida, cerramos el archivo
      if (uploadFile)
      {
        uploadFile.close();
        Serial.println("Subida finalizada. Archivo guardado.");
      }

      // Responder al cliente que la subida ha sido exitosa
      server.send(200, "text/html", "Archivo subido exitosamente.");
    }
  }

  void handlePause()
  {
    Engine::m_shouldPause = Engine::m_shouldPause;
  }

  // Función para leer un archivo HTML y devolver su contenido como un String
  String readHTMLFile(const char *path)
  {
    // Asegúrate de que SPIFFS esté montado
    if (!SPIFFS.begin(true))
    {
      Serial.println("Error al montar SPIFFS");
      return String();
    }

    // Abrir el archivo
    File file = SPIFFS.open(path, "r");
    if (!file)
    {
      Serial.println("Error al abrir el archivo");
      return String();
    }

    // Leer el contenido del archivo
    String fileContent = file.readString();

    // Cerrar el archivo
    file.close();

    return fileContent; // Devuelve el contenido como String
  }

  void handleSendGcode()
  {
    int contentLength = server.client().available();
    Serial.print("Content Length: ");
    Serial.println(contentLength);

    if (contentLength == 0)
    {
      Serial.println("No body to extract");
      return;
    }

    String gCodeContent;
    while (server.client().available())
    {
      gCodeContent += (char)server.client().read();
    }

    Serial.print("Contenido del G-code recibido: ");
    Serial.println(gCodeContent);

    // Procesar el G-code y separarlo en líneas
    std::vector<String> gCodeLines;
    int pos = 0;
    while ((pos = gCodeContent.indexOf(';')) != -1)
    {
      String line = gCodeContent.substring(0, pos);
      Serial.println(line);
      gCodeLines.push_back(line);
      gCodeContent = gCodeContent.substring(pos + 1);
    }

    // Añadir la última línea si existe
    if (gCodeContent.length() > 0)
    {
      gCodeLines.push_back(gCodeContent);
    }

    Serial.printf("size of gcodelines: %d", gCodeLines.size());
    Engine::setCurrentGcode(gCodeLines);
    server.send(200, "text/plain", "data received");
  }

  void responseUploadFile()
  {
    Serial.print("Uploading File...");
    server.send(200, "text/plain", "{data: \"ok\"}");
  }

  void listFiles()
  {
    String response = "<html><body><h1>Archivos en SPIFFS</h1><ul>";

    File root = SPIFFS.open("/");
    if (!root)
    {
      response += "<p>Error al abrir SPIFFS.</p>";
      response += "</ul></body></html>";
      server.send(500, "text/html", response);
      return;
    }

    File file = root.openNextFile();
    while (file)
    {
      response += "<li>" + String(file.name()) + " (" + String(file.size()) + " bytes)</li>";
      file = root.openNextFile();
    }

    response += "</ul></body></html>";
    server.send(200, "text/html", response);
  }

  void Run()
  {
    server.handleClient();
  }

  void ConnectToWiFi(const char *_ssid, const char *_password)
  {
    WiFi.begin(_ssid, _password);

    while (WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      Serial.print("Connecting to WiFi...");
    }
  }

  void Init(const bool _wifi = false, const bool _ap = true)
  {
    // Inicializar SPIFFS
    if (!SPIFFS.begin(true))
    {
      Serial.println("Error al inicializar SPIFFS");
      return;
    }
    Serial.println("SPIFFS montado correctamente");

    WiFi.begin();

    if (_wifi)
      ConnectToWiFi(SSID, PASSWORD);

    Serial.println(WiFi.localIP());

    server.onFileUpload(handleUpload);
    server.on("/", HTTP_GET, []()
              { server.send(200, "text/html", htmlContent()); });

    server.on("/status", HTTP_GET, handleStatus);

    server.on("/upload", HTTP_POST, responseUploadFile, handleUpload);

    server.on("/pause", HTTP_POST, handlePause);

    server.on("/list", HTTP_GET, listFiles);
    server.on("/sendGcode", HTTP_POST, []()
              { Serial.printf("args: %d", server.args()); server.send(200, "text/plain", "recibido"); }, handleSendGcode);

    server.begin();

    setupAccessPoint(apSSID, apPassword);
  }

  String htmlContent()
  {
    return R"(<!DOCTYPE html>
<html lang="es">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Controlador de Pen Plotter</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
        }

        #container {
            max-width: 800px;
            margin: 0 auto;
            background-color: #f0f0f0;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
        }

        button {
            margin: 10px;
            padding: 10px 20px;
            font-size: 16px;
            cursor: pointer;
            transition: background-color 0.3s ease;
        }

        button:hover {
            background-color: #ddd;
        }

        input[type="file"] {
            display: none;
        }

        label {
            display: inline-block;
            padding: 10px 20px;
            background-color: #007bff;
            color: white;
            border-radius: 5px;
            cursor: pointer;
        }

        label:hover {
            background-color: #0056b3;
        }

        #printerStatus {
            margin-top: 20px;
            font-weight: bold;
            color: #00698f;
        }

        #console {
            margin-top: 20px;
            background-color: #333;
            color: white;
            padding: 10px;
            border-radius: 5px;
            overflow-y: scroll;
            height: 150px;
        }

        #status {
            font-size: larger;
        }
    </style>
</head>

<body>
    <div id="container">
        <h1>Controlador de Pen Plotter</h1>
        <span id="status"></span>
        <div id="gCodeContainer">
            <textarea id="gCodeInput" rows="10" cols="50"></textarea>
            <button id="sendGCodeButton">Enviar G-code</button>
        </div>

        <button id="pauseButton">Pausar impresión</button>

        <div id="console"></div>
    </div>

    <script>
        const printerStatusDiv = document.getElementById('printerStatus');
        const pauseButton = document.getElementById('pauseButton');
        const consoleDiv = document.getElementById('console');
        const spanStatus = document.getElementById('status');

        // Función para actualizar el estado de la impresora
        function updatePrinterStatus() {
            fetch('/status')
                .then(response => response.text())
                .then(data => {
                    spanStatus.innerText = data;
                })
                .catch(error => console.error('Error:', error));
        }

        // Actualizar el estado cada 5 segundos
        setInterval(updatePrinterStatus, 5000);

        const gCodeInput = document.getElementById('gCodeInput');
        const sendGCodeButton = document.getElementById('sendGCodeButton');

        sendGCodeButton.addEventListener('click', () => {
            const gCodeContent = gCodeInput.value.trim();

            // Separar el G-code en líneas
            const gCodeLines = gCodeContent.split('\n');

            // Construir la cadena de texto para enviar al servidor
            const gCodeString = gCodeLines.join(';');

            fetch('/sendGcode', {
                method: 'POST',
                body: gCodeString,
            })
                .then(response => response.text())
                .then(data => {
                    consoleDiv.innerText += '\nG-code enviado: ' + gCodeString;
                })
                .catch(error => console.error('Error:', error));
        });


        // Manejar el botón de pausa
        pauseButton.addEventListener('click', function () {
            fetch('/pause', { method: 'POST', body: '' })
                .then(response => response.text())
                .then(data => {
                    consoleDiv.innerText += '\n' + data;
                })
                .catch(error => console.error('Error:', error));
        });
    </script>
</body>

</html>)";
  }
};