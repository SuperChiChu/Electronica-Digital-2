/*
 * Archivo: main_esclavo.c
 *
 *
 * Laboratorio 03 - Comunicaci?n SPI
 * Curso: Electr?nica Digital 2
 *
 * Autores:
 * Juan Daniel Sandoval - Carn? 24209
 * Fernando Guzm?n       - Carn? 24734
 *
 * Fecha: 29/07/2026
 */

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#include "adc.h"
#include "spi_slave.h"
#include "uart.h"
#include "led_counter.h"


uint16_t ConvertirAVoltaje(uint16_t valorADC)
{
	return ((uint32_t)valorADC * 5000UL) / 1023UL;
}


void MostrarVoltaje(const char *nombre, uint16_t mv)
{
	char buffer[40];

	sprintf(
		buffer,
		"%s: %u.%03u V\r\n",
		nombre,
		mv / 1000,
		mv % 1000
	);

	UART_TransmitString(buffer);
}


int main(void)
{
	uint16_t pot1;
	uint16_t pot2;

	uint16_t mv1;
	uint16_t mv2;

	uint8_t valorLED;

	ADC_Init();
	SPI_Slave_Init();
	UART_Init(9600);
	LEDs_Init();

	sei();

	while (1)
	{
		/*
		 * Lectura de A0.
		 * Se descarta la primera conversi?n.
		 */
		ADC_Read(0);
		pot1 = ADC_Read(0);

		/*
		 * Lectura de A1.
		 * Se descarta la primera conversi?n.
		 */
		ADC_Read(1);
		pot2 = ADC_Read(1);

		mv1 = ConvertirAVoltaje(pot1);
		mv2 = ConvertirAVoltaje(pot2);

		/*
		 * Actualizamos los datos disponibles para el maestro.
		 */
		SPI_Slave_ActualizarTrama(
			(uint8_t)(mv1 >> 8),
			(uint8_t)(mv1 & 0xFF),
			(uint8_t)(mv2 >> 8),
			(uint8_t)(mv2 & 0xFF)
		);

		/*
		 * Revisamos si el maestro envi? un nuevo n?mero.
		 */
		if (SPI_Slave_LeerValorLED(&valorLED))
		{
			LEDs_Mostrar(valorLED);
		}

		MostrarVoltaje("Pot1", mv1);
		MostrarVoltaje("Pot2", mv2);

		_delay_ms(100);
	}
}