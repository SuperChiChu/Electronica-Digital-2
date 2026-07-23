/*
 * adc.c
 *
 * Implementacion del modulo ADC
 */

#include <avr/io.h>

#include "adc.h"

void adc_init(void)
{
    /* REFS0 = 1 -> referencia AVcc (+5V)*/
    ADMUX = (1 << REFS0);

    /*
     * ADEN  = habilita el ADC
     * ADPS2:0 = 111 -> preescaler 128 -> 16MHz/128 = 125kHz
     */
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t adc_read(uint8_t channel)
{
    /* Selecciona el canal (0-7), conservando REFS0 en ADMUX */
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

    /* Inicia la conversion */
    ADCSRA |= (1 << ADSC);

    /* Espera a que termine (ADSC vuelve a 0 automaticamente) */
    while (ADCSRA & (1 << ADSC)) {
        /* esperando */
    }

    return ADC; /* Valor de 10 bits (0-1023) */
}