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
    server.on("/start", [](){
      Engine::StartPrintingAsync();
      server.send(200, "text/plain", "printing...");
    });

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
    <script src="https://cdn.tailwindcss.com"></script>
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.2/codemirror.min.css">
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.2/theme/monokai.min.css">
    <script src="https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.2/codemirror.min.js"></script>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/codemirror/5.65.2/mode/clike/clike.min.js"></script>
    <style>
        body {
            font-family: 'Inter', sans-serif;
        }

        /* Oculta el input de tipo file por defecto */
        input[type="file"] {
            display: none;
        }

        /* Estilos para el scrollbar */
        #console::-webkit-scrollbar {
            width: 8px;
        }

        #console::-webkit-scrollbar-track {
            background: #4b5563;
            border-radius: 10px;
        }

        #console::-webkit-scrollbar-thumb {
            background-color: #d1d5db;
            border-radius: 10px;
            border: 2px solid #4b5563;
        }

        .CodeMirror {
            border: 2px solid #e5e7eb;
            border-radius: 0.5rem;
            height: 320px;
            font-size: 14px;
        }
    </style>
</head>"

<body class="bg-gray-100 flex items-center justify-center min-h-screen p-4">
    <div
        class="bg-white rounded-xl shadow-2xl p-6 md:p-10 w-full max-w-5xl flex flex-col lg:flex-row gap-6 lg:gap-10">

        <!-- Columna de control principal -->
        <div class="flex-1 flex flex-col space-y-6">
            <h1 class="text-3xl md:text-4xl font-bold text-gray-800 text-center">Controlador de Pen Plotter</h1>

            <!-- Estado de la impresora -->
            <div class="bg-gray-200 rounded-lg p-4 shadow-inner text-center">
                <p class="text-lg font-semibold text-gray-600">Estado de la impresora:</p>
                <span id="status" class="text-xl font-bold text-gray-800 mt-1 block">Conectando...</span>
            </div>

            <!-- Carga de G-code -->
            <div class="flex flex-col md:flex-row items-center justify-between gap-4">
                <label for="fileInput"
                    class="flex-1 text-center bg-blue-600 text-white font-medium py-3 px-6 rounded-lg cursor-pointer hover:bg-blue-700 transition duration-300 ease-in-out transform hover:scale-105 shadow-md">
                    Seleccionar archivo G-code
                </label>
                <input type="file" id="fileInput" accept=".gcode,.txt">
            </div>

            <!-- Editor de código para G-code -->
            <textarea id="gCodeEditor" class="hidden"></textarea>
            <div id="gCodeEditorContainer" class="w-full"></div>

            <!-- Botones de acción -->
            <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
                <button id="sendGCodeButton"
                    class="bg-green-600 text-white font-bold py-3 px-6 rounded-lg hover:bg-green-700 transition duration-300 ease-in-out transform hover:scale-105 shadow-md">
                    Enviar G-code
                </button>
                <button id="pauseButton"
                    class="bg-yellow-500 text-gray-800 font-bold py-3 px-6 rounded-lg hover:bg-yellow-600 transition duration-300 ease-in-out transform hover:scale-105 shadow-md">
                    Pausar impresión
                </button>
            </div>
        </div>

        <!-- Columna de consola y control manual -->
        <div class="flex-1 flex flex-col gap-6">
            <!-- Consola -->
            <div class="flex-1 flex flex-col">
                <h2 class="text-xl md:text-2xl font-semibold text-gray-800 mb-2 text-center">Consola</h2>
                <div id="console"
                    class="bg-gray-800 text-white p-4 rounded-lg overflow-y-auto h-64 text-sm flex-1">
                    <!-- Los mensajes de la consola aparecerán aquí -->
                </div>
            </div>

            <!-- Control manual -->
            <div class="bg-gray-100 p-4 rounded-lg shadow-inner">
                <h2 class="text-xl font-semibold text-gray-800 mb-4 text-center">Control Manual</h2>
                <div class="grid grid-cols-3 gap-2 w-48 mx-auto">
                    <div></div>
                    <button class="bg-gray-300 text-gray-800 rounded-full w-10 h-10 flex items-center justify-center shadow-md hover:bg-gray-400" onclick="sendManualCommand('G0 Y10')\">▲</button>
                    <div></div>
                    <button class="bg-gray-300 text-gray-800 rounded-full w-10 h-10 flex items-center justify-center shadow-md hover:bg-gray-400" onclick="sendManualCommand('G0 X-10')\">◄</button>
                    <button class="bg-gray-300 text-gray-800 rounded-full w-10 h-10 flex items-center justify-center shadow-md hover:bg-gray-400" onclick="sendManualCommand('G0 X10 Y10 Z0')\">⌂</button>
                    <button class="bg-gray-300 text-gray-800 rounded-full w-10 h-10 flex items-center justify-center shadow-md hover:bg-gray-400" onclick="sendManualCommand('G0 X10')\">►</button>
                    <div></div>
                    <button class="bg-gray-300 text-gray-800 rounded-full w-10 h-10 flex items-center justify-center shadow-md hover:bg-gray-400" onclick="sendManualCommand('G0 Y-10')\">▼</button>
                    <div></div>
                </div>
                <div class="mt-4 flex flex-row gap-2 justify-center">
                    <button class="bg-purple-500 text-white py-2 px-4 rounded-lg shadow-md hover:bg-purple-600" onclick="sendManualCommand('M3')\">Pluma Abajo</button>
                    <button class="bg-indigo-500 text-white py-2 px-4 rounded-lg shadow-md hover:bg-indigo-600" onclick="sendManualCommand('M5')\">Pluma Arriba</button>
                </div>
            </div>
        </div>
    </div>

    <script>
        // Referencias a los elementos del DOM
        const statusSpan = document.getElementById('status');
        const fileInput = document.getElementById('fileInput');
        const gCodeEditorContainer = document.getElementById('gCodeEditorContainer');
        const gCodeInput = document.getElementById('gCodeEditor');
        const sendGCodeButton = document.getElementById('sendGCodeButton');
        const pauseButton = document.getElementById('pauseButton');
        const consoleDiv = document.getElementById('console');

        // Inicializar CodeMirror
        const editor = CodeMirror.fromTextArea(gCodeInput, {
            lineNumbers: true,
            mode: "clike",
            theme: "monokai",
            matchBrackets: true,
            indentUnit: 4,
            smartIndent: true,
            tabSize: 4,
            indentWithTabs: false,
            autofocus: true
        });

        // Montar el editor en el contenedor
        gCodeEditorContainer.appendChild(editor.getWrapperElement());

        // Función para agregar un mensaje a la consola
        function logToConsole(message) {
            const now = new Date();
            const timestamp = `${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}:${String(now.getSeconds()).padStart(2, '0')}`;
            consoleDiv.innerHTML += `<p class="text-gray-400"><strong>[${timestamp}]</strong> ${message}</p>`;
            consoleDiv.scrollTop = consoleDiv.scrollHeight; // Desplazarse al final
        }

        // Función para actualizar el estado de la impresora desde el ESP32
        function updatePrinterStatus() {
            fetch('/status')
                .then(response => {
                    if (!response.ok) {
                        throw new Error('No se pudo conectar con el ESP32.');
                    }
                    return response.text();
                })
                .then(data => {
                    statusSpan.innerText = data;
                })
                .catch(error => {
                    statusSpan.innerText = 'Desconectado';
                    logToConsole(`Error: ${error.message}`);
                });
        }

        // Cargar el archivo G-code cuando se selecciona uno
        fileInput.addEventListener('change', (event) => {
            const file = event.target.files[0];
            if (file) {
                const reader = new FileReader();
                reader.onload = (e) => {
                    editor.setValue(e.target.result);
                    logToConsole(`Archivo "${file.name}" cargado correctamente.`);
                };
                reader.onerror = () => {
                    logToConsole("Error al leer el archivo.");
                };
                reader.readAsText(file);
            }
        });

        // Enviar G-code al ESP32
        sendGCodeButton.addEventListener('click', () => {
            const gCodeContent = editor.getValue().trim();
            if (gCodeContent.length === 0) {
                logToConsole("El campo de G-code está vacío.");
                return;
            }

            const gCodeLines = gCodeContent.split('\n');
            const filteredGCodeLines = gCodeLines.filter(line => line.trim() !== '');
            const gCodeString = filteredGCodeLines.join(';');

            logToConsole("Enviando G-code...");

            fetch('/sendGcode', {
                method: 'POST',
                headers: {
                    'Content-Type': 'text/plain'
                },
                body: gCodeString,
            })
                .then(response => response.text())
                .then(data => {
                    logToConsole(`Respuesta del ESP32: ${data}`);
                })
                .catch(error => {
                    logToConsole(`Error al enviar G-code: ${error.message}`);
                });
        });

        // Enviar un solo comando G-code manualmente
        function sendManualCommand(command) {
            logToConsole(`Enviando comando manual: ${command}`);
            fetch('/sendGcode', {
                method: 'POST',
                headers: {
                    'Content-Type': 'text/plain'
                },
                body: command + ';',
            })
            .then(response => response.text())
            .then(data => {
                logToConsole(`Respuesta del ESP32: ${data}`);
            })
            .catch(error => {
                logToConsole(`Error al enviar comando manual: ${error.message}`);
            });
        }

        // Pausar la impresión
        pauseButton.addEventListener('click', function () {
            logToConsole("Solicitando pausa...");
            fetch('/pause', {
                method: 'POST'
            })
                .then(response => response.text())
                .then(data => {
                    logToConsole(`Respuesta del ESP32: ${data}`);
                })
                .catch(error => {
                    logToConsole(`Error al pausar: ${error.message}`);
                });
        });

        // Actualizar el estado de la impresora cada 3 segundos
        setInterval(updatePrinterStatus, 3000);

        // Inicializar
        document.addEventListener('DOMContentLoaded', () => {
            updatePrinterStatus();
            logToConsole("Interfaz de Pen Plotter lista. ¡Conectando con ESP32!");
        });
    </script>
</body>

</html>)";
      }
};