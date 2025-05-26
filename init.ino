// Used for precompiling below libraries to bypass compilation issues when other libraries are in use as well, look at README
#include <OneWire.h>
#include <DallasTemperature.h>

void setup() {
  Serial.begin(115200);
  
  Serial.println("Test compilation to cache Arduino.h...");
  }

void loop() {
  }
