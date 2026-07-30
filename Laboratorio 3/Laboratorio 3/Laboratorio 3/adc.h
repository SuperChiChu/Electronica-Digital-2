/*
 * Archivo: adc.h
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
#ifndef ADC_H_
#define ADC_H_

#include <avr/io.h>

void ADC_Init(void);
uint16_t ADC_Read(uint8_t canal);

#endif