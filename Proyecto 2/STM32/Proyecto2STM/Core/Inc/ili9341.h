#ifndef INC_ILI9341_H_
#define INC_ILI9341_H_

#include "main.h"
#include <stdint.h>

#define ILI9341_WIDTH   240
#define ILI9341_HEIGHT  320

#define ILI9341_BLACK   0x0000
#define ILI9341_WHITE   0xFFFF
#define ILI9341_RED     0xF800
#define ILI9341_GREEN   0x07E0
#define ILI9341_BLUE    0x001F
#define ILI9341_YELLOW  0xFFE0
#define ILI9341_GRAY    0x8410

void ILI9341_Init(void);

void ILI9341_FillScreen(uint16_t color);

void ILI9341_FillRect(uint16_t x,
                      uint16_t y,
                      uint16_t w,
                      uint16_t h,
                      uint16_t color);

void ILI9341_DrawPixel(uint16_t x,
                       uint16_t y,
                       uint16_t color);

void ILI9341_BeginFrame(void);

void ILI9341_WriteLine(const uint8_t *data,
                       uint16_t size);

void ILI9341_EndFrame(void);

void ILI9341_WriteLineDMA(const uint8_t *data, uint16_t size);

void ILI9341_WaitDMA(void);

#endif
