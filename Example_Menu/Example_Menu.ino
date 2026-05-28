/*
 * Example: Interactive Menu System
 * Board: CrowPanel ESP32 E-Paper 5.79"
 * 
 * Demonstrates:
 * - A clean multi-page menu system
 * - Navigation using the rotary dial (rotate up/down to move selection)
 * - Confirmation using Rotary Click
 * - Going back using the Exit Button
 */

#include "EPD.h"
#include "crowpanel_pins.h"

uint8_t ImageBW[27200];

// Menu State variables
int currentMenuPage = 0; // 0 = Main Menu, 1 = Submenu Settings, 2 = Submenu Sensors, 3 = Submenu Info
int selectedOption = 0;
const int MAIN_MENU_MAX_OPTIONS = 3;
const char* mainMenuItems[] = {
  "1. Hardware Settings",
  "2. Environmental Sensors",
  "3. System & Device Info"
};

bool displayNeedsUpdate = true;
String statusMsg = "Ready";

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Menu System Initializing...");

  pinMode(EINK_PWR, OUTPUT);
  digitalWrite(EINK_PWR, HIGH);
  delay(150);

  pinMode(BTN_MENU, INPUT_PULLUP);
  pinMode(BTN_EXIT, INPUT_PULLUP);
  pinMode(ROTARY_UP, INPUT_PULLUP);
  pinMode(ROTARY_DOWN, INPUT_PULLUP);
  pinMode(ROTARY_CLICK, INPUT_PULLUP);

  EPD_GPIOInit();

  Paint_NewImage(ImageBW, EPD_W, EPD_H, Rotation, WHITE);
  Paint_Clear(WHITE);
  
  EPD_FastMode1Init();
  EPD_Display_Clear();
  EPD_Update();
  EPD_Clear_R26A6H();

  Serial.println("Menu System ready.");
}

void loop() {
  checkInputs();

  if (displayNeedsUpdate) {
    updateDisplay();
    displayNeedsUpdate = false;
  }
  delay(50);
}

void checkInputs() {
  // Exit Button goes back to Main Menu
  if (digitalRead(BTN_EXIT) == LOW) {
    if (currentMenuPage != 0) {
      currentMenuPage = 0;
      selectedOption = 0;
      statusMsg = "Returned to Main";
      displayNeedsUpdate = true;
    }
    delay(250);
  }

  // Rotary Click enters submenus
  if (digitalRead(ROTARY_CLICK) == LOW) {
    if (currentMenuPage == 0) {
      currentMenuPage = selectedOption + 1; // Enter Submenus 1, 2, or 3
      selectedOption = 0;
      statusMsg = "Entered Submenu";
    } else {
      statusMsg = "Action Triggered";
    }
    displayNeedsUpdate = true;
    delay(250);
  }

  // Rotary Up/Down changes selection
  if (digitalRead(ROTARY_UP) == LOW) {
    if (currentMenuPage == 0) {
      selectedOption = (selectedOption - 1 + MAIN_MENU_MAX_OPTIONS) % MAIN_MENU_MAX_OPTIONS;
    } else {
      selectedOption++;
    }
    statusMsg = "Scrolled Up";
    displayNeedsUpdate = true;
    delay(200);
  }
  else if (digitalRead(ROTARY_DOWN) == LOW) {
    if (currentMenuPage == 0) {
      selectedOption = (selectedOption + 1) % MAIN_MENU_MAX_OPTIONS;
    } else {
      selectedOption--;
    }
    statusMsg = "Scrolled Down";
    displayNeedsUpdate = true;
    delay(200);
  }
}

void updateDisplay() {
  Serial.println("Refreshing Menu...");
  Paint_Clear(WHITE);

  // Outer Border
  EPD_DrawRectangle(5, 5, 787, 267, BLACK, 0);
  
  // Header Area
  EPD_DrawLine(5, 55, 787, 55, BLACK);
  
  if (currentMenuPage == 0) {
    // ==========================================
    // MAIN MENU VIEW
    // ==========================================
    EPD_ShowString(20, 15, "MAIN MENU - CHOOSE OPTION", 24, BLACK);
    EPD_ShowString(550, 18, "Status: Active", 16, BLACK);

    // List Options
    for (int i = 0; i < MAIN_MENU_MAX_OPTIONS; i++) {
      int yPos = 80 + (i * 45);
      if (selectedOption == i) {
        // Draw selection cursor block (Inverted appearance)
        EPD_DrawRectangle(20, yPos - 5, 500, yPos + 30, BLACK, 1);
        EPD_ShowString(30, yPos, mainMenuItems[i], 24, WHITE);
      } else {
        EPD_ShowString(30, yPos, mainMenuItems[i], 24, BLACK);
      }
    }

    // Navigation Hint Box
    EPD_DrawRectangle(530, 80, 770, 240, BLACK, 0);
    EPD_ShowString(545, 95, "NAVIGATION:", 16, BLACK);
    EPD_ShowString(545, 125, "- Rotate Dial: Select", 16, BLACK);
    EPD_ShowString(545, 155, "- Press Click: Enter", 16, BLACK);
    EPD_ShowString(545, 185, "- Press Exit : Back", 16, BLACK);
    EPD_ShowString(545, 210, "[SSD1683 Cascaded]", 12, BLACK);

  } 
  else if (currentMenuPage == 1) {
    // ==========================================
    // SUBMENU 1: HARDWARE SETTINGS
    // ==========================================
    EPD_ShowString(20, 15, "1. HARDWARE SETTINGS", 24, BLACK);
    EPD_ShowString(600, 18, "[Exit] Back", 16, BLACK);

    EPD_ShowString(30, 80, "Contrast Level   :  Normal (60%)", 16, BLACK);
    EPD_ShowString(30, 110, "Fast Refresh Mode:  [ ENABLED ]", 16, BLACK);
    EPD_ShowString(30, 140, "Deep Sleep Timer :  10 Minutes", 16, BLACK);
    EPD_ShowString(30, 170, "Active Core Debug:  None", 16, BLACK);
    EPD_ShowString(30, 200, "PSRAM Heap status:  OPI Enabled (8MB)", 16, BLACK);
    
    EPD_ShowString(30, 235, "Click rotary dial to toggle selected items.", 12, BLACK);
  } 
  else if (currentMenuPage == 2) {
    // ==========================================
    // SUBMENU 2: SENSORS DASHBOARD
    // ==========================================
    EPD_ShowString(20, 15, "2. ENVIRONMENTAL SENSORS", 24, BLACK);
    EPD_ShowString(600, 18, "[Exit] Back", 16, BLACK);

    // Left block
    EPD_ShowString(30, 80, "SGP40 Gas VOC : 105 index", 16, BLACK);
    EPD_ShowString(30, 115, "AHT20 Temp    : 28.4 C", 16, BLACK);
    EPD_ShowString(30, 150, "AHT20 Humidity: 64.2 % RH", 16, BLACK);
    EPD_ShowString(30, 185, "BMP280 Press  : 1013.25 hPa", 16, BLACK);

    // Right block graphics (Progress Bar)
    EPD_DrawRectangle(450, 80, 750, 100, BLACK, 0);
    EPD_ShowString(450, 60, "VOC Index Status:", 12, BLACK);
    EPD_DrawRectangle(452, 82, 600, 98, BLACK, 1); // Mock 50% fill

    EPD_DrawRectangle(450, 140, 750, 160, BLACK, 0);
    EPD_ShowString(450, 120, "Humidity Status:", 12, BLACK);
    EPD_DrawRectangle(452, 142, 644, 158, BLACK, 1); // Mock 64% fill

    EPD_ShowString(30, 230, "System reads sensors every 2.5 seconds dynamically.", 12, BLACK);
  } 
  else if (currentMenuPage == 3) {
    // ==========================================
    // SUBMENU 3: SYSTEM INFO
    // ==========================================
    EPD_ShowString(20, 15, "3. SYSTEM & DEVICE INFO", 24, BLACK);
    EPD_ShowString(600, 18, "[Exit] Back", 16, BLACK);

    EPD_ShowString(30, 80, "Product Model: CrowPanel ESP32 E-Paper HMI", 16, BLACK);
    EPD_ShowString(30, 110, "Display Spec : 5.79-inch (792 x 272 Pixels)", 16, BLACK);
    EPD_ShowString(30, 140, "Controller   : ESP32-S3-WROOM-1-N8R8 (AP_3V3)", 16, BLACK);
    EPD_ShowString(30, 170, "Firmware Ver : v1.0.4-Build2026", 16, BLACK);
    EPD_ShowString(30, 200, "MAC Address  : D8:3B:DA:43:BB:D8", 16, BLACK);
    EPD_ShowString(30, 230, "Elecrow Wiki : CrowPanel_ESP32_E-paper_5.79", 16, BLACK);
  }

  EPD_Display(ImageBW);
  EPD_PartUpdate();
  Serial.println("Menu refresh complete.");
}
