/*
 * Archivo: uart.h
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


#ifndef UART_H_
#define UART_H_

#include <avr/io.h>

void UART_Init(uint32_t baudios);
void UART_TransmitChar(char c);
void UART_TransmitString(const char *str);




#endif /* UART_H_ */