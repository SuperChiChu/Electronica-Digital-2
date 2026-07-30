/*
 * Archivo: spi_slave.h
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
#ifndef SPI_SLAVE_H_
#define SPI_SLAVE_H_

#include <avr/io.h>

void SPI_Slave_Init(void);

void SPI_Slave_ActualizarTrama(
	uint8_t b0,
	uint8_t b1,
	uint8_t b2,
	uint8_t b3
);

/*
 * Devuelve 1 cuando lleg? un nuevo n?mero para los LEDs.
 * El valor recibido se guarda en la direcci?n indicada.
 */
uint8_t SPI_Slave_LeerValorLED(uint8_t *valor);

#endif