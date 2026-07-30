/*
 * Archivo: Led_counter.c
 *
 *
 * Laboratorio 03 - Comunicaci?n SPI
 * Curso: Electr?nica Digital 2
 *
 * Autores:
 * Juan Daniel Sandoval - Carn? 24209
 * Fernando Guzm?n       - Carn? 24734
 *
 * Fecha: 29/07/2026
 */
#include "led_counter.h"

void LEDs_Init(void)
{
	/*
	 * D2-D7 corresponden a PD2-PD7.
	 */
	DDRD |= (1 << DDD2) |
	        (1 << DDD3) |
	        (1 << DDD4) |
	        (1 << DDD5) |
	        (1 << DDD6) |
	        (1 << DDD7);

	/*
	 * D8-D9 corresponden a PB0-PB1.
	 *
	 */
	DDRB |= (1 << DDB0) | (1 << DDB1);

	LEDs_Mostrar(0);
}

void LEDs_Mostrar(uint8_t valor)
{
	/*
	 * Conservamos PD0 y PD1 porque pertenecen a UART.
	 */
	PORTD = (PORTD & 0x03) | ((valor & 0x3F) << 2);

	/*
	 * Bits 6-7:
	 * Conservamos PB2-PB7 para no afectar SPI.
	 */
	PORTB = (PORTB & 0xFC) | ((valor >> 6) & 0x03);
}