/*
 * Example: Smart Dashboard
 * Board: CrowPanel ESP32 E-Paper 5.79"
 * 
 * Shows a premium grid layout dashboard with:
 * - System stats & time
 * - Sensors widget (Temp, Humid, Pres, Air Quality)
 * - Interactive counter & hardware button states
 * - Graphical battery status bar
 */

#include "EPD.h"
#include "crowpanel_pins.h"

uint8_t ImageBW[27200]; 
int counter = 100;
bool displayNeedsUpdate = true;
String lastAction = "None";

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Dashboard Initializing...");

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

  Serial.println("Dashboard ready.");
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
  if (digitalRead(BTN_MENU) == LOW) {
    lastAction = "MENU PRESS";
    displayNeedsUpdate = true;
    delay(200);
  }
  if (digitalRead(BTN_EXIT) == LOW) {
    lastAction = "EXIT PRESS";
    displayNeedsUpdate = true;
    delay(200);
  }
  if (digitalRead(ROTARY_CLICK) == LOW) {
    lastAction = "ROTARY RESET";
    counter = 100;
    displayNeedsUpdate = true;
    delay(250);
  }
  if (digitalRead(ROTARY_UP) == LOW) {
    counter += 5;
    lastAction = "VAL INCREASE";
    displayNeedsUpdate = true;
    delay(100);
  }
  else if (digitalRead(ROTARY_DOWN) == LOW) {
    counter -= 5;
    lastAction = "VAL DECREASE";
    displayNeedsUpdate = true;
    delay(100);
  }
}

void updateDisplay() {
  Serial.println("Updating Dashboard...");
  Paint_Clear(WHITE);

  // --- Outer Border ---
  EPD_DrawRectangle(5, 5, 787, 267, BLACK, 0);

  // --- Column Dividers ---
  EPD_DrawLine(260, 5, 260, 267, BLACK);
  EPD_DrawLine(530, 5, 530, 267, BLACK);

  // ==========================================
  // COLUMN 1: SYSTEM & TIME
  // ==========================================
  EPD_ShowString(20, 20, "SYSTEM MONITOR", 24, BLACK);
  EPD_DrawLine(20, 48, 240, 48, BLACK);

  EPD_ShowString(20, 70, "Time  : 23:18:45", 16, BLACK);
  EPD_ShowString(20, 100, "Uptime: 01h 45m", 16, BLACK);
  EPD_ShowString(20, 130, "WiFi  : CONNECTED", 16, BLACK);
  EPD_ShowString(20, 160, "RSSI  : -58 dBm", 16, BLACK);
  EPD_ShowString(20, 190, "IP    : 192.168.1.50", 16, BLACK);
  
  // Custom Battery Indicator
  EPD_ShowString(20, 230, "BATT: ", 16, BLACK);
  EPD_DrawRectangle(75, 230, 175, 246, BLACK, 0); // Battery body
  EPD_DrawRectangle(175, 234, 180, 242, BLACK, 1); // Battery tip
  EPD_DrawRectangle(78, 233, 150, 243, BLACK, 1); // 75% Fill

  // ==========================================
  // COLUMN 2: HARDWARE INTERFACE
  // ==========================================
  EPD_ShowString(280, 20, "HARDWARE CONTROL", 24, BLACK);
  EPD_DrawLine(280, 48, 510, 48, BLACK);

  EPD_ShowString(280, 70, "Target Val :", 16, BLACK);
  char countStr[16];
  snprintf(countStr, sizeof(countStr), "%d %%", counter);
  EPD_ShowString(400, 70, countStr, 16, BLACK);

  EPD_ShowString(280, 100, "Last Cmd   :", 16, BLACK);
  EPD_ShowString(400, 100, lastAction.c_str(), 16, BLACK);

  // Visual Horizontal Progress Bar for Counter
  EPD_DrawRectangle(280, 130, 510, 145, BLACK, 0);
  int progressWidth = map(constrain(counter, 0, 100), 0, 100, 0, 226);
  if (progressWidth > 0) {
    EPD_DrawRectangle(282, 132, 282 + progressWidth, 143, BLACK, 1);
  }

  // Relay states (Mock toggle icons)
  EPD_ShowString(280, 175, "RELAY 1: [ ON ]", 16, BLACK);
  EPD_ShowString(280, 205, "RELAY 2: [OFF ]", 16, BLACK);
  EPD_ShowString(280, 235, "BUZZER : [OFF ]", 16, BLACK);

  // ==========================================
  // COLUMN 3: ENVIRONMENT SENSORS
  // ==========================================
  EPD_ShowString(550, 20, "ENVIRONMENT", 24, BLACK);
  EPD_DrawLine(550, 48, 770, 48, BLACK);

  EPD_ShowString(550, 70, "Temp   : 26.8 C", 16, BLACK);
  EPD_ShowString(550, 100, "Humid  : 62.4 %", 16, BLACK);
  EPD_ShowString(550, 130, "Press  : 1012 hPa", 16, BLACK);
  EPD_ShowString(550, 160, "Air Q. : Good (34)", 16, BLACK);

  // Mini Graph/Chart inside the third column
  EPD_ShowString(550, 195, "TEMP TREND:", 12, BLACK);
  EPD_DrawRectangle(550, 215, 770, 255, BLACK, 0); // Graph frame
  // Draw mock line charts
  EPD_DrawLine(555, 245, 590, 235, BLACK);
  EPD_DrawLine(590, 235, 625, 240, BLACK);
  EPD_DrawLine(625, 240, 660, 220, BLACK);
  EPD_DrawLine(660, 220, 695, 230, BLACK);
  EPD_DrawLine(695, 230, 730, 225, BLACK);
  EPD_DrawLine(730, 225, 765, 220, BLACK);

  EPD_Display(ImageBW);
  EPD_PartUpdate();
  Serial.println("Dashboard updated.");
}
