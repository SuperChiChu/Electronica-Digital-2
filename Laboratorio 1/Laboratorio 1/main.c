#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include "display.h"
static ticks_ms = 0;
static void timer0_init(void) {
	TCCR0A = (1 << WGM01);              /* Modo CTC */
	TCCR0B = (1 << CS01) | (1 << CS00); /* Prescaler 64 */
	OCR0A  = 249;                       /* 16MHz/64/250 = 1000 Hz -> 1 ms */
	TIMSK0 = (1 << OCIE0A);
	sei();
}
ISR(TIMER0_COMPA_vect) {
	ticks_ms++;
}
static millis(void) {
	unsigned long m;
	cli();
	m = ticks_ms;
	sei();
	return m;
}
typedef enum { MOSTRANDO, PAUSA } EstadoContador;
int main(void) {
	int n = 5;
	EstadoContador estado = MOSTRANDO;
	unsigned long ultimoCambio;
	wdt_disable();
	timer0_init();
	display_init();

	/* A0 como entrada con pull-up: el boton conecta a GND */
	DDRC  &= ~(1 << 0);
	PORTC |= (1 << 0);

	display_apagar();
	while (PINC & (1 << 0)) {
		/* espera aqui hasta que se presione el boton de A0 */
	}

	display_mostrar_digito(n);
	ultimoCambio = millis();
	for (;;) {
		if ((millis() - ultimoCambio) >= 1000) {
			ultimoCambio = millis();
			if (estado == MOSTRANDO) {
				if (n > 0) {
					n--;
					display_mostrar_digito(n);
					} else {
					display_apagar();
					estado = PAUSA;
				}
				} else { /* PAUSA */
				n = 5;
				display_mostrar_digito(n);
				estado = MOSTRANDO;
			}
		}
	}
	return 0;
}