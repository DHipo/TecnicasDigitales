#include "Network.h"
#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
  
  // Haciendo referencia al namespace creado
  // llamo a la función init
  Network::Init();
}

void loop() {
  // put your main code here, to run repeatedly:
  Network::Run();
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}