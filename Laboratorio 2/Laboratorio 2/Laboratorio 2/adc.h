/*
 * adc.h
 *
 * Referencia: AVcc (+5V), conversion simple por canal.
 */

#ifndef ADC_H_
#define ADC_H_

#include <stdint.h>

/* Configura el ADC: referencia AVcc, preescaler 128 */
void adc_init(void);

/* Lee un canal (0-7, correspondiente a A0-A7) y devuelve el valor 0-1023 */
uint16_t adc_read(uint8_t channel);

#endif /* ADC_H_ */