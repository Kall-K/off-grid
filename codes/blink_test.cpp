#include <Arduino.h>

#define LED_PIN 37
#define BUTTON_PIN 0

unsigned long lastBlinkTime = 0;
const unsigned BLINK_INTERVAL = 1000;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);

  // Print startup message
  Serial.println("\n\n");
  Serial.println("╔══════════════════════════════════╗");
  Serial.println("║   LilyGO T3S3 LoRa - Blink Test  ║");
  Serial.println("╚══════════════════════════════════╝");

  // configure gpio pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  digitalWrite(LED_PIN, LOW);

  Serial.println("[SETUP] GPIO pins configured");
  Serial.print("[SETUP] LED pin: ");
  Serial.println(LED_PIN);  // Verified: GPIO 37 (not schematic's 38)
  Serial.print("[SETUP] Button pin: ");
  Serial.println(BUTTON_PIN);
  Serial.println("[SETUP] Ready to blink!\n");

  Serial.print("Board: ");
  Serial.println(ARDUINO_BOARD);
  Serial.print("CPU Frequency: ");
  Serial.print(getCpuFrequencyMhz());
  Serial.println(" MHz");
  Serial.print("Flash Size: ");
  Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
  Serial.println(" MB");
  Serial.print("Free Heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastBlinkTime >= BLINK_INTERVAL) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    lastBlinkTime = currentTime;

    // Print status
    Serial.print("[BLINK] LED is ");
    Serial.println(digitalRead(LED_PIN) ? "ON" : "OFF");
  }

  if (!digitalRead(BUTTON_PIN)) {
    delay(50);
    if (!digitalRead(BUTTON_PIN)) {
      Serial.println("[BUTTON] Pressed!");
      delay(500);
    }
  }

  delay(10);

}
