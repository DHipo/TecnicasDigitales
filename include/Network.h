#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "SPIFFS.h"

#define SSID "UA-Alumnos"
#define PASSWORD "41umn05WLC"

#define LOG(x) Serial.print(x)

namespace Network
{

  const char *apSSID = "PEN-PLOTER"; // Nombre de la red WiFi para el punto de acceso
  const char *apPassword = "";       // Contraseña de la red WiFi para el punto de acceso

  WebServer server(80);

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
      server.send(200, "text/plain", "Archivo subido exitosamente.");
    }
  }

  void handlePause()
  {
    // Aquí implementarías la lógica para pausar o reanudar la impresión
    String message = "Impresión pausada"; // O "Impresión reanudada"
    server.send(200, "text/plain", message);
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

  String htmlContent()
  {
    return R"(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Controlador de Pen Plotter</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }
        #container { max-width: 800px; margin: 0 auto; background-color: #f0f0f0; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
        button { margin: 10px; padding: 10px 20px; font-size: 16px; cursor: pointer; transition: background-color 0.3s ease; }
        button:hover { background-color: #ddd; }
        input[type="file"] { display: none; }
        label { display: inline-block; padding: 10px 20px; background-color: #007bff; color: white; border-radius: 5px; cursor: pointer; }
        label:hover { background-color: #0056b3; }
        #printerStatus { margin-top: 20px; font-weight: bold; color: #00698f; }
        #console { margin-top: 20px; background-color: #333; color: white; padding: 10px; border-radius: 5px; overflow-y: scroll; height: 150px; }
    </style>
</head>
<body>
    <div id="container">
        <h1>Controlador de Pen Plotter</h1>
        
        <form id="uploadForm">
            <label for="fileInput">Subir archivo</label>
            <input type="file" id="fileInput" accept=".svg,.png">
        </form>
        
        <div id="printerStatus"></div>
        
        <button id="pauseButton">Pausar impresión</button>
        
        <div id="console"></div>
    </div>

    <script>
        const form = document.getElementById('uploadForm');
        const fileInput = document.getElementById('fileInput');
        const printerStatusDiv = document.getElementById('printerStatus');
        const pauseButton = document.getElementById('pauseButton');
        const consoleDiv = document.getElementById('console');

        // Función para actualizar el estado de la impresora
        function updatePrinterStatus() {
            fetch('/status')
                .then(response => response.text())
                .then(data => {
                    printerStatusDiv.innerHTML = data;
                })
                .catch(error => console.error('Error:', error));
        }

        // Actualizar el estado cada 5 segundos
        setInterval(updatePrinxterStatus, 5000);

        // Manejar la subida de archivos
        form.addEventListener('submit', function(e){
            e.preventDefault();
            const formData = new FormData(this);
            fetch('./upload', {
                method: 'POST',
                body: formData
            })
            .then(response => response.text())
            .then(data => {
                consoleDiv.innerText += '\nArchivo subido: ' + data;
            })
            .catch(error => console.error('Error:', error));
        });

        // Manejar el botón de pausa
        pauseButton.addEventListener('click', function(){
            fetch('/pause')
                .then(response => response.text())
                .then(data => {
                    consoleDiv.innerText += '\n' + data;
                })
                .catch(error => console.error('Error:', error));
        });
    </script>
</body>
</html>
)";
  }

  void responseUploadFile()
  {
    Serial.print("Uploading File...");
    server.send(200, "text/json", "{data: \"ok\"}");
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
    WiFi.begin();

    if (_wifi)
      ConnectToWiFi(SSID, PASSWORD);

    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, []()
              { server.send(200, "text/html", htmlContent()); });

    server.on("/status", HTTP_GET, handleStatus);

    server.on("/upload", HTTP_POST, responseUploadFile, handleUpload);

    server.on("/pause", HTTP_GET, handlePause);

    server.begin();

    setupAccessPoint(apSSID, apPassword);
  }

};