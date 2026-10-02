#include "ili9341.h"

extern SPI_HandleTypeDef hspi1;

#define LCD_CS_LOW()    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET)
#define LCD_CS_HIGH()   HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET)

#define LCD_DC_LOW()    HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET)
#define LCD_DC_HIGH()   HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET)

#define LCD_RST_LOW()   HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET)
#define LCD_RST_HIGH()  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET)


static void LCD_SPI_Write(uint8_t data)
{
    HAL_SPI_Transmit(
        &hspi1,
        &data,
        1,
        HAL_MAX_DELAY
    );
}


static void LCD_Command16(uint8_t command)
{
    LCD_CS_LOW();
    LCD_DC_LOW();

    LCD_SPI_Write(0x00);
    LCD_SPI_Write(command);

    LCD_CS_HIGH();
}


static void LCD_Command8(uint8_t command)
{
    LCD_DC_LOW();

    LCD_SPI_Write(command);
}


static void LCD_PushCommand(uint8_t command,
                            const uint8_t *data,
                            uint16_t length)
{
    LCD_CS_LOW();

    LCD_DC_LOW();

    LCD_SPI_Write(0x00);
    LCD_SPI_Write(command);

    LCD_DC_HIGH();

    if (length > 0)
    {
        HAL_SPI_Transmit(
            &hspi1,
            (uint8_t *)data,
            length,
            HAL_MAX_DELAY
        );
    }

    LCD_CS_HIGH();
}


static void LCD_Reset(void)
{
    LCD_CS_HIGH();
    LCD_DC_HIGH();

    LCD_RST_HIGH();
    HAL_Delay(20);

    LCD_RST_LOW();
    HAL_Delay(5);

    LCD_RST_HIGH();
    HAL_Delay(150);

    LCD_CS_LOW();
    LCD_DC_LOW();

    LCD_SPI_Write(0x00);

    LCD_CS_HIGH();

    HAL_Delay(10);
}


void ILI9341_Init(void)
{
    LCD_Reset();

    LCD_Command16(0x01);
    HAL_Delay(50);

    LCD_Command16(0x28);


    uint8_t interfaceControl[] =
    {
        0x01,
        0x01,
        0x00
    };

    LCD_PushCommand(
        0xF6,
        interfaceControl,
        sizeof(interfaceControl)
    );


    uint8_t powerControlB[] =
    {
        0x00,
        0x81,
        0x30
    };

    LCD_PushCommand(
        0xCF,
        powerControlB,
        sizeof(powerControlB)
    );


    uint8_t powerSequence[] =
    {
        0x64,
        0x03,
        0x12,
        0x81
    };

    LCD_PushCommand(
        0xED,
        powerSequence,
        sizeof(powerSequence)
    );


    uint8_t driverTimingA[] =
    {
        0x85,
        0x10,
        0x78
    };

    LCD_PushCommand(
        0xE8,
        driverTimingA,
        sizeof(driverTimingA)
    );


    uint8_t powerControlA[] =
    {
        0x39,
        0x2C,
        0x00,
        0x34,
        0x02
    };

    LCD_PushCommand(
        0xCB,
        powerControlA,
        sizeof(powerControlA)
    );


    uint8_t pumpRatio[] =
    {
        0x20
    };

    LCD_PushCommand(
        0xF7,
        pumpRatio,
        sizeof(pumpRatio)
    );


    uint8_t driverTimingB[] =
    {
        0x00,
        0x00
    };

    LCD_PushCommand(
        0xEA,
        driverTimingB,
        sizeof(driverTimingB)
    );


    uint8_t rgbSignal[] =
    {
        0x00
    };

    LCD_PushCommand(
        0xB0,
        rgbSignal,
        sizeof(rgbSignal)
    );


    uint8_t inversionControl[] =
    {
        0x00
    };

    LCD_PushCommand(
        0xB4,
        inversionControl,
        sizeof(inversionControl)
    );


    uint8_t powerControl1[] =
    {
        0x21
    };

    LCD_PushCommand(
        0xC0,
        powerControl1,
        sizeof(powerControl1)
    );


    uint8_t powerControl2[] =
    {
        0x11
    };

    LCD_PushCommand(
        0xC1,
        powerControl2,
        sizeof(powerControl2)
    );


    uint8_t vcomControl1[] =
    {
        0x3F,
        0x3C
    };

    LCD_PushCommand(
        0xC5,
        vcomControl1,
        sizeof(vcomControl1)
    );


    uint8_t vcomControl2[] =
    {
        0xB5
    };

    LCD_PushCommand(
        0xC7,
        vcomControl2,
        sizeof(vcomControl2)
    );


    uint8_t memoryControl[] =
    {
        0x48
    };

    LCD_PushCommand(
        0x36,
        memoryControl,
        sizeof(memoryControl)
    );


    uint8_t pixelFormat[] =
    {
        0x55
    };

    LCD_PushCommand(
        0x3A,
        pixelFormat,
        sizeof(pixelFormat)
    );


    uint8_t frameControl[] =
    {
        0x00,
        0x1B
    };

    LCD_PushCommand(
        0xB1,
        frameControl,
        sizeof(frameControl)
    );


    uint8_t gamma3G[] =
    {
        0x00
    };

    LCD_PushCommand(
        0xF2,
        gamma3G,
        sizeof(gamma3G)
    );


    uint8_t gammaSet[] =
    {
        0x01
    };

    LCD_PushCommand(
        0x26,
        gammaSet,
        sizeof(gammaSet)
    );


    uint8_t positiveGamma[] =
    {
        0x0F,
        0x26,
        0x24,
        0x0B,
        0x0E,
        0x09,
        0x54,
        0xA8,
        0x46,
        0x0C,
        0x17,
        0x09,
        0x0F,
        0x07,
        0x00
    };

    LCD_PushCommand(
        0xE0,
        positiveGamma,
        sizeof(positiveGamma)
    );


    uint8_t negativeGamma[] =
    {
        0x00,
        0x19,
        0x1B,
        0x04,
        0x10,
        0x07,
        0x2A,
        0x47,
        0x39,
        0x03,
        0x06,
        0x06,
        0x30,
        0x38,
        0x0F
    };

    LCD_PushCommand(
        0xE1,
        negativeGamma,
        sizeof(negativeGamma)
    );


    uint8_t entryMode[] =
    {
        0x07
    };

    LCD_PushCommand(
        0xB7,
        entryMode,
        sizeof(entryMode)
    );


    LCD_Command16(0x11);
    HAL_Delay(150);

    LCD_Command16(0x29);
    HAL_Delay(100);
}


static void ILI9341_SetAddressWindow(uint16_t x0,
                                     uint16_t y0,
                                     uint16_t x1,
                                     uint16_t y1)
{
    uint8_t xData[4];

    xData[0] = x0 >> 8;
    xData[1] = x0 & 0xFF;
    xData[2] = x1 >> 8;
    xData[3] = x1 & 0xFF;

    LCD_PushCommand(
        0x2A,
        xData,
        4
    );


    uint8_t yData[4];

    yData[0] = y0 >> 8;
    yData[1] = y0 & 0xFF;
    yData[2] = y1 >> 8;
    yData[3] = y1 & 0xFF;

    LCD_PushCommand(
        0x2B,
        yData,
        4
    );
}


void ILI9341_BeginFrame(void)
{
    ILI9341_SetAddressWindow(
        0,
        0,
        ILI9341_WIDTH - 1,
        ILI9341_HEIGHT - 1
    );

    LCD_CS_LOW();

    LCD_Command8(0x2C);

    LCD_DC_HIGH();
}


void ILI9341_WriteLine(const uint8_t *data,
                       uint16_t size)
{
    HAL_SPI_Transmit(
        &hspi1,
        (uint8_t *)data,
        size,
        HAL_MAX_DELAY
    );
}


void ILI9341_EndFrame(void)
{
    LCD_CS_HIGH();
}


void ILI9341_FillRect(uint16_t x,
                      uint16_t y,
                      uint16_t w,
                      uint16_t h,
                      uint16_t color)
{
    if (w == 0 || h == 0)
    {
        return;
    }

    if (x >= ILI9341_WIDTH ||
        y >= ILI9341_HEIGHT)
    {
        return;
    }

    if (x + w > ILI9341_WIDTH)
    {
        w = ILI9341_WIDTH - x;
    }

    if (y + h > ILI9341_HEIGHT)
    {
        h = ILI9341_HEIGHT - y;
    }


    ILI9341_SetAddressWindow(
        x,
        y,
        x + w - 1,
        y + h - 1
    );


    LCD_CS_LOW();

    LCD_Command8(0x2C);

    LCD_DC_HIGH();


    uint8_t colorBuffer[128];

    uint8_t highByte =
        (color >> 8) & 0xFF;

    uint8_t lowByte =
        color & 0xFF;


    for (uint16_t i = 0;
         i < sizeof(colorBuffer);
         i += 2)
    {
        colorBuffer[i] = highByte;
        colorBuffer[i + 1] = lowByte;
    }


    uint32_t pixels =
        (uint32_t)w *
        (uint32_t)h;


    while (pixels > 0)
    {
        uint16_t pixelsNow;

        if (pixels > 64)
        {
            pixelsNow = 64;
        }
        else
        {
            pixelsNow =
                (uint16_t)pixels;
        }


        HAL_SPI_Transmit(
            &hspi1,
            colorBuffer,
            pixelsNow * 2,
            HAL_MAX_DELAY
        );


        pixels -= pixelsNow;
    }


    LCD_CS_HIGH();
}


void ILI9341_FillScreen(uint16_t color)
{
    ILI9341_FillRect(
        0,
        0,
        ILI9341_WIDTH,
        ILI9341_HEIGHT,
        color
    );
}


void ILI9341_DrawPixel(uint16_t x,
                       uint16_t y,
                       uint16_t color)
{
    ILI9341_FillRect(
        x,
        y,
        1,
        1,
        color
    );
}

void ILI9341_WriteLineDMA(const uint8_t *data, uint16_t size)
{
    HAL_SPI_Transmit_DMA(
        &hspi1,
        (uint8_t *)data,
        size
    );
}


void ILI9341_WaitDMA(void)
{
    while (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY)
    {
    }
}
