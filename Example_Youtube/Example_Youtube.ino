/*
 * YouTube Subscriber Counter — Premium Edition
 * Board: CrowPanel ESP32 E-Paper 5.79" (792×272)
 *
 * Features:
 * - Custom bold Helvetica font for large numbers
 * - Bitmap YouTube logo and icons
 * - Card-based dashboard layout
 * - Live subscriber count, views, video count
 * - Latest YouTube comment display
 * - Power-safe WiFi (disconnect during e-paper refresh)
 */

#include "EPD.h"
#include "bitmaps.h"
#include "crowpanel_pins.h"
#include "custom_font.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

uint8_t ImageBW[27200];

// ─── WiFi Credentials ───
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

// ─── YouTube API Settings ───
const char *youtubeApiKey = "YOUR_YOUTUBE_API_KEY";
const char *channelId = "YOUR_YOUTUBE_CHANNEL_ID";

// ─── Timing ───
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 60000; // 60 seconds

// ─── Cached Stats ───
String subscriberCount = "---";
String viewCount = "---";
String videoCount = "---";
String latestComment = "";
String commentAuthor = "";
String lastStatus = "Starting...";
bool mockMode = true;

// ═══════════════════════════════════════════════
// Custom Big Number Renderer (uses custom_font.h)
// ═══════════════════════════════════════════════
void showBigNumber(uint16_t x, uint16_t y, const char *str) {
  while (*str) {
    int idx = bigfont_index(*str);
    if (idx >= 0) {
      uint8_t w = bigfont_widths[idx];
      EPD_ShowPicture(x, y, w, BIGFONT_HEIGHT, bigfont_ptrs[idx], WHITE);
      x += w + 1; // 1px inter-character gap
    }
    str++;
  }
}

// Calculate rendered width of a big number string
uint16_t bigNumberWidth(const char *str) {
  uint16_t w = 0;
  while (*str) {
    int idx = bigfont_index(*str);
    if (idx >= 0) {
      w += bigfont_widths[idx] + 1;
    }
    str++;
  }
  if (w > 0)
    w -= 1; // remove trailing gap
  return w;
}

// ═══════════════════════════════════════════════
// WiFi Power Management
// ═══════════════════════════════════════════════
bool connectWiFi() {
  Serial.println("WiFi: Turning ON...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi: Connected, IP=");
    Serial.println(WiFi.localIP());
    return true;
  }
  Serial.println("WiFi: Connection failed!");
  return false;
}

void disconnectWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100);
  Serial.println("WiFi: OFF");
}

// ═══════════════════════════════════════════════
// Setup
// ═══════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("YouTube Counter Premium — Initializing...");

  // Power up e-paper
  pinMode(EINK_PWR, OUTPUT);
  digitalWrite(EINK_PWR, HIGH);
  delay(150);

  EPD_GPIOInit();
  Paint_NewImage(ImageBW, EPD_W, EPD_H, Rotation, WHITE);
  Paint_Clear(WHITE);

  EPD_FastMode1Init();
  EPD_Display_Clear();
  EPD_Update();
  EPD_Clear_R26A6H();

  // Determine mode
  if (strcmp(youtubeApiKey, "YOUR_YOUTUBE_API_KEY") != 0 &&
      strlen(youtubeApiKey) > 10) {
    mockMode = false;
  }

  // Show initial layout
  lastStatus = "Booting...";
  updateDisplay();
}

// ═══════════════════════════════════════════════
// Main Loop
// ═══════════════════════════════════════════════
void loop() {
  if (millis() - lastUpdate >= updateInterval || lastUpdate == 0) {
    lastUpdate = millis();

    if (!mockMode) {
      // Connect WiFi, fetch data, disconnect, then update display
      if (connectWiFi()) {
        fetchYouTubeStats();
        fetchLatestComment();
        disconnectWiFi();
        lastStatus = "Synced";
      } else {
        disconnectWiFi();
        lastStatus = "WiFi Error";
      }
      updateDisplay();
    } else {
      // Mock mode
      static long mockSubs = 1120000;
      mockSubs += random(-5, 12);
      char buf[24];
      snprintf(buf, sizeof(buf), "%ld", mockSubs);
      subscriberCount = buf;
      viewCount = "169440727";
      videoCount = "2246";
      latestComment = "Keren banget bang!";
      commentAuthor = "viewer123";
      lastStatus = "Demo Mode";
      updateDisplay();
    }
  }
  delay(50);
}

// ═══════════════════════════════════════════════
// YouTube Stats API
// ═══════════════════════════════════════════════
void fetchYouTubeStats() {
  Serial.println("API: Fetching channel stats...");

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url =
      "https://www.googleapis.com/youtube/v3/channels?part=statistics&id=";
  url += channelId;
  url += "&key=";
  url += youtubeApiKey;

  if (http.begin(client, url)) {
    int code = http.GET();
    if (code == HTTP_CODE_OK) {
      String payload = http.getString();
      DynamicJsonDocument doc(2048);
      if (!deserializeJson(doc, payload)) {
        JsonObject stats = doc["items"][0]["statistics"];
        const char *subs = stats["subscriberCount"];
        const char *views = stats["viewCount"];
        const char *videos = stats["videoCount"];
        if (subs)
          subscriberCount = subs;
        if (views)
          viewCount = views;
        if (videos)
          videoCount = videos;
        Serial.println("API: Stats updated!");
      } else {
        Serial.println("API: JSON parse error");
        lastStatus = "Parse Error";
      }
    } else {
      Serial.printf("API: HTTP error %d\n", code);
      lastStatus = "HTTP " + String(code);
    }
    http.end();
  }
}

// ═══════════════════════════════════════════════
// YouTube Comments API
// ═══════════════════════════════════════════════
void fetchLatestComment() {
  Serial.println("API: Fetching latest comment...");

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url =
      "https://www.googleapis.com/youtube/v3/commentThreads?part=snippet";
  url += "&allThreadsRelatedToChannelId=";
  url += channelId;
  url += "&maxResults=1&order=time&key=";
  url += youtubeApiKey;

  if (http.begin(client, url)) {
    int code = http.GET();
    if (code == HTTP_CODE_OK) {
      String payload = http.getString();
      DynamicJsonDocument doc(4096);
      if (!deserializeJson(doc, payload)) {
        JsonObject snippet =
            doc["items"][0]["snippet"]["topLevelComment"]["snippet"];
        const char *author = snippet["authorDisplayName"];
        const char *text = snippet["textDisplay"];
        if (author)
          commentAuthor = author;
        if (text) {
          latestComment = text;
          // Truncate to 45 chars for display
          if (latestComment.length() > 45) {
            latestComment = latestComment.substring(0, 42) + "...";
          }
          // Remove newlines
          latestComment.replace("\n", " ");
          latestComment.replace("\r", "");
        }
        Serial.println("API: Comment fetched!");
      }
    } else {
      Serial.printf("API: Comment HTTP error %d\n", code);
    }
    http.end();
  }
}

// ═══════════════════════════════════════════════
// Smart Number Formatting
// - Abbreviates numbers to K (thousands) and M (millions)
// ═══════════════════════════════════════════════
String formatNumber(String numStr) {
  // Extract digits only
  String digits = "";
  for (unsigned int i = 0; i < numStr.length(); i++) {
    if (isDigit(numStr[i]))
      digits += numStr[i];
  }
  if (digits.length() == 0)
    return numStr;

  // Convert to long long for comparison
  long long num = 0;
  for (unsigned int i = 0; i < digits.length(); i++) {
    num = num * 10 + (digits[i] - '0');
  }

  char buf[16];
  if (num >= 1000000LL) {
    float mVal = num / 1000000.0f;
    if (mVal >= 100.0f) {
      snprintf(buf, sizeof(buf), "%.1fM", mVal);
    } else {
      snprintf(buf, sizeof(buf), "%.2fM", mVal);
    }
    return String(buf);
  } else if (num >= 1000LL) {
    float kVal = num / 1000.0f;
    if (kVal >= 100.0f) {
      snprintf(buf, sizeof(buf), "%.0fK", kVal);
    } else {
      snprintf(buf, sizeof(buf), "%.1fK", kVal);
    }
    return String(buf);
  }

  // Full number with dot separator for < 1000
  String result = "";
  int len = digits.length();
  int count = 0;
  for (int i = len - 1; i >= 0; i--) {
    result = digits[i] + result;
    count++;
    if (count % 3 == 0 && i != 0) {
      result = "." + result;
    }
  }
  return result;
}

// ═══════════════════════════════════════════════
// Display Layout — Premium Card Dashboard
// ═══════════════════════════════════════════════
void updateDisplay() {
  Serial.println("Display: Updating...");
  Paint_Clear(WHITE);

  // Format numbers (dot separator, abbreviate if too long)
  String fmtSubs = formatNumber(subscriberCount);
  String fmtViews = formatNumber(viewCount);
  String fmtVideos = formatNumber(videoCount);

  // ┌──────────────────────────────────────────┐
  // │ HEADER: YouTube Logo + Channel Info      │
  // ├──────────────────────────────────────────┤
  // │ SUBSCRIBERS    │ TOTAL VIEWS │ VIDEOS    │
  // │ 1,120,000      │ 169,440,727 │ 2,246     │
  // ├──────────────────────────────────────────┤
  // │ 💬 Latest comment...        Status: OK   │
  // └──────────────────────────────────────────┘

  // ══ Outer Frame ══
  EPD_DrawRectangle(2, 2, 789, 269, BLACK, 0);
  EPD_DrawRectangle(4, 4, 787, 267, BLACK, 0);

  // ══ HEADER SECTION (y: 4–78) ══
  // YouTube logo bitmap
  EPD_ShowPicture(18, 16, YT_LOGO_W, YT_LOGO_H, yt_logo_bmp, WHITE);

  // Channel name (bold, large)
  EPD_ShowString(100, 18, "Anak Agung Duwi Arsana", 24, BLACK);
  // Channel handle
  EPD_ShowString(100, 48, "@anakagungduwiarsana", 16, BLACK);

  // Header divider (double line for premium feel)
  EPD_DrawLine(8, 78, 783, 78, BLACK);
  EPD_DrawLine(8, 80, 783, 80, BLACK);

  // ══ STATS SECTION (y: 82–228) ══
  // Three columns: Subscribers | Views | Videos
  // Column dividers
  int col1_x = 8;
  int col2_x = 285;
  int col3_x = 548;

  EPD_DrawLine(col2_x - 5, 82, col2_x - 5, 226, BLACK);
  EPD_DrawLine(col3_x - 5, 82, col3_x - 5, 226, BLACK);

  // ── Column 1: Subscribers ──
  EPD_ShowPicture(col1_x + 10, 90, ICON_SUBSCRIBER_W, ICON_SUBSCRIBER_H,
                  icon_subscriber_bmp, WHITE);
  EPD_ShowString(col1_x + 48, 95, "SUBSCRIBERS", 16, BLACK);

  // Big subscriber number
  showBigNumber(col1_x + 14, 130, fmtSubs.c_str());

  // ── Column 2: Total Views ──
  EPD_ShowPicture(col2_x + 10, 90, ICON_EYE_W, ICON_EYE_H, icon_eye_bmp, WHITE);
  EPD_ShowString(col2_x + 48, 95, "TOTAL VIEWS", 16, BLACK);

  // Big views number
  showBigNumber(col2_x + 14, 130, fmtViews.c_str());

  // ── Column 3: Total Videos ──
  EPD_ShowPicture(col3_x + 10, 90, ICON_CAMERA_W, ICON_CAMERA_H,
                  icon_camera_bmp, WHITE);
  EPD_ShowString(col3_x + 48, 95, "TOTAL VIDEOS", 16, BLACK);

  // Big video count
  showBigNumber(col3_x + 14, 130, fmtVideos.c_str());

  // ── Thin line separating numbers from sub-info ──
  EPD_DrawLine(col1_x, 180, col2_x - 8, 180, BLACK);
  EPD_DrawLine(col2_x, 180, col3_x - 8, 180, BLACK);
  EPD_DrawLine(col3_x, 180, 783, 180, BLACK);

  // Sub-labels under numbers
  EPD_ShowString(col1_x + 14, 188, "subscribers", 12, BLACK);
  EPD_ShowString(col2_x + 14, 188, "total views", 12, BLACK);
  EPD_ShowString(col3_x + 14, 188, "videos uploaded", 12, BLACK);

  // ══ FOOTER SECTION (y: 228–267) ══
  // Footer divider
  EPD_DrawLine(8, 228, 783, 228, BLACK);
  EPD_DrawLine(8, 230, 783, 230, BLACK);

  // Comment icon + latest comment
  EPD_ShowPicture(16, 240, ICON_COMMENT_W, ICON_COMMENT_H, icon_comment_bmp,
                  WHITE);

  if (latestComment.length() > 0) {
    String commentDisplay = commentAuthor + ": " + latestComment;
    // Truncate if too long for display (size 16 allows ~60 chars)
    if (commentDisplay.length() > 65) {
      commentDisplay = commentDisplay.substring(0, 62) + "...";
    }
    EPD_ShowString(54, 240, commentDisplay.c_str(), 16, BLACK);
  } else {
    EPD_ShowString(54, 240, "No comments yet", 16, BLACK);
  }

  // Status indicator (right side of footer)
  EPD_ShowString(680, 243, lastStatus.c_str(), 12, BLACK);

  // ── Render to screen ──
  EPD_Display(ImageBW);
  EPD_PartUpdate();
  Serial.println("Display: Update complete.");
}
