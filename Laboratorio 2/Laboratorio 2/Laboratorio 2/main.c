/*
 * Laboratorio 2.c
 *
 * Created: 16/07/2026 18:01:19
 * Author : Juan Daniel Sandoval
 *
 */

#include <avr/io.h>
#include <stdint.h>

#include "lcd.h"
#include "adc.h"
#include "uart.h"
#include <util/delay.h>

#define POT1_CHANNEL 0   /* A0 = ADC0 */
#define POT2_CHANNEL 1   /* A1 = ADC1 */
#define BAUD_RATE    9600UL

#define CONTADOR_MIN 0
#define CONTADOR_MAX 1023

/* Convierte un valor ADC (0-1023) a una cadena "X.XV" (4 caracteres + nulo) */
static void adc_to_voltage_string(uint16_t adc_value, char *out)
{
	/* tenths va de 0 a 50, representando 0.0V a 5.0V en decimas de voltaje */
	uint16_t tenths = (uint16_t)(((uint32_t)adc_value * 50UL) / 1023UL);
	
	uint8_t entero  = tenths / 10;   /* 0-5 */
	uint8_t decimal = tenths % 10;   /* 0-9 */
	
	out[0] = '0' + entero;
	out[1] = '.';
	out[2] = '0' + decimal;
	out[3] = 'V';
	out[4] = '\0';
}

/* Convierte un valor 0-1023 a una cadena de 4 digitos con ceros a la izquierda */
static void counter_to_string4(uint16_t value, char *out)
{
	out[0] = '0' + (value / 1000) % 10;
	out[1] = '0' + (value / 100) % 10;
	out[2] = '0' + (value / 10) % 10;
	out[3] = '0' + value % 10;
	out[4] = '\0';
}

int main(void)
{
	char voltaje_pot1[5];
	char voltaje_pot2[5];
	char contador_str[5];
	uint16_t contador = 0;
	
	lcd_init();
	adc_init();
	uart_init(BAUD_RATE);
	
	lcd_gotoxy(0, 0);
	lcd_string("POT1  POT2  UART");
	
	while (1) {
		/* Lectura y muestra de los potenciometros */
		uint16_t pot1_raw = adc_read(POT1_CHANNEL);
		adc_to_voltage_string(pot1_raw, voltaje_pot1);
		
		uint16_t pot2_raw = adc_read(POT2_CHANNEL);
		adc_to_voltage_string(pot2_raw, voltaje_pot2);
		
		lcd_gotoxy(0, 1);
		lcd_string(voltaje_pot1);
		
		lcd_gotoxy(6, 1);
		lcd_string(voltaje_pot2);
		
		/* Envio de ambas lecturas por UART */
		uart_print("POT1:");
		uart_print(voltaje_pot1);
		uart_print(" POT2:");
		uart_print(voltaje_pot2);
		uart_print("\r\n");
		
		/* Recepcion de '+' / '-' para el contador */
		if (uart_available()) {
			char c = uart_receive();
			
			if (c == '+' && contador < CONTADOR_MAX) {
				contador++;
				} else if (c == '-' && contador > CONTADOR_MIN) {
				contador--;
			}
		}
		
		counter_to_string4(contador, contador_str);
		lcd_gotoxy(12, 1);
		lcd_string(contador_str);
		
		_delay_ms(200);
	}
}