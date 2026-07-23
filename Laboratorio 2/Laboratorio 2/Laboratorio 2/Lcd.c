/*
 * lcd.c
 *
 * Implementacion de la libreria LCD HD44780 en modo 8 bits.
 *
 */

#include <avr/io.h>
#include <util/delay.h>

#include "lcd_config.h"
#include "lcd.h"

/*  Funciones internas (privadas de este archivo) */

/* Genera el pulso de Enable necesario para validar cualquier operacion */
static void lcd_enable_pulse(void)
{
    LCD_CTRL_PORT |= (1 << LCD_EN);
    _delay_us(1);                     /* ancho minimo de pulso EN */
    LCD_CTRL_PORT &= ~(1 << LCD_EN);
}

/* Coloca el byte completo en los dos puertos y genera el pulso de Enable */
static void lcd_write_byte(uint8_t value)
{
    /* bits 0-5 del dato -> PD2-PD7, conservando PD0/PD1 (UART) */
    LCD_DATA1_PORT = (LCD_DATA1_PORT & 0x03) | (uint8_t)(value << 2);

    /* bits 6-7 del dato -> PB0-PB1, conservando el resto de PORTB */
    LCD_DATA2_PORT = (LCD_DATA2_PORT & 0xFC) | ((value >> 6) & 0x03);

    lcd_enable_pulse();
}

/* ---------------------------------------------------------------------
 * Funciones publicas
 * --------------------------------------------------------------------- */

void lcd_command(uint8_t cmd)
{
    LCD_CTRL_PORT &= ~(1 << LCD_RS);  /* RS = 0 -> comando */
    lcd_write_byte(cmd);

    /* Clear display (0x01) y Return home (0x02) tardan ~1.64ms */
    if (cmd == 0x01 || cmd == 0x02) {
        _delay_ms(2);
    } else {
        _delay_us(100);
    }
}

void lcd_data(uint8_t data)
{
    LCD_CTRL_PORT |= (1 << LCD_RS);   /* RS = 1 -> dato */
    lcd_write_byte(data);
    _delay_us(100);
}

void lcd_init(void)
{
    /* Bus de datos como salida */
    LCD_DATA1_DDR |= 0xFC;            /* PD2-PD7 como salida (no toca PD0/PD1) */
    LCD_DATA2_DDR |= 0x03;            /* PB0-PB1 como salida */

    /* Pines de control como salida */
    LCD_CTRL_DDR |= (1 << LCD_RS) | (1 << LCD_EN);
    LCD_CTRL_PORT &= ~((1 << LCD_RS) | (1 << LCD_EN));

    /* ---- PASO 1: Power on ----*/
    _delay_ms(20);

    LCD_CTRL_PORT &= ~(1 << LCD_RS);

    lcd_write_byte(0x30);
    _delay_ms(5);

    lcd_write_byte(0x30);
    _delay_us(150);

    lcd_write_byte(0x30);
    _delay_us(150);

    /* ---- PASO 2: Function set ----*/
    lcd_command(0x38);

    /* ---- PASO 3: Display on/off control ----*/
    lcd_command(0x0C);

    /* ---- PASO 4: Entry mode set ---- */
    lcd_command(0x06);

    /* ---- PASO 5: Clear display ----*/
    lcd_command(0x01);
}

void lcd_clear(void)
{
    lcd_command(0x01);
}

void lcd_home(void)
{
    lcd_command(0x02);
}

void lcd_gotoxy(uint8_t col, uint8_t row)
{
    uint8_t address;

    switch (row) {
        case 0: address = 0x00 + col; break;
        case 1: address = 0x40 + col; break;
        case 2: address = 0x14 + col; break;
        case 3: address = 0x54 + col; break;
        default: address = 0x00 + col; break;
    }

    lcd_command(0x80 | address);
}

void lcd_string(const char *str)
{
    while (*str != '\0') {
        lcd_data((uint8_t)*str);
        str++;
    }
}