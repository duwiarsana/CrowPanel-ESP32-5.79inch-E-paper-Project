#ifndef CROWPANEL_PINS_H
#define CROWPANEL_PINS_H

// ==========================================
// CROWPANEL ESP32 E-PAPER 5.79" PIN DEFINITIONS
// ==========================================

// 1. E-Paper Display Pins (SPI + Control)
#define EINK_PWR   7   // HIGH = Power Enabled, LOW = Power Disabled (CRITICAL!)
#define EINK_BUSY  48  // Busy status input from SSD1683
#define EINK_RST   47  // Hardware Reset
#define EINK_DC    46  // Data/Command control line
#define EINK_CS    45  // Chip Select SPI
#define EINK_SCK   12  // SPI Clock
#define EINK_MOSI  11  // SPI MOSI (Master Out Slave In)

// 2. Physical Buttons
#define BTN_MENU   2   // Menu Button (Active Low, pull-up recommended)
#define BTN_EXIT   1   // Exit/Back Button (Active Low, pull-up recommended)

// 3. Rotary Encoder / Dial Switch
#define ROTARY_UP    6   // Rotate Up / Anti-clockwise
#define ROTARY_DOWN  4   // Rotate Down / Clockwise
#define ROTARY_CLICK 5   // Click / Push button (Confirm)

// 4. MicroSD Card Interface (SPI)
#define SD_SCK   18
#define SD_MOSI  23
#define SD_MISO  19
#define SD_CS    39

#endif // CROWPANEL_PINS_H
