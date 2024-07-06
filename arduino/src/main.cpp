#include <Arduino.h>

constexpr auto LED_SOCKET = 13;

void setup() {
  pinMode(LED_SOCKET, OUTPUT);
}

void loop() {
  digitalWrite(LED_SOCKET, HIGH);
  delay(100);
  digitalWrite(LED_SOCKET, LOW);
  delay(900);
}
