/*
 * Archivo: spi_master.h
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

#ifndef SPI_MASTER_H_
#define SPI_MASTER_H_

#include <avr/io.h>

void SPI_Master_Init(void);
uint8_t SPI_Master_Transmit(uint8_t dato);
void SPI_Master_Select(void);
void SPI_Master_Deselect(void);

#endif /* SPI_MASTER_H_ */
