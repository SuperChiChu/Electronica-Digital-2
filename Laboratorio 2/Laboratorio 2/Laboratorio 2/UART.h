/*
 * uart.h
 *
 * 8 bits de datos, sin paridad, 1 bit de stop (8N1).
 */
 
#ifndef UART_H_
#define UART_H_
 
#include <stdint.h>
 
/* Configura el UART a la velocidad indicada */
void uart_init(uint32_t baud);
 
/* Envia un solo caracter (bloqueante hasta que el registro est? libre) */
void uart_transmit(char data);
 
/* Envia una cadena terminada en '\0' */
void uart_print(const char *str);
 
/* Devuelve 1 si hay un byte recibido esperando a ser leido, 0 si no */
uint8_t uart_available(void);
 
/* Lee un caracter recibido (bloqueante hasta que llegue uno) */
char uart_receive(void);
 
#endif