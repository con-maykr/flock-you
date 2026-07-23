#pragma once

#include <stdint.h>

// Shared status/alert display API. Backed by display_dongle.cpp (LilyGO
// T-Dongle S3, ST7735) when USE_T_DONGLE_DISPLAY is set, or display_heltec.cpp
// (Heltec WiFi LoRa 32 V4, SSD1306) when USE_HELTEC_OLED is set.
#if defined(USE_T_DONGLE_DISPLAY) || defined(USE_HELTEC_OLED)

void dongleDisplayInit();
void dongleDisplayShowIdle(uint8_t ch, int detCount);
void dongleDisplayShowAlert(const char* method, const char* mac, int8_t rssi,
                            uint8_t ch, unsigned long alertMs);
void dongleDisplayTick(unsigned long now, uint8_t ch, int detCount);
bool dongleDisplayInAlert(unsigned long now);

#else

static inline void dongleDisplayInit() {}
static inline void dongleDisplayShowIdle(uint8_t, int) {}
static inline void dongleDisplayShowAlert(const char*, const char*, int8_t, uint8_t,
                                          unsigned long) {}
static inline void dongleDisplayTick(unsigned long, uint8_t, int) {}
static inline bool dongleDisplayInAlert(unsigned long) { return false; }

#endif
