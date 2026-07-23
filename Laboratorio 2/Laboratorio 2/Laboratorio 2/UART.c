/*
 * uart.c
 *
 */
 
#ifndef F_CPU
#define F_CPU 16000000UL
#endif
 
#include <avr/io.h>
 
#include "uart.h"
 
void uart_init(uint32_t baud)
{
    uint16_t ubrr = (uint16_t)((F_CPU / (16UL * baud)) - 1);
 
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)(ubrr);
 
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);            /* habilita RX y TX */
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);          /* 8 bits de datos, sin paridad, 1 stop */
}
 
void uart_transmit(char data)
{
    while (!(UCSR0A & (1 << UDRE0))) {
        /* espera a que el buffer este en 0*/
    }
    UDR0 = (uint8_t)data;
}
 
void uart_print(const char *str)
{
    while (*str != '\0') {
        uart_transmit(*str);
        str++;
    }
}
 
uint8_t uart_available(void)
{
    return (UCSR0A & (1 << RXC0)) ? 1 : 0;
}
 
char uart_receive(void)
{
    while (!(UCSR0A & (1 << RXC0))) {
        /* espera a que llegue un byte */
    }
    return (char)UDR0;
}