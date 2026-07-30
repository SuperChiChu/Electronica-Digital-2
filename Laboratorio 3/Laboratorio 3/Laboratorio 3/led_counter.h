/*
 * Archivo: led_counter.h
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
#ifndef LED_COUNTER_H_
#define LED_COUNTER_H_

#include <avr/io.h>

void LEDs_Init(void);
void LEDs_Mostrar(uint8_t valor);

#endif