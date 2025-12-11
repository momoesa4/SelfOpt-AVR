#include <Arduino.h>
#include "SelfOptScheduler.h"

SelfOptScheduler scheduler(4);

void blink(void*){
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void setup(){
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  scheduler.addTask(blink, nullptr, 250);
}

void loop(){
  scheduler.run();
}
