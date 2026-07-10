#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include "display.h"

typedef struct {
	volatile uint8_t *port;
	volatile uint8_t *ddr;
	uint8_t bit;
} PinSeg;

static const PinSeg SEG[7] = {
	{ &PORTB, &DDRB, 2 },
	{ &PORTB, &DDRB, 3 },
	{ &PORTB, &DDRB, 4 },
	{ &PORTB, &DDRB, 5 },
	{ &PORTC, &DDRC, 3 },
	{ &PORTC, &DDRC, 4 },
	{ &PORTC, &DDRC, 5 }
};

static const bool DIGITOS[10][7] = {
	{1,1,1,1,1,1,0},
	{0,1,1,0,0,0,0},
	{1,1,0,1,1,0,1},
	{1,1,1,1,0,0,1},
	{0,1,1,0,0,1,1},
	{1,0,1,1,0,1,1},
	{1,0,1,1,1,1,1},
	{1,1,1,0,0,0,0},
	{1,1,1,1,1,1,1},
	{1,1,1,1,0,1,1}
};

static void seg_write(const PinSeg *p, uint8_t valor) {
	if (valor) {
		*(p->port) |= (uint8_t)(1 << p->bit);
		} else {
		*(p->port) &= (uint8_t)~(1 << p->bit);
	}
}

void display_init(void) {
	int i;
	for (i = 0; i < 7; i++) {
		*(SEG[i].ddr) |= (uint8_t)(1 << SEG[i].bit);
	}
	display_apagar();
}

void display_apagar(void) {
	int i;
	for (i = 0; i < 7; i++) {
		seg_write(&SEG[i], 1);
	}
}

void display_mostrar_digito(int d) {
	int i;
	if (d < 0 || d > 9) {
		display_apagar();
		return;
	}
	for (i = 0; i < 7; i++) {
		seg_write(&SEG[i], DIGITOS[d][i] ? 0 : 1);
	}
}