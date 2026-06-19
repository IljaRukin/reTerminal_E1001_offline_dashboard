
#include <Arduino.h>
#include "config.h"
#include "led.h"

void LED::init() {
  pinMode(LED_PIN, OUTPUT);
  m_initialized = true;
}

void LED::ledOn() {
  if (!m_initialized) {
    return;
  }
  digitalWrite(LED_PIN, LOW);
}

void LED::ledOff() {
  if (!m_initialized) {
    return;
  }
  digitalWrite(LED_PIN, HIGH);
}
