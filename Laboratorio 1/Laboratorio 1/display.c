#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include "display.h"

/* Estructura interna de pin de segmento
	volatile uint8_t *port;
	volatile uint8_t *ddr;
	uint8_t bit;
} PinSeg;

/* a,b,c,d -> D10-D13 (PORTB)   e,f,g -> A3-A5 (PORTC) */
static const PinSeg SEG[7] = {
	{ &PORTB, &DDRB, 2 }, /* a - D10 */
	{ &PORTB, &DDRB, 3 }, /* b - D11 */
	{ &PORTB, &DDRB, 4 }, /* c - D12 */
	{ &PORTB, &DDRB, 5 }, /* d - D13 */
	{ &PORTC, &DDRC, 3 }, /* e - A3  */
	{ &PORTC, &DDRC, 4 }, /* f - A4  */
	{ &PORTC, &DDRC, 5 }, /* g - A5  */
};

/* Patrones de segmentos para digitos 0-9 (1 = segmento ENCENDIDO) */
static const bool DIGITOS[10][7] = {
	{1,1,1,1,1,1,0}, /* 0 */
	{0,1,1,0,0,0,0}, /* 1 */
	{1,1,0,1,1,0,1}, /* 2 */
	{1,1,1,1,0,0,1}, /* 3 */
	{0,1,1,0,0,1,1}, /* 4 */
	{1,0,1,1,0,1,1}, /* 5 */
	{1,0,1,1,1,1,1}, /* 6 */
	{1,1,1,0,0,0,0}, /* 7 */
	{1,1,1,1,1,1,1}, /* 8 */
	{1,1,1,1,0,1,1}  /* 9 */
};

static inline void seg_write(const PinSeg *p, uint8_t valor) {
	if (valor) {
		*(p->port) |= (uint8_t)(1 << p->bit);
		} else {
		*(p->port) &= (uint8_t)~(1 << p->bit);
	}
}

void display_init(void) {
	int i;
	for (i = 0; i < 7; i++) {
		*(SEG[i].ddr) |= (uint8_t)(1 << SEG[i].bit); /* pin como salida */
	}
	display_apagar();
}

void display_apagar(void) {
	int i;
	for (i = 0; i < 7; i++) {
		seg_write(&SEG[i], 1); /* Anodo comun: 1(HIGH) = segmento apagado */
	}
}

void display_mostrar_digito(int d) {
	int i;
	if (d < 0 || d > 9) {
		display_apagar();
		return;
	}
	for (i = 0; i < 7; i++) {
		seg_write(&SEG[i], DIGITOS[d][i] ? 0 : 1); /* 0(LOW) enciende */
	}
}