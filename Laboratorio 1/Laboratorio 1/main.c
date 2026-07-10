#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdint.h>
#include "display.h"

static volatile uint32_t ticks_ms = 0;

static void timer0_init(void)
{
    TCCR0A = (1 << WGM01);              /* Modo CTC */
    TCCR0B = (1 << CS01) | (1 << CS00); /* Prescaler 64 */
    OCR0A  = 249;                       /* Interrupci?n cada 1 ms */
    TIMSK0 = (1 << OCIE0A);

    sei();
}

ISR(TIMER0_COMPA_vect)
{
    ticks_ms++;
}

static uint32_t millis(void)
{
    uint32_t tiempo;
    uint8_t estado_interrupciones = SREG;

    cli();
    tiempo = ticks_ms;
    SREG = estado_interrupciones;

    return tiempo;
}

static uint8_t boton_presionado(void)
{
    return !(PINC & (1 << PC0));
}

static void esperar_liberacion_boton(void)
{
    while (boton_presionado()) {
        /* Esperar hasta soltar el bot?n */
    }
}

typedef enum {
    MOSTRANDO,
    PAUSA
} EstadoContador;

int main(void)
{
    int8_t n = 5;
    EstadoContador estado = PAUSA;
    uint32_t ultimoCambio = 0;

    wdt_disable();
    timer0_init();
    display_init();

    /* A0/PC0 como entrada con resistencia pull-up */
    DDRC  &= ~(1 << DDC0);
    PORTC |=  (1 << PORTC0);

    display_apagar();

    for (;;) {

        if (estado == PAUSA) {

            /*
             * El contador permanece detenido hasta que
             * se presione el bot?n.
             */
            if (boton_presionado()) {

                /* Eliminaci?n sencilla del rebote */
                uint32_t inicioRebote = millis();

                while ((millis() - inicioRebote) < 30) {
                    /* Esperar 30 ms */
                }

                if (boton_presionado()) {
                    esperar_liberacion_boton();

                    n = 5;
                    display_mostrar_digito(n);
                    ultimoCambio = millis();
                    estado = MOSTRANDO;
                }
            }
        }

        else if (estado == MOSTRANDO) {

            if ((millis() - ultimoCambio) >= 1000) {
                ultimoCambio += 1000;

                if (n > 0) {
                    n--;
                    display_mostrar_digito(n);
                }

                /*
                 * Cuando llega a cero, queda mostrando cero
                 * y entra en pausa.
                 */
                if (n == 0) {
                    estado = PAUSA;
                }
            }
        }
    }

    return 0;
}