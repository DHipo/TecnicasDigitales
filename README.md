# Pen Plotter Project

Este es un proyecto de **Pen Plotter** basado en la plataforma **Platformio**. El proyecto está diseñado para controlar un Pen Plotter utilizando un microcontrolador. Este repositorio incluye el código fuente y las instrucciones necesarias para compilar y ejecutar el proyecto.

## Requisitos

- [Platformio](https://platformio.org/)
- Microcontrolador compatible (ESP32, Arduino, etc.)
- Motores paso a paso y controladores correspondientes
- Pluma o lápiz para el trazado
- Fuente de alimentación adecuada para los motores

## Instalación y Configuración

### 1. Clona el repositorio

Primero, clona el repositorio en tu máquina local:

```bash
git clone https://github.com/DHipo/TecnicasDigitales.git
cd TecnicasDigitales
```

### 2. Instala Platformio

Asegúrate de tener instalado Platformio. Si no lo tienes, sigue estas instrucciones:

- Instala [VS Code](https://code.visualstudio.com/)
- Instala la extensión [Platformio IDE](https://platformio.org/install/ide?install=vscode)

### 3. Abre el proyecto en Platformio

1. Abre **VS Code**.
2. Selecciona "Open Folder" y navega hasta la carpeta del proyecto.
3. Platformio debería reconocer automáticamente el entorno y cargar las dependencias.

### 4. Configura el entorno (opcional)

Si necesitas cambiar las configuraciones del microcontrolador, edita el archivo `platformio.ini`. Por ejemplo, para cambiar la placa utilizada:

```ini
[env:upesy_wroom]
platform = espressif32
board = upesy_wroom
framework = arduino
```

### 5. Compilar y Cargar el código

Conecta tu microcontrolador al puerto USB de tu computadora y ejecuta el siguiente comando para compilar y cargar el código:

```bash
pio run --target upload
```

Este comando compilará el código y lo subirá al microcontrolador.

### 6. Monitor Serie

Para verificar los mensajes enviados por el microcontrolador, puedes abrir el monitor serie con el siguiente comando:

```bash
pio device monitor
```

### 7. Uso del Pen Plotter

Una vez que el código esté cargado en el microcontrolador, puedes empezar a controlar el Pen Plotter. El comportamiento y los movimientos están controlados por el código cargado en el dispositivo. Asegúrate de tener todos los componentes conectados correctamente, incluyendo los motores paso a paso y el mecanismo de trazado.

## Estructura del Proyecto

- **src/**: Contiene el código fuente del proyecto.
- **include/**: Archivos de encabezado (si es necesario).
- **lib/**: Dependencias externas.
- **platformio.ini**: Archivo de configuración para Platformio.

## Contribuciones

Si deseas contribuir a este proyecto, por favor abre un **issue** o crea un **pull request** con tus cambios.

## Licencia

Este proyecto está bajo la licencia MIT. Para más detalles, consulta el archivo LICENSE.