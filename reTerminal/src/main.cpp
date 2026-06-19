#include <Arduino.h>

#define LED_PIN 6
#define SERIAL_RX 44
#define SERIAL_TX 43

void setup() {
  Serial1.begin(9600, SERIAL_8N1, SERIAL_RX, SERIAL_TX);
  while (!Serial1) {
    delay(10); // Wait for serial port to connect
  }
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, LOW);
  Serial1.println("LED ON");
  delay(1000);

  digitalWrite(LED_PIN, HIGH);
  Serial1.println("LED OFF");
  delay(1000);
}
