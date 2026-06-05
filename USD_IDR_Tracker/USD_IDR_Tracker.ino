/*
 * USD/IDR Real-time Exchange Rate Tracker & Chart
 * Board: CrowPanel ESP32 E-Paper 5.79" (792x272 resolution)
 *
 * Features:
 * - Connects to WiFi
 * - Syncs time via NTP
 * - Fetches USD to IDR rates (current + 10-day historical trend)
 * - Renders a sleek UI with current rate and auto-scaled line chart
 * - Manual update trigger via Rotary Click
 */

#include "EPD.h"
#include "crowpanel_pins.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <time.h>

// ================= USER CONFIGURATION =================
const char *ssid = "Pengki";       // Ganti dengan SSID WiFi Anda
const char *password = "tehpucuk"; // Ganti dengan Password WiFi Anda
// ======================================================

// E-Paper image buffer
uint8_t ImageBW[27200];

/// Global variables to store exchange rate data
float currentRate = 0.0;
float lastRateValue = 0.0;
bool forceRefreshThisTime = false;
String currentDateStr = "N/A";
float historicalRates[120];
String historicalDates[120];
int ratesCount = 0;
bool wifiConnected = false;
String statusMessage = "Initializing...";

// Configuration for update interval (e.g., every 1 hour)
unsigned long lastUpdateMillis = 0;
const unsigned long updateInterval = 3600000; // 1 hour in ms

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("IHSG Tracker Initializing...");

  // Power up E-Paper display
  pinMode(EINK_PWR, OUTPUT);
  digitalWrite(EINK_PWR, HIGH);
  delay(150);

  // Setup rotary encoder button for manual refresh
  pinMode(ROTARY_CLICK, INPUT_PULLUP);

  // Initialize E-Paper GPIOs
  EPD_GPIOInit();

  // Create clean white canvas
  Paint_NewImage(ImageBW, EPD_W, EPD_H, Rotation, WHITE);
  Paint_Clear(WHITE);

  EPD_FastMode1Init();
  EPD_Display_Clear();
  EPD_Update();
  EPD_Clear_R26A6H();

  // Draw Initial Screen (Connecting to WiFi)
  drawConnectingScreen();

  // Start WiFi Connection
  connectWiFi();

  // Fetch initial data - force a display refresh on startup
  forceRefreshThisTime = true;
  fetchAndDisplay();
  lastUpdateMillis = millis();
}

void loop() {
  // Check for manual refresh button press (Rotary Click)
  if (digitalRead(ROTARY_CLICK) == LOW) {
    Serial.println("Manual refresh triggered!");
    statusMessage = "Refreshing data...";
    drawConnectingScreen(); // Show loading/refreshing state
    forceRefreshThisTime = true; // Force refresh EPD
    fetchAndDisplay();
    lastUpdateMillis = millis();
    delay(500); // Debounce
  }

  // Automatic hourly update
  if (millis() - lastUpdateMillis >= updateInterval) {
    fetchAndDisplay();
    lastUpdateMillis = millis();
  }

  delay(100);
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    statusMessage = "WiFi Connected";
    return;
  }

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    wifiConnected = true;
    statusMessage = "WiFi Connected";

    // Sync time using NTP
    configTime(7 * 3600, 0, "pool.ntp.org",
               "time.nist.gov"); // GMT+7 for Indonesia
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      Serial.println("Time synchronized successfully.");
    }
  } else {
    Serial.println("\nWiFi Connection Failed.");
    wifiConnected = false;
    statusMessage = "WiFi Failed";
  }
}

// Helper to get formatted date string in YYYY-MM-DD
String getDateStringOffset(int daysOffset) {
  time_t rawtime;
  time(&rawtime);
  rawtime += (daysOffset * 24 * 3600);
  struct tm *timeinfo = localtime(&rawtime);

  char buffer[12];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
  return String(buffer);
}

void fetchAndDisplay() {
  if (WiFi.status() != WL_CONNECTED) {
    statusMessage = "WiFi Disconnected";
    updateEPaperDisplay();
    return;
  }

  // Yahoo Finance chart API for IHSG (^JKSE) - 3 months, daily interval
  String url = "https://query1.finance.yahoo.com/v8/finance/chart/%5EJKSE?range=3mo&interval=1d";
  Serial.print("Requesting URL: ");
  Serial.println(url);

  HTTPClient http;
  http.begin(url);
  // Set User-Agent so Yahoo Finance doesn't reject the connection (very important!)
  http.setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36");
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Payload received successfully.");

    parseYahooPayload(payload);
    statusMessage = "Success";
    lastUpdateMillis = millis();

    // Smart Auto-Refresh Logic:
    // Only refresh physically if the value has changed OR the user pressed the button (forceRefreshThisTime)
    if (forceRefreshThisTime || currentRate != lastRateValue) {
      Serial.print("Value changed (New: ");
      Serial.print(currentRate);
      Serial.print(" | Old: ");
      Serial.print(lastRateValue);
      Serial.println(") or Force Refresh active. Updating E-Paper display!");
      
      lastRateValue = currentRate;
      forceRefreshThisTime = false; // Reset force flag
      updateEPaperDisplay();
    } else {
      Serial.print("Value unchanged (");
      Serial.print(currentRate);
      Serial.println("). Skipping E-Paper physical update to preserve screen life.");
    }
  } else {
    Serial.print("HTTP GET Error: ");
    Serial.println(httpCode);
    statusMessage = "API Error: " + String(httpCode);
    updateEPaperDisplay();
  }
  http.end();
}

void parseYahooPayload(String json) {
  int closeStart = json.indexOf("\"close\":[");
  if (closeStart == -1) {
    closeStart = json.indexOf("\"adjclose\":[");
  }
  if (closeStart == -1) {
    Serial.println("Error: 'close' array not found in JSON");
    statusMessage = "Parse Error";
    return;
  }
  
  int pos = closeStart + 9; // move past "\"close\":["
  ratesCount = 0;
  float lastValidRate = 0.0;
  
  while (ratesCount < 120) {
    // Find next comma or closing bracket
    int nextComma = json.indexOf(",", pos);
    int nextBracket = json.indexOf("]", pos);
    int endPos = (nextComma == -1) ? nextBracket : ((nextBracket == -1) ? nextComma : min(nextComma, nextBracket));
    
    if (endPos == -1 || endPos == pos) break;
    
    String valStr = json.substring(pos, endPos);
    valStr.trim();
    
    if (valStr == "null") {
      // Handle nulls by repeating the last valid value to avoid chart breaks
      if (ratesCount > 0) {
        historicalRates[ratesCount] = lastValidRate;
        ratesCount++;
      }
    } else {
      float rate = valStr.toFloat();
      if (rate > 0.0) {
        historicalRates[ratesCount] = rate;
        lastValidRate = rate;
        ratesCount++;
      }
    }
    
    if (endPos == nextBracket) {
      break; // reached end of array
    }
    pos = endPos + 1;
  }
  
  if (ratesCount > 0) {
    currentRate = historicalRates[ratesCount - 1];
    
    // Format current local time for header update time
    time_t rawtime;
    time(&rawtime);
    struct tm *timeinfo = localtime(&rawtime);
    char buffer[24];
    strftime(buffer, sizeof(buffer), "%m-%d %H:%M", timeinfo);
    currentDateStr = String(buffer);
    
    // Generate start date label (approx 90 days ago)
    time_t startTime = rawtime - (90 * 24 * 3600);
    struct tm *startTimeinfo = localtime(&startTime);
    char startBuffer[12];
    strftime(startBuffer, sizeof(startBuffer), "%Y-%m-%d", startTimeinfo);
    historicalDates[0] = String(startBuffer);
    
    // Generate end date label (today)
    char endBuffer[12];
    strftime(endBuffer, sizeof(endBuffer), "%Y-%m-%d", timeinfo);
    historicalDates[ratesCount - 1] = String(endBuffer);
  }
}

void drawConnectingScreen() {
  Paint_Clear(WHITE);
  EPD_DrawRectangle(5, 5, 787, 267, BLACK, 0);

  EPD_ShowString(30, 80, "IHSG Index Real-time Tracker", 24, BLACK);
  EPD_ShowString(30, 130,
                 "Status: Fetching IHSG (^JKSE) index from Yahoo Finance...", 16,
                 BLACK);
  EPD_ShowString(30, 180, "Connecting to WiFi Network...", 16, BLACK);

  EPD_Display(ImageBW);
  EPD_PartUpdate();
}

void drawDottedLine(int x0, int y0, int x1, int y1) {
  int len = abs(x1 - x0);
  for (int i = 0; i < len; i += 6) {
    EPD_DrawLine(x0 + i, y0, min(x0 + i + 3, x1), y0, BLACK);
  }
}

void updateEPaperDisplay() {
  Paint_Clear(WHITE);

  // Outer Border
  EPD_DrawRectangle(5, 5, 787, 267, BLACK, 0);

  // ==========================================
  // HEADER BAR (X: 20 -> 770, Y: 15 -> 50)
  // ==========================================
  // Title on Left
  EPD_ShowString(20, 15, "IHSG Index", 24, BLACK);
  
  // Index value and changes
  if (ratesCount > 0) {
    char rateStr[32];
    snprintf(rateStr, sizeof(rateStr), "%.2f", currentRate);
    EPD_ShowString(190, 15, rateStr, 24, BLACK);
    
    // Calculate Change
    float change = 0.0;
    float changePercent = 0.0;
    bool isUp = true;
    if (ratesCount > 1) {
      change = currentRate - historicalRates[ratesCount - 2];
      changePercent = (change / historicalRates[ratesCount - 2]) * 100.0;
      if (change < 0.0) {
        isUp = false;
      }
    }

    // Draw Arrow Indicator
    int arrowX = 330;
    int arrowY = 20;
    if (isUp) {
      EPD_DrawLine(arrowX, arrowY, arrowX - 4, arrowY + 4, BLACK);
      EPD_DrawLine(arrowX, arrowY, arrowX + 4, arrowY + 4, BLACK);
      EPD_DrawLine(arrowX, arrowY, arrowX, arrowY + 8, BLACK);
    } else {
      EPD_DrawLine(arrowX, arrowY + 8, arrowX - 4, arrowY + 4, BLACK);
      EPD_DrawLine(arrowX, arrowY + 8, arrowX + 4, arrowY + 4, BLACK);
      EPD_DrawLine(arrowX, arrowY, arrowX, arrowY + 8, BLACK);
    }

    // Print change text
    char changeStr[48];
    if (isUp) {
      snprintf(changeStr, sizeof(changeStr), "+%.2f (+%.2f%%)", change, changePercent);
    } else {
      snprintf(changeStr, sizeof(changeStr), "%.2f (%.2f%%)", change, changePercent);
    }
    EPD_ShowString(345, 20, changeStr, 16, BLACK);
    
    // Latest Date on Right
    char dateLabel[32];
    snprintf(dateLabel, sizeof(dateLabel), "Update: %s", currentDateStr.c_str());
    EPD_ShowString(580, 20, dateLabel, 16, BLACK);
  } else {
    EPD_ShowString(190, 15, "No Data", 24, BLACK);
  }

  // Header bottom border
  EPD_DrawLine(20, 50, 770, 50, BLACK);

  // ==========================================
  // CHART AREA (X: 75 -> 765, Y: 75 -> 215)
  // ==========================================
  if (ratesCount > 1) {
    int chartXStart = 75;
    int chartXEnd = 765;
    int chartYStart = 75;
    int chartYEnd = 215;

    // Find min and max rates for auto-scaling
    float minRate = historicalRates[0];
    float maxRate = historicalRates[0];
    for (int i = 1; i < ratesCount; i++) {
      if (historicalRates[i] < minRate) minRate = historicalRates[i];
      if (historicalRates[i] > maxRate) maxRate = historicalRates[i];
    }
    
    // Add small margin to min/max
    float range = maxRate - minRate;
    if (range < 1.0) {
      minRate -= 10.0;
      maxRate += 10.0;
      range = maxRate - minRate;
    } else {
      minRate -= range * 0.05;
      maxRate += range * 0.05;
      range = maxRate - minRate;
    }

    // Draw grid horizontal dotted lines for max and min rates
    drawDottedLine(chartXStart, chartYStart, chartXEnd, chartYStart);
    drawDottedLine(chartXStart, chartYEnd, chartXEnd, chartYEnd);
    
    // Grid horizontal line at center/median
    int yCenter = chartYStart + (chartYEnd - chartYStart) / 2;
    drawDottedLine(chartXStart, yCenter, chartXEnd, yCenter);

    // Label min & max rates on Y axis (left side) - no "Rp" for stock index
    char maxLabel[32];
    char minLabel[32];
    char midLabel[32];
    snprintf(maxLabel, sizeof(maxLabel), "%.0f", maxRate);
    snprintf(minLabel, sizeof(minLabel), "%.0f", minRate);
    snprintf(midLabel, sizeof(midLabel), "%.0f", minRate + range/2.0);
    
    EPD_ShowString(10, chartYStart - 5, maxLabel, 12, BLACK);
    EPD_ShowString(10, yCenter - 5, midLabel, 12, BLACK);
    EPD_ShowString(10, chartYEnd - 5, minLabel, 12, BLACK);

    // Plot points
    int prevX = 0;
    int prevY = 0;
    float xStep = (float)(chartXEnd - chartXStart) / (ratesCount - 1);

    for (int i = 0; i < ratesCount; i++) {
      int x = chartXStart + (int)(i * xStep);
      // Invert Y coordinate since 0 is at top
      int y = chartYEnd - (int)(((historicalRates[i] - minRate) / range) * (chartYEnd - chartYStart));
      
      // Connect with line
      if (i > 0) {
        // Draw double line (bold/thickness)
        EPD_DrawLine(prevX, prevY, x, y, BLACK);
        EPD_DrawLine(prevX, prevY + 1, x, y + 1, BLACK);
        EPD_DrawLine(prevX, prevY - 1, x, y - 1, BLACK);
      }
      
      prevX = x;
      prevY = y;
    }

    // Draw little circles for the start and end points of the graph to look stylish
    int startY = chartYEnd - (int)(((historicalRates[0] - minRate) / range) * (chartYEnd - chartYStart));
    int endY = chartYEnd - (int)(((historicalRates[ratesCount-1] - minRate) / range) * (chartYEnd - chartYStart));
    EPD_DrawCircle(chartXStart, startY, 4, BLACK, 1);
    EPD_DrawCircle(chartXEnd, endY, 4, BLACK, 1);

    // ==========================================
    // FOOTER BAR (Y: 235 -> 260)
    // ==========================================
    // Divider line above footer
    EPD_DrawLine(20, 230, 770, 230, BLACK);

    // Start date (left)
    char startLabel[32];
    snprintf(startLabel, sizeof(startLabel), "Start: %s", historicalDates[0].c_str());
    EPD_ShowString(20, 240, startLabel, 16, BLACK);

    // Refresh prompt (middle)
    EPD_ShowString(300, 240, "Press Encoder to Force Refresh", 16, BLACK);

    // Status / End Date (right)
    char statusLabel[48];
    snprintf(statusLabel, sizeof(statusLabel), "WiFi: %s", statusMessage.c_str());
    EPD_ShowString(600, 240, statusLabel, 16, BLACK);
    
  } else {
    EPD_ShowString(60, 120, "Unable to load chart. Check WiFi connection.", 16, BLACK);
  }

  // Refresh physical screen
  EPD_Display(ImageBW);
  EPD_PartUpdate();
  Serial.println("Display update complete.");
}
