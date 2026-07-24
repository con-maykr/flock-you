#ifdef USE_HELTEC_OLED

#include "display_dongle.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>
#include <stdio.h>

// Heltec WiFi LoRa 32 V4 (ESP32-S3R2 variant) onboard SSD1306-compatible
// 128x64 OLED. Vext (active-low) gates power to the display; it must be
// driven low before the panel will respond on I2C. Pins per Heltec's
// official V4 (R2) pinmap / Meshtastic's heltec_v4 variant.
#define HELTEC_VEXT_PIN     36
#define HELTEC_OLED_RST_PIN 21
#define HELTEC_OLED_SDA_PIN 17
#define HELTEC_OLED_SCL_PIN 18
#define HELTEC_OLED_ADDR    0x3C

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
static unsigned long alertUntilMs = 0;
static uint8_t idleCh = 1;
static int idleDetCount = 0;
static bool inAlert = false;
static uint8_t lastDrawnCh = 0xFF;
static int lastDrawnHits = -1;

static void vextOn() {
  pinMode(HELTEC_VEXT_PIN, OUTPUT);
  digitalWrite(HELTEC_VEXT_PIN, LOW);  // active low — enables the OLED rail
}

static void oledHardwareReset() {
  pinMode(HELTEC_OLED_RST_PIN, OUTPUT);
  digitalWrite(HELTEC_OLED_RST_PIN, HIGH);
  delay(1);
  digitalWrite(HELTEC_OLED_RST_PIN, LOW);
  delay(10);
  digitalWrite(HELTEC_OLED_RST_PIN, HIGH);
  delay(10);
}

static void drawMethodLines(const char* method, int y) {
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  char line[24];
  const char* p = method;
  while (*p) {
    size_t n = 0;
    const char* seg = p;
    while (p[n] && p[n] != '_' && n < 20) n++;
    if (p[n] == '_') n++;
    size_t copy = n;
    if (copy >= sizeof(line)) copy = sizeof(line) - 1;
    memcpy(line, seg, copy);
    line[copy] = '\0';
    display.setCursor(2, y);
    display.print(line);
    y += 9;
    p += n;
    if (y > 62) break;
  }
}

void dongleDisplayInit() {
  vextOn();
  delay(50);  // let the rail stabilize before the panel is addressed
  oledHardwareReset();
  Wire.begin(HELTEC_OLED_SDA_PIN, HELTEC_OLED_SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, HELTEC_OLED_ADDR);
  display.clearDisplay();
  display.display();
  dongleDisplayShowIdle(1, 0);
}

void dongleDisplayShowIdle(uint8_t ch, int detCount) {
  idleCh = ch;
  idleDetCount = detCount;
  inAlert = false;
  alertUntilMs = 0;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(4, 4);
  display.print("SCANNING");

  display.setTextSize(1);
  char line[24];
  display.setCursor(2, 40);
  snprintf(line, sizeof(line), "Ch: %u", (unsigned)ch);
  display.print(line);
  display.setCursor(2, 52);
  snprintf(line, sizeof(line), "Hits: %d", detCount);
  display.print(line);
  display.display();

  lastDrawnCh = ch;
  lastDrawnHits = detCount;
}

void dongleDisplayShowAlert(const char* method, const char* mac, int8_t rssi,
                            uint8_t ch, unsigned long alertMs) {
  idleCh = ch;
  inAlert = true;
  if (alertMs == 0) alertMs = 1;
  alertUntilMs = millis() + alertMs;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(4, 0);
  display.print("DETECT");

  drawMethodLines(method ? method : "?", 18);

  display.setTextSize(1);
  char line[24];
  display.setCursor(2, 44);
  display.print(mac ? mac : "");
  display.setCursor(2, 53);
  snprintf(line, sizeof(line), "RSSI %d  CH %u", (int)rssi, (unsigned)ch);
  display.print(line);
  display.display();
}

bool dongleDisplayInAlert(unsigned long now) {
  return inAlert && alertUntilMs != 0 && (long)(now - alertUntilMs) < 0;
}

void dongleDisplayTick(unsigned long now, uint8_t ch, int detCount) {
  idleCh = ch;
  idleDetCount = detCount;
  if (inAlert && alertUntilMs != 0 && (long)(now - alertUntilMs) >= 0) {
    dongleDisplayShowIdle(idleCh, idleDetCount);
    return;
  }
  if (!inAlert && (ch != lastDrawnCh || detCount != lastDrawnHits)) {
    dongleDisplayShowIdle(ch, detCount);
  }
}

void dongleDisplayShutdown() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(16, 16);
  display.print("BYE");
  display.setTextSize(1);
  display.setCursor(2, 44);
  display.print("Tap button");
  display.setCursor(2, 54);
  display.print("to wake");
  display.display();
  delay(600);  // let the message be visible before the rail cuts

  display.clearDisplay();
  display.display();
  digitalWrite(HELTEC_VEXT_PIN, HIGH);  // active-low rail — HIGH cuts OLED power
}

#endif
