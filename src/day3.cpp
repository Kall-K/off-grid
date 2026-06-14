#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RadioLib.h>

// ============= Device ID =============
#define ID 1
// ============= Pin Definitions =============
#define LED_PIN 37
#define BUTTON_PIN 0
#define OLED_SDA 18
#define OLED_SCL 17
// ============= HARDWARE SPI PINS =============
#define LORA_SCK 5
#define LORA_MISO 3
#define LORA_MOSI 6
// ============= SX1262 CHIP PINS =============
#define LORA_CS 7       // NSS (Chip Select)
#define LORA_RST 8      // Reset Pin
#define LORA_BUSY 34    // SX1262 Busy Tracking Pin
#define LORA_DIO1 33    // SX1262 Interrupt Pin
#define BATTERY_ADC_PIN 1
// ============= Display Setup =============
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ============= LoRa Setup =============
// Create SPI object
SPIClass spi(FSPI);

// Create SX1262 object using RadioLib
SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY, spi);

// ============= Global Variables =============
unsigned long lastBlinkTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastPacketTransmitTime = 0;
unsigned long bootTime = 0;

const unsigned long BLINK_INTERVAL = 500;
const unsigned long DISPLAY_INTERVAL = 1000;
const unsigned long PACKET_INTERVAL = 15000;

// Statistics
int buttonPresses = 0;
int packetsTransmitted = 0;
int packetsReceived = 0;
float lastRSSI = 0;
float lastSNR = 0;
int loraState = 0;
volatile bool receivedFlag = false;
float batteryVoltage = 0.0;
int batteryPercent = 0;

// LoRa settings
const float FREQUENCY = 868.0;      // 868 MHz (EU ISM band)
const uint8_t SPREADING_FACTOR = 9; // SF9 (medium range)
const float BANDWIDTH = 125.0;      // 125 kHz
const uint8_t CODING_RATE = 7;      // 4/7
const int8_t TX_POWER = 10;         // 10 dBm (safe, ~100 mW)
const float BATTERY_MIN_V = 3.2;
const float BATTERY_MAX_V = 4.2;

// struct LoRaPacket {
//     uint16_t packetId;
//     uint32_t timestamp;
//     float temperature;
//     float humidity;
//     uint8_t batteryLevel;
// };

void handleButton();
void transmitLoRaPacket();
void handleLoRaReceive();
void updateDisplay();

void onLoRaPacketReceived() {
    receivedFlag = true;
}

float readBatteryVoltage() {
    return (analogReadMilliVolts(BATTERY_ADC_PIN) * 2.0) / 1000.0;
}

int batteryVoltageToPercent(float voltage) {
    float percent = (voltage - BATTERY_MIN_V) * 100.0 / (BATTERY_MAX_V - BATTERY_MIN_V);
    return constrain((int)(percent + 0.5), 0, 100);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    bootTime = millis();

    Serial.println("\n\n╔════════════════════════════════════════╗");
    Serial.println("║   Day 3: SX1262 LoRa TX/RX             ║");
    Serial.println("╚════════════════════════════════════════╝\n");

    // GPIO Setup
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);
    pinMode(BATTERY_ADC_PIN, INPUT);
    digitalWrite(LED_PIN, LOW);
    analogReadResolution(12);
    analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
    Serial.println("[GPIO] Pins configured");

    // I2C Setup (for OLED)
    Serial.println("[I2C] Initializing...");
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(400000);

    // OLED Setup
    Serial.println("[OLED] Initializing SSD1306...");
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[ERROR] Display failed!");
        while (1);
    }
    Serial.println("[OLED] Ready!");

    // SPI Setup (for SX1262)
    Serial.println("[SPI] Initializing SPI bus...");
    spi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

    // SX1262 Setup
    Serial.println("[LORA] Initializing SX1262...");
    loraState = radio.begin();

    if (loraState == RADIOLIB_ERR_NONE) {
        Serial.println("[LORA] SX1262 initialized successfully!");

        // Configure LoRa parameters
        Serial.println("[LORA] Configuring parameters...");

        // Set frequency (868 MHz for EU)
        radio.setFrequency(FREQUENCY);

        // Set spreading factor
        radio.setSpreadingFactor(SPREADING_FACTOR);

        // Set bandwidth
        radio.setBandwidth(BANDWIDTH);

        // Set coding rate
        radio.setCodingRate(CODING_RATE);

        // Set TX power
        radio.setOutputPower(TX_POWER);

        // Set preamble length
        radio.setPreambleLength(8);

        radio.setPacketReceivedAction(onLoRaPacketReceived);

        // Put in RX mode (listen for packets)
        radio.startReceive();
        Serial.println("[LORA] Ready - listening for packets!");

    } else {
        Serial.print("[ERROR] SX1262 init failed: ");
        Serial.println(loraState);
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("ERROR:");
        display.println("SX1262 Init Failed!");
        display.print("Code: ");
        display.println(loraState);
        display.display();
        while (1);  // Halt
    }

    // Startup screen
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("LilyGO T3S3 LoRa");
    display.println("Day 3: TX/RX Test");
    display.println("");
    display.println("SX1262: OK");
    display.println("868 MHz");
    display.println("Ready!");
    display.display();

    Serial.println("[SETUP] Ready!\n");
    delay(2000);
}

void loop() {
    unsigned long now = millis();
  
    if (ID == 0) {
        // Button press
        handleButton(); 
    } else if (ID == 1) {
        // Transmit packet every 15 seconds
        if (now - lastPacketTransmitTime >= PACKET_INTERVAL) {
            lastPacketTransmitTime = now;
            transmitLoRaPacket();
        }        
    }
    
    // Check for received packets
    handleLoRaReceive();

    // Update display
    if (now - lastDisplayTime >= DISPLAY_INTERVAL) {
        lastDisplayTime = now;
        updateDisplay();
    }

    delay(10);
}

void handleButton() {
    static unsigned long lastCheck = 0;

    if (millis() - lastCheck < 100) return;
    lastCheck = millis();

    if (digitalRead(BUTTON_PIN) == LOW) {
        delay(50);
        if (digitalRead(BUTTON_PIN) == LOW) {
            buttonPresses++;
            transmitLoRaPacket();
            
            while (digitalRead(BUTTON_PIN) == LOW) {
                delay(10);
            }
            delay(50);
        }
    }
}

void transmitLoRaPacket() {
    Serial.println("[LORA] Transmitting packet...");

    // Create message with packet number
    char message[32];
    snprintf(message, sizeof(message), "Hello %d", packetsTransmitted + 1);

    // Transmit
    int state = radio.transmit(message);

    if (state == RADIOLIB_ERR_NONE) {
        packetsTransmitted++;
        Serial.print("[LORA] TX OK - Message: ");
        Serial.println(message);

        // Flash LED
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
    } else {
        Serial.print("[ERROR] TX failed: ");
        Serial.println(state);
    }

    // Return to RX mode
    radio.startReceive();
}

void handleLoRaReceive() {
    if (!receivedFlag) {
        return;
    }

    receivedFlag = false;

    String payload;
    int state = radio.readData(payload);

    if (state == RADIOLIB_ERR_NONE) {
        // Packet received!
        packetsReceived++;

        // Get signal quality
        lastRSSI = radio.getRSSI();
        lastSNR = radio.getSNR();

        Serial.print("[LORA] RX OK - ");
        Serial.print("Message: '");
        Serial.print(payload);
        Serial.print("' | RSSI: ");
        Serial.print(lastRSSI);
        Serial.print(" dBm | SNR: ");
        Serial.print(lastSNR);
        Serial.println(" dB");

        // Flash LED twice
        digitalWrite(LED_PIN, HIGH);
        delay(50);
        digitalWrite(LED_PIN, LOW);
        delay(50);
        digitalWrite(LED_PIN, HIGH);
        delay(50);
        digitalWrite(LED_PIN, LOW);

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);

        // display received message and signal quality
        display.print("Message: '");
        display.print(payload);
        display.println("'");
        display.print("RSSI: ");
        display.print(lastRSSI, 0);
        display.println(" dBm");
        display.print("SNR: ");
        display.print(lastSNR, 1);
        display.println(" dB");
        display.display();
        delay(1000);
    } else {
        Serial.print("[ERROR] RX failed: ");
        Serial.println(state);
    }

    // Return to RX mode
    radio.startReceive();
}

void updateDisplay() {
    batteryVoltage = readBatteryVoltage();
    batteryPercent = batteryVoltageToPercent(batteryVoltage);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    // Header
    display.println("=== LoRa Status ===");

    // TX/RX counts
    display.print("TX: ");
    display.print(packetsTransmitted);
    display.print(" | RX: ");
    display.println(packetsReceived);

    // Signal quality
    display.print("RSSI: ");
    display.print(lastRSSI, 0);
    display.println(" dBm");

    display.print("SNR: ");
    display.print(lastSNR, 1);
    display.println(" dB");

    display.print("BAT: ");
    display.print(batteryVoltage, 2);
    display.print("V ");
    display.print(batteryPercent);
    display.println("%");

    // Uptime
    display.print("Uptime: ");
    display.print((millis() - bootTime) / 1000);
    display.println("s");

    // Button hint
    display.println("Press BOOT to TX");

    display.display();
}
