/*
 * CrowPanel ESP32 E-Paper 5.79" HMI Display Demo
 * 
 * Demonstrates:
 * 1. Powering up the e-paper panel (GPIO 7).
 * 2. Initializing the 5.79" display (272x792, SSD1683 cascaded driver) using Elecrow driver.
 * 3. Drawing text and shapes.
 * 4. Reading input from menu/exit buttons and the rotary encoder.
 * 
 * Hardware Settings in Arduino IDE:
 * - Board: ESP32S3 Dev Module
 * - PSRAM: OPI PSRAM
 * - Partition Scheme: Huge App (3MB No OTA/1MB SPIFFS)
 * - USB CDC On Boot: Enabled
 */

#include "EPD.h"
#include "crowpanel_pins.h"

// Define the black and white image array as the buffer for the e-paper display
uint8_t ImageBW[27200]; 

// Variables to keep track of button and rotary states
int counter = 0;
bool displayNeedsUpdate = true;
String lastAction = "None";

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("CrowPanel ESP32 E-Paper 5.79-inch Initializing...");

  // 1. MUST set GPIO 7 to HIGH to power up the screen!
  pinMode(EINK_PWR, OUTPUT);
  digitalWrite(EINK_PWR, HIGH);
  delay(150); // Give the power supply a moment to stabilize

  // 2. Initialize GPIOs for buttons and rotary dial (built-in pullups)
  pinMode(BTN_MENU, INPUT_PULLUP);
  pinMode(BTN_EXIT, INPUT_PULLUP);
  pinMode(ROTARY_UP, INPUT_PULLUP);
  pinMode(ROTARY_DOWN, INPUT_PULLUP);
  pinMode(ROTARY_CLICK, INPUT_PULLUP);

  // 3. Initialize display GPIOs using the Elecrow driver
  EPD_GPIOInit();

  // 4. Initialize display buffer and screen
  Paint_NewImage(ImageBW, EPD_W, EPD_H, Rotation, WHITE);
  Paint_Clear(WHITE);
  
  EPD_FastMode1Init(); // Initialize the e-ink screen
  EPD_Display_Clear(); // Clear the screen display
  EPD_Update();        // Update the screen
  EPD_Clear_R26A6H();  // Clear the e-ink screen cache

  Serial.println("Initialization done. Starting main loop.");
}

void loop() {
  // Check physical interface inputs
  checkInputs();

  // If the user triggered an action, update the screen
  if (displayNeedsUpdate) {
    updateDisplay();
    displayNeedsUpdate = false;
  }

  delay(50); // Small delay to avoid contact bounce
}

void checkInputs() {
  // 1. Menu Button
  if (digitalRead(BTN_MENU) == LOW) {
    lastAction = "Menu Button Pressed";
    displayNeedsUpdate = true;
    Serial.println(lastAction);
    delay(200); // Simple debounce
  }

  // 2. Exit Button
  if (digitalRead(BTN_EXIT) == LOW) {
    lastAction = "Exit Button Pressed";
    displayNeedsUpdate = true;
    Serial.println(lastAction);
    delay(200);
  }

  // 3. Rotary Encoder Click
  if (digitalRead(ROTARY_CLICK) == LOW) {
    lastAction = "Rotary Clicked";
    counter = 0; // Reset counter
    displayNeedsUpdate = true;
    Serial.println(lastAction);
    delay(250);
  }

  // 4. Rotary Rotation (Simplistic polling approach)
  if (digitalRead(ROTARY_UP) == LOW) {
    counter++;
    lastAction = "Rotated UP (CW)";
    displayNeedsUpdate = true;
    Serial.print(lastAction);
    Serial.print(" | Counter: ");
    Serial.println(counter);
    delay(150);
  }
  else if (digitalRead(ROTARY_DOWN) == LOW) {
    counter--;
    lastAction = "Rotated DOWN (CCW)";
    displayNeedsUpdate = true;
    Serial.print(lastAction);
    Serial.print(" | Counter: ");
    Serial.println(counter);
    delay(150);
  }
}

// Function to draw the user interface
void updateDisplay() {
  Serial.println("Refreshing screen...");

  // Fill buffer with white
  Paint_Clear(WHITE);

  // Draw header border
  EPD_DrawRectangle(5, 5, 787, 267, BLACK, 0);
  EPD_DrawLine(5, 50, 787, 50, BLACK);

  // Header (Font size 24)
  EPD_ShowString(20, 15, "CrowPanel 5.79\" E-Paper Demo", 24, BLACK);

  // Status Area
  EPD_ShowString(30, 80, "Counter Value : ", 24, BLACK);
  
  // Format counter value
  char countBuf[16];
  snprintf(countBuf, sizeof(countBuf), "%d", counter);
  EPD_ShowString(240, 80, countBuf, 24, BLACK);

  // Last Action
  EPD_ShowString(30, 120, "Last Action   : ", 24, BLACK);
  EPD_ShowString(240, 120, lastAction.c_str(), 24, BLACK);

  // Screen Size
  EPD_ShowString(30, 160, "Screen Size   : 792 x 272 pixels", 24, BLACK);

  // Draw instructions at the bottom (Font size 16)
  EPD_ShowString(30, 220, "Rotate dial to change Counter. Click to reset.", 16, BLACK);

  // Draw some graphic elements
  EPD_DrawCircle(712, 120, 30, BLACK, 0);
  EPD_DrawCircle(712, 120, 15, BLACK, 1);

  // Refresh display
  EPD_Display(ImageBW);
  EPD_PartUpdate();

  Serial.println("Screen refresh complete.");
}
