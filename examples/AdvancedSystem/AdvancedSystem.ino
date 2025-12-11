#include <Arduino.h>
#include "SelfOptScheduler.h"

SelfOptScheduler scheduler(6);

void ledBlink(void* ctx){
  uint8_t pin = (uint8_t)(uintptr_t)ctx;
  digitalWrite(pin, !digitalRead(pin));
}

void heavyWork(void*){
  volatile uint32_t x = 0;
  for (uint16_t i=0;i<4000;i++) x += i;
}

void telemetry(void*){
  scheduler.printStats(Serial);
}

void setup(){
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  // Add tasks: blink on LED, heavyWork, telemetry
  scheduler.addTask(ledBlink, (void*)(uintptr_t)LED_BUILTIN, 100, 20, 1000, 196); // alpha ~0.19
  scheduler.addTask(heavyWork, nullptr, 200, 50, 5000, 128); // slower smoothing
  scheduler.addTask(telemetry, nullptr, 2000, 1000, 60000, 512);
}

void loop(){
  scheduler.run();
}
