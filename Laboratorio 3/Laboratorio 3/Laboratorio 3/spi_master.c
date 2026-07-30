/*
 * Archivo: spi_master.c
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

#include "spi_master.h"

// Pinout SPI del ATmega328p
//   PB5 = D13 = SCK   -> en modo maestro: salida
//   PB4 = D12 = MISO  -> en modo maestro: entrada (por aqui llega lo que contesta el esclavo)
//   PB3 = D11 = MOSI  -> en modo maestro: salida definida por el usuario
//   PB2 = D10 = SS    -> en modo maestro: salida definida por el usuario

void SPI_Master_Init(void)
{
	// MOSI, SCK y SS como salidas
	DDRB |= (1 << PB3) | (1 << PB5) | (1 << PB2);

	// MISO como entrada
	DDRB &= ~(1 << PB4);

	// SS inicialmente en alto
	PORTB |= (1 << PB2);

	/*
	 * SPE  = habilitar SPI
	 * MSTR = modo maestro
	 * SPR1:SPR0 = 11 -> F_CPU / 128
	 * CPOL = 0, CPHA = 0 -> modo SPI 0
	 */
	SPCR = (1 << SPE) |
	       (1 << MSTR) |
	       (1 << SPR1) |
	       (1 << SPR0);

	// SPI2X = 0
	SPSR &= ~(1 << SPI2X);
}

uint8_t SPI_Master_Transmit(uint8_t dato)
{
	SPDR = dato;
	while (!(SPSR & (1 << SPIF)));  // esperamos a que SPIF se ponga en 1
	return SPDR;
}

void SPI_Master_Select(void)
{
	PORTB &= ~(1 << PB2);  // SS en bajo -> "El esclavo es selecionado para ver que esta haciendo"
}

void SPI_Master_Deselect(void)
{
	PORTB |= (1 << PB2);   // SS en alto -> fin de la conversacion
}