#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include "display.h"

/* Timer0 -> base de tiempo de 1 ms */
static unsigned long ticks_ms = 0;

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

static unsigned long millis(void) {
    unsigned long m;
    cli();
    m = ticks_ms;
    sei();
    return m;
}

/* Botones: A0 Inicio */
static void botones_config(void) {
    DDRC  &= (uint8_t)~((1 << 0) | (1 << 1) | (1 << 2));
    PORTC |= (uint8_t)((1 << 0) | (1 << 1) | (1 << 2));
}


static uint8_t boton_leer(uint8_t idx) {
    return (uint8_t)((PINC >> idx) & 0x01);
}

#define DEBOUNCE_MS 25
static uint8_t lastFlicker[3]   = {1, 1, 1};
static uint8_t estadoEstable[3] = {1, 1, 1};
static unsigned long lastDebounceTime[3] = {0, 0, 0};

/* Devuelve 1 una sola vez, en el flanco de bajada (recien presionado) */
static uint8_t botonPresionado(uint8_t idx) {
    uint8_t lectura = boton_leer(idx);
    uint8_t presionado = 0;

    if (lectura != lastFlicker[idx]) {
        lastDebounceTime[idx] = millis();
    }

    if ((millis() - lastDebounceTime[idx]) > DEBOUNCE_MS) {
        if (lectura != estadoEstable[idx]) {
            estadoEstable[idx] = lectura;
            if (estadoEstable[idx] == 0) {
                presionado = 1;
            }
        }
    }

    lastFlicker[idx] = lectura;
    return presionado;
}

/* LEDs Jugador 1 */
static void leds_p1_config(void) {
    DDRD |= (uint8_t)((1 << 2) | (1 << 3) | (1 << 4) | (1 << 5));
}

static void leds_p1_set(uint8_t patron) {
    if (patron & 0x01) PORTD |= (1 << 2); else PORTD &= (uint8_t)~(1 << 2);
    if (patron & 0x02) PORTD |= (1 << 3); else PORTD &= (uint8_t)~(1 << 3);
    if (patron & 0x04) PORTD |= (1 << 4); else PORTD &= (uint8_t)~(1 << 4);
    if (patron & 0x08) PORTD |= (1 << 5); else PORTD &= (uint8_t)~(1 << 5);
}

/* LEDs Jugador 2: D6,D7 (PORTD) y D8,D9 (PORTB) */
static void leds_p2_config(void) {
    DDRD |= (uint8_t)((1 << 6) | (1 << 7));
    DDRB |= (uint8_t)((1 << 0) | (1 << 1));
}

static void leds_p2_set(uint8_t patron) {
    if (patron & 0x01) PORTD |= (1 << 6); else PORTD &= (uint8_t)~(1 << 6);
    if (patron & 0x02) PORTD |= (1 << 7); else PORTD &= (uint8_t)~(1 << 7);
    if (patron & 0x04) PORTB |= (1 << 0); else PORTB &= (uint8_t)~(1 << 0);
    if (patron & 0x08) PORTB |= (1 << 1); else PORTB &= (uint8_t)~(1 << 1);
}

/* Convierte el numero de pulsaciones (0-4) al patron "de decada":
   0->0000  1->0001  2->0010  3->0100  4->1000 */
static uint8_t patronDecada(uint8_t pulsaciones) {
    if (pulsaciones == 0) return 0x00;
    return (uint8_t)(1 << (pulsaciones - 1));
}

/* Estados del juego */
typedef enum { ESPERA, CUENTA_REGRESIVA, CARRERA, FIN } EstadoJuego;
static EstadoJuego estado = ESPERA;

static uint8_t countP1 = 0;
static uint8_t countP2 = 0;
static int n = 5;
static unsigned long ultimoCambio = 0;

static void mostrarGanador(uint8_t jugador) {
    uint8_t rep;
    estado = FIN;
    leds_p1_set(0);
    leds_p2_set(0);

    for (rep = 0; rep < 8; rep++) {
        display_mostrar_digito(jugador);
        if (jugador == 1) leds_p1_set(0x0F); else leds_p2_set(0x0F);
        _delay_ms(250);

        display_apagar();
        leds_p1_set(0);
        leds_p2_set(0);
        _delay_ms(250);
    }

    display_mostrar_digito(jugador);
}

int main(void) {
    wdt_disable();

    timer0_init();
    display_init();
    botones_config();
    leds_p1_config();
    leds_p2_config();

    leds_p1_set(0);
    leds_p2_set(0);
    display_apagar();

    for (;;) {
        switch (estado) {

        case ESPERA:
            if (botonPresionado(0)) { /* boton Inicio */
                n = 5;
                display_mostrar_digito(n);
                ultimoCambio = millis();
                estado = CUENTA_REGRESIVA;
            }
            break;

        case CUENTA_REGRESIVA:
            if ((millis() - ultimoCambio) >= 1000) {
                ultimoCambio = millis();
                if (n > 0) {
                    n--;
                    display_mostrar_digito(n);
                } else {
                    display_apagar();
                    countP1 = 0;
                    countP2 = 0;
                    leds_p1_set(0);
                    leds_p2_set(0);
                    estado = CARRERA;
                }
            }
            break;

        case CARRERA: {
            uint8_t p1 = botonPresionado(1);
            uint8_t p2 = botonPresionado(2);

            if (p1 && countP1 < 4) {
                countP1++;
                leds_p1_set(patronDecada(countP1));
            }
            if (p2 && countP2 < 4) {
                countP2++;
                leds_p2_set(patronDecada(countP2));
            }

            if (countP1 >= 4) {
                mostrarGanador(1);
            } else if (countP2 >= 4) {
                mostrarGanador(2);
            }
            break;
        }

        case FIN:
            if (botonPresionado(0)) {
                countP1 = 0;
                countP2 = 0;
                leds_p1_set(0);
                leds_p2_set(0);
                display_apagar();
                estado = ESPERA;
            }
            break;
        }
    }

    return 0;
}