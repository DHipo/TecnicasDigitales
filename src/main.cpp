#include <Arduino.h>
#include "Engine.h"

void setup() {
  Serial.begin(115200);
  Engine::Init(9600);
  if (!Engine::SelectFile("cuadrado.gcode")) return;
  Engine::StartPrintingAsync();
}

void loop() {
  // put your main code here, to run repeatedly:
}