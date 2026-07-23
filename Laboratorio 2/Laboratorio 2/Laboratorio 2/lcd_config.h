/*
 * lcd_config.h
 *
 * Configuraci?n de pines para LCD HD44780 en modo 8 bits.
 *
 *
 * D0/D1 del Arduino (PD0/PD1) quedan libres para UART.
 * A0/A1 quedan libres para los potenci?metros (ADC0/ADC1).
 *
 * SI yo quiero cambiar los pines de uso es en este archivo
 */

#ifndef LCD_CONFIG_H_
#define LCD_CONFIG_H_

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

/* ---------- Bus de datos, parte baja: bits 0-5 en PORTD bits 2-7 ---------- */
#define LCD_DATA1_DDR   DDRD
#define LCD_DATA1_PORT  PORTD

/* ---------- Bus de datos, parte alta: bits 6-7 en PORTB bits 0-1 ---------- */
#define LCD_DATA2_DDR   DDRB
#define LCD_DATA2_PORT  PORTB

/* ---------- Pines de control (RS y E; R/W va fijo a GND) ---------- */
#define LCD_CTRL_DDR    DDRB
#define LCD_CTRL_PORT   PORTB

#define LCD_RS          PB2   /* D10: Register Select: 0 = comando, 1 = dato */
#define LCD_EN          PB3   /* D11: Enable: pulso para validar la operaci?n */

#endif /* LCD_CONFIG_H_ */