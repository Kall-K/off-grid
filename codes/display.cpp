#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pin definitions
#define LED 37
#define BUTTON 0
#define SDA 18
#define SCL 17

// OLED Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

// Create OLED display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Global variables
unsigned long lastBlinkTime = 0;
unsigned long lastUpdateTime = 0;
unsigned long bootTime = 0;
const unsigned long BLINK_INTERVAL = 1000;
const unsigned long UPDATE_INTERVAL = 500;
const unsigned long DOUBLE_PRESS_INTERVAL = 800; // ms
int buttonPressCount = 0;
bool ledState = false;
enum DisplayPage {
    PAGE_MAIN = 0,
    PAGE_MEMORY = 1,
    PAGE_SYSTEM = 2
};
DisplayPage currentPage = PAGE_MAIN;

void checkButton();
void updateDisplay();
void displayMainPage();
void displayMemoryPage();
void displaySystemPage();


void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LED, OUTPUT);
    pinMode(BUTTON, INPUT);
    digitalWrite(LED, LOW);

    // Initialize I2C bus
    Serial.println("[I2C] Initializing I2C bus (SDA=18, SCL=17)...");
    Wire.begin(SDA, SCL);
    Wire.setClock(100000); //100 kHz

    Serial.println("[OLED] Initializing SSD1306 display...");
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[ERROR] SSD1306 allocation failed!");
        while (1); // halt is display fails
    }

    Serial.println("[OLED] Display initialized successfully!");

    // clear display and set text color
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);

    // display startup message
    display.println("LilyGO T3S3 LoRa");
    display.println("Day 2: OLED Test");
    display.println("");
    display.println("Initializing...");
    display.display();

    Serial.println("[SETUP] All systems ready!\n");
    delay(2000); // show startup message for 2 seconds
}

void loop() {
    // check button
    Serial.println(buttonPressCount);
    checkButton();

    unsigned long currentTime = millis();
    
    // Serial.println("stage1");
    if (buttonPressCount == 1) {
        // change display page
        currentPage = (DisplayPage)((currentPage + 1) % 3);
        buttonPressCount = 0; // reset count after changing page
    } else if (buttonPressCount == 2) {
        ledState = !ledState;
        buttonPressCount = 0; // reset count after toggling LED
    }

    // Serial.println("stage2");
    // handle LED blinking
    if (ledState) {
        // update led (every 1 second)
        if (currentTime - lastBlinkTime >= BLINK_INTERVAL) {
            lastBlinkTime = currentTime;
            digitalWrite(LED, !digitalRead(LED));
        }
    } else {
        digitalWrite(LED, LOW);
    }
    
    // Serial.println("stage3");
    // update display every 500ms
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
        lastUpdateTime = currentTime;
        updateDisplay();
    }

    // Serial.println("end");
    delay(10);
}

void checkButton() {
    static unsigned long lastCheck = 0; // here static is necessary to keep track of time between checks

    if (millis() - lastCheck < 100) return;
    lastCheck = millis();
    // Serial.println("Checking button..." + String(millis()));
    if (digitalRead(BUTTON) == LOW) {
        delay(50);
        if (digitalRead(BUTTON) == LOW) {
            while (digitalRead(BUTTON) == LOW) {
                delay(10);
            }
            delay(50);
            buttonPressCount++;
            // Serial.println("first press");

            // wait if second time the button is pressed within the double press interval
            while (millis() - lastCheck < DOUBLE_PRESS_INTERVAL) {
                if (digitalRead(BUTTON) == LOW) {
                    delay(50);
                    if (digitalRead(BUTTON) == LOW) {
                        while (digitalRead(BUTTON) == LOW) {
                            delay(10);
                        }
                        delay(50);
                        buttonPressCount++;
                        break;
                    }
                }
            }
            Serial.print("[BUTTON] Presses! Count: ");
            Serial.println(buttonPressCount);
        }
    }
    // Serial.println("Button check complete." + String(millis()));
}

void updateDisplay() {
    switch (currentPage) {
        case PAGE_MAIN:
            displayMainPage();
            break;
        case PAGE_MEMORY:
            displayMemoryPage();
            break;
        case PAGE_SYSTEM:
            displaySystemPage();
            break;
    }
}

void displaySystemPage() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    // Header
    display.println("=== SYSTEM INFO ===");
    display.println("");

    // CPU
    display.print("CPU: ");
    display.print(getCpuFrequencyMhz());
    display.println(" MHz");
    // Temperature
    display.print("Temp: ");
    display.print(temperatureRead(), 1);
    display.println(" C");
    
    // Refresh
    display.display();
}

void displayMemoryPage() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    // Header
    display.println("=== MEMORY INFO ===");
    display.println("");    
    // Memory
    display.print("Free Heap: ");
    display.print(ESP.getFreeHeap() / 1024);
    display.println(" KB");
    
    display.print("Max Alloc: ");
    display.print(ESP.getMaxAllocHeap() / 1024);
    display.println(" KB");
    
    // Refresh
    display.display();
}

void displayMainPage() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    // Header
    display.println("=== MAIN PAGE ===");
    display.println("");

    // Uptime
    unsigned long uptime = (millis() - bootTime) / 1000;
    display.print("Uptime: ");
    display.print(uptime);
    display.println(" s");
    // LED State
    display.print("LED: ");
    display.println(ledState ? "ON" : "OFF");

    // Refresh
    display.display();
}