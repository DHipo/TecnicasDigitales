#include "Network.h"
#include "Engine.h"
#include <Arduino.h>

#define PIN_PULSADOR 21

void setup()
{
  Serial.begin(115200);
  pinMode(PIN_PULSADOR, INPUT);  
  
  // Haciendo referencia al namespace creado
  // llamo a la función init
  Network::Init();
  Engine::initMotores();
  delay(100);

}

void loop()
{
  if (digitalRead(PIN_PULSADOR)){
    delay(1000);
    Engine::StartPrintingAsync();
  }
}
