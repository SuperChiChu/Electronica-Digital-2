#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdbool.h>
#include "display.h"
 

void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init(void) {
    MCUSR = 0;
    wdt_disable();
}
 
/*========================================================
   A0 = Boton Inicio      A1 = Boton J1     A2 = Boton J2
   D2-D5 = LEDs J1        D6-D9 = LEDs J2
   Display 7 seg manejado por display.h / display.c
   ======================================================== */
 
typedef struct {
    volatile uint8_t *port;
    volatile uint8_t *ddr;
    volatile uint8_t *pin;
    uint8_t bit;
} Pin;
 
/* ---------------- Botones (entradas) ---------------- */
static const Pin BOTON_INICIO = { &PORTC, &DDRC, &PINC, 0 }; /* A0 */
static const Pin BOTON_P1     = { &PORTC, &DDRC, &PINC, 1 }; /* A1 */
static const Pin BOTON_P2     = { &PORTC, &DDRC, &PINC, 2 }; /* A2 */
static const Pin *BOTONES[3]  = { &BOTON_INICIO, &BOTON_P1, &BOTON_P2 };
 
/* ---------------- LEDs Jugador 1: D2-D5 (PORTD) ---------------- */
static const Pin LED_P1[4] = {
    { &PORTD, &DDRD, &PIND, 2 },
    { &PORTD, &DDRD, &PIND, 3 },
    { &PORTD, &DDRD, &PIND, 4 },
    { &PORTD, &DDRD, &PIND, 5 },
};
 
/* ---------------- LEDs Jugador 2: D6,D7 (PORTD) y D8,D9 (PORTB) ---------------- */
static const Pin LED_P2[4] = {
    { &PORTD, &DDRD, &PIND, 6 },
    { &PORTD, &DDRD, &PIND, 7 },
    { &PORTB, &DDRB, &PINB, 0 },
    { &PORTB, &DDRB, &PINB, 1 },
};
 
/* ---------------- Funciones basicas de pines (reemplazan a Arduino.h) ---------------- */
static inline void pin_output(const Pin *p) {
    *(p->ddr) |= (uint8_t)(1 << p->bit);
}
 
static inline void pin_input_pullup(const Pin *p) {
    *(p->ddr)  &= (uint8_t)~(1 << p->bit);
    *(p->port) |= (uint8_t)(1 << p->bit);
}
 
static inline void pin_write(const Pin *p, uint8_t valor) {
    if (valor) {
        *(p->port) |= (uint8_t)(1 << p->bit);
    } else {
        *(p->port) &= (uint8_t)~(1 << p->bit);
    }
}
 
static inline uint8_t pin_read(const Pin *p) {
    return (uint8_t)((*(p->pin) >> p->bit) & 0x01);
}
 
/* ---------------- Temporizador (reemplaza a millis() de Arduino) ---------------- */
static volatile unsigned long ticks_ms = 0;
 
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
 
/* ---------------- Estado del juego ---------------- */
static int countP1 = 0;
static int countP2 = 0;
 
typedef enum { ESPERA, CUENTA_REGRESIVA, CARRERA, FIN } EstadoJuego;
static EstadoJuego estado = ESPERA;
 
/* ---------------- Antirrebote ---------------- */
#define DEBOUNCE_MS 25
 
static uint8_t lastFlicker[3]   = {1, 1, 1};
static uint8_t estadoEstable[3] = {1, 1, 1};
static unsigned long lastDebounceTime[3] = {0, 0, 0};
 
/* Devuelve 1 una sola vez, justo cuando el boton pasa de
   no-presionado a presionado (flanco de bajada, con antirrebote real) */
static uint8_t botonPresionado(int idx) {
    uint8_t lectura = pin_read(BOTONES[idx]);
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
 
/* ---------------- LEDs ----------------
   Contador BINARIO de 4 bits: cada LED representa un bit
   (LED[0]=bit0/LSB ... LED[3]=bit3/MSB). El valor va de
   0 a 15; gana quien llegue primero a 15 (1111 en binario). */
static void actualizarLEDs(void) {
    int i;
    for (i = 0; i < 4; i++) {
        pin_write(&LED_P1[i], (uint8_t)((countP1 >> i) & 1));
        pin_write(&LED_P2[i], (uint8_t)((countP2 >> i) & 1));
    }
}
 
static void apagarLEDs(void) {
    int i;
    for (i = 0; i < 4; i++) {
        pin_write(&LED_P1[i], 0);
        pin_write(&LED_P2[i], 0);
    }
}
 
/* ---------------- Cuenta regresiva ---------------- */
static void cuentaRegresiva(void) {
    int n;
    for (n = 3; n >= 1; n--) {
        display_mostrar_digito(n);
        _delay_ms(1000);
    }
    display_mostrar_digito(0);
    _delay_ms(400);
    display_apagar();
}
 
/* ---------------- Mostrar ganador ---------------- */
static void mostrarGanador(int jugador) {
    int rep, i;
    estado = FIN;
    apagarLEDs();
 
    for (rep = 0; rep < 8; rep++) {
        display_mostrar_digito(jugador);
        if (jugador == 1) {
            for (i = 0; i < 4; i++) pin_write(&LED_P1[i], 1);
        } else {
            for (i = 0; i < 4; i++) pin_write(&LED_P2[i], 1);
        }
        _delay_ms(250);
 
        display_apagar();
        apagarLEDs();
        _delay_ms(250);
    }
 
    display_mostrar_digito(jugador); /* deja fijo el numero del ganador */
}
 
/* ---------------- Reinicio ---------------- */
static void reiniciarRonda(void) {
    countP1 = 0;
    countP2 = 0;
    apagarLEDs();
    display_apagar();
    estado = ESPERA;
}
 
/* ---------------- Inicializacion de pines ---------------- */
static void inicializarPines(void) {
    int i;
 
    pin_input_pullup(&BOTON_INICIO);
    pin_input_pullup(&BOTON_P1);
    pin_input_pullup(&BOTON_P2);
 
    for (i = 0; i < 4; i++) {
        pin_output(&LED_P1[i]);
        pin_output(&LED_P2[i]);
    }
 
    display_init(); /* configura pines del display y lo apaga */
 
    apagarLEDs();
}
 
/* ================== MAIN ================== */
int main(void) {
    timer0_init();
    inicializarPines();
 
    for (;;) {
        switch (estado) {
 
        case ESPERA:
            if (botonPresionado(0)) { /* boton INICIO */
                estado = CUENTA_REGRESIVA;
            }
            break;
 
        case CUENTA_REGRESIVA:
            cuentaRegresiva();
            countP1 = 0;
            countP2 = 0;
            apagarLEDs();
            estado = CARRERA;
            break;
 
        case CARRERA: {
            uint8_t p1 = botonPresionado(1);
            uint8_t p2 = botonPresionado(2);
 
            if (p1 && countP1 < 15) countP1++;
            if (p2 && countP2 < 15) countP2++;
 
            if (p1 || p2) actualizarLEDs();
 
            if (countP1 >= 15) {
                mostrarGanador(1);
            } else if (countP2 >= 15) {
                mostrarGanador(2);
            }
            break;
        }
 
        case FIN:
            if (botonPresionado(0)) {
                reiniciarRonda();
            }
            break;
        }
    }
 
    return 0;
}