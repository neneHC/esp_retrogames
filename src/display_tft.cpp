#include <Arduino.h>
#include <TFT_eSPI.h>
#include "display_tft.h"

extern TFT_eSPI tft;
extern uint16_t myPalette[256];

static uint16_t line_buffer[240];
static int nes_line_lut[135];

extern "C" void display_begin()
{
    // Backlight da IdeaSpark (GPIO 32 e 27)
    pinMode(32, OUTPUT);
    digitalWrite(32, HIGH);
    pinMode(27, OUTPUT);
    digitalWrite(27, HIGH);

    // Garante CS alternativo ativo
    pinMode(5, OUTPUT);
    digitalWrite(5, LOW);

    tft.init();
    tft.setRotation(1); // Paisagem 240x135
    tft.fillScreen(TFT_BLACK);
}

extern "C" void display_init()
{
    // Mapeamento vertical de 240 scanlines do NES para as 135 linhas da tela ST7789
    for (int y = 0; y < 135; y++) {
        nes_line_lut[y] = (y * 240) / 135;
    }
}

extern "C" void display_write_frame(const uint8_t *data[])
{
    if (!data) return;

    tft.startWrite();
    tft.setAddrWindow(0, 0, 240, 135);
    for (int y = 0; y < 135; y++) {
        // Corta 8 pixels laterais (256 -> 240 pixels)
        const uint8_t *src = data[nes_line_lut[y]] + 8;
        for (int x = 0; x < 240; x++) {
            line_buffer[x] = myPalette[src[x]];
        }
        tft.pushColors(line_buffer, 240, false);
    }
    tft.endWrite();
}

extern "C" void display_clear()
{
    tft.fillScreen(TFT_BLACK);
}
