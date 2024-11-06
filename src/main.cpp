#include "Network.h"
#include "Engine.h"
#include <Arduino.h>

#define PIN_PULSADOR 4

void setup()
{
  Serial.begin(115200);
  pinMode(PIN_PULSADOR, INPUT);  
  
  // Haciendo referencia al namespace creado
  // llamo a la función init
  Network::Init(true);
  Engine::initMotores();
  delay(100);

  attachInterrupt(digitalPinToInterrupt(PIN_PULSADOR), Engine::StartPrintingAsync, HIGH);
}

void loop()
{
  Network::Run();
}
