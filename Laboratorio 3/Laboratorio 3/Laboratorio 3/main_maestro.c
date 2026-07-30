/*
 * Archivo: main_maestro.c
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

#include "spi_master.h"
#include "uart.h"
#include "led_counter.h"

#define SPI_COMANDO_LEER_ADC 0xA5
#define SPI_COMANDO_LEDS     0x5A


/*
 * Variables utilizadas por la interrupci?n UART.
 *
 * El m?ximo n?mero tiene tres d?gitos:
 * "255"
 */
static volatile char bufferRX[4];
static volatile uint8_t indiceRX = 0;
static volatile uint8_t numeroDisponible = 0;
static volatile uint8_t numeroRecibido = 0;
static volatile uint8_t entradaInvalida = 0;


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


/*
 * Convierte los caracteres almacenados en bufferRX a un n?mero.
 *
 * Ejemplo:
 * '1', '7', '3' -> 173
 */
static void UART_ProcesarNumero(void)
{
	uint16_t resultado = 0;
	uint8_t i;

	if (indiceRX == 0)
	{
		return;
	}

	for (i = 0; i < indiceRX; i++)
	{
		resultado = (resultado * 10) +
		            (uint8_t)(bufferRX[i] - '0');
	}

	if (resultado <= 255)
	{
		numeroRecibido = (uint8_t)resultado;
		numeroDisponible = 1;
	}
	else
	{
		entradaInvalida = 1;
	}

	indiceRX = 0;
}


/*
 * Interrupci?n cuando llega un car?cter por UART.
 */
ISR(USART_RX_vect)
{
	char recibido = UDR0;

	if ((recibido == '\r') || (recibido == '\n'))
	{
		UART_ProcesarNumero();
	}
	else if ((recibido >= '0') && (recibido <= '9'))
	{
		if (indiceRX < 3)
		{
			bufferRX[indiceRX] = recibido;
			indiceRX++;
		}
		else
		{
			/*
			 * M?s de tres d?gitos.
			 */
			indiceRX = 0;
			entradaInvalida = 1;
		}
	}
	else
	{
		/*
		 * Se recibi? algo que no es un n?mero.
		 */
		indiceRX = 0;
		entradaInvalida = 1;
	}
}


static void SPI_EnviarValorLED(uint8_t valor)
{
	SPI_Master_Select();

	/*
	 * Primero indicamos que enviaremos el valor para los LEDs.
	 */
	SPI_Master_Transmit(SPI_COMANDO_LEDS);
	_delay_us(20);

	/*
	 * Despu?s mandamos el n?mero de 8 bits.
	 */
	SPI_Master_Transmit(valor);
	_delay_us(20);

	SPI_Master_Deselect();
}


static void SPI_LeerVoltajes(uint16_t *mv1, uint16_t *mv2)
{
	uint8_t b0;
	uint8_t b1;
	uint8_t b2;
	uint8_t b3;

	SPI_Master_Select();

	/*
	 * Solicitamos una nueva trama.
	 */
	SPI_Master_Transmit(SPI_COMANDO_LEER_ADC);
	_delay_us(20);

	b0 = SPI_Master_Transmit(0x00);
	_delay_us(20);

	b1 = SPI_Master_Transmit(0x00);
	_delay_us(20);

	b2 = SPI_Master_Transmit(0x00);
	_delay_us(20);

	b3 = SPI_Master_Transmit(0x00);
	_delay_us(20);

	SPI_Master_Deselect();

	*mv1 = ((uint16_t)b0 << 8) | b1;
	*mv2 = ((uint16_t)b2 << 8) | b3;
}


int main(void)
{
	uint16_t mv1;
	uint16_t mv2;

	uint8_t valor;

	char buffer[50];

	SPI_Master_Init();
	UART_Init(9600);
	LEDs_Init();

	/*
	 * Necesario porque UART utiliza USART_RX_vect.
	 */
	sei();

	UART_TransmitString(
		"\r\nIngrese un numero entre 0 y 255 y presione Enter:\r\n"
	);

	while (1)
	{
		/*
		 * Revisamos si lleg? un nuevo n?mero por UART.
		 */
		if (numeroDisponible)
		{
			uint8_t estadoAnterior = SREG;

			cli();

			valor = numeroRecibido;
			numeroDisponible = 0;

			SREG = estadoAnterior;

			/*
			 * Mostrar localmente en el maestro.
			 */
			LEDs_Mostrar(valor);

			/*
			 * Enviar al esclavo.
			 */
			SPI_EnviarValorLED(valor);

			sprintf(
				buffer,
				"Valor enviado a ambos contadores: %u\r\n",
				valor
			);

			UART_TransmitString(buffer);
		}

		if (entradaInvalida)
		{
			uint8_t estadoAnterior = SREG;

			cli();
			entradaInvalida = 0;
			SREG = estadoAnterior;

			UART_TransmitString(
				"Entrada invalida. Use un numero entre 0 y 255.\r\n"
			);
		}

		SPI_LeerVoltajes(&mv1, &mv2);

		MostrarVoltaje("Pot1", mv1);
		MostrarVoltaje("Pot2", mv2);

		_delay_ms(500);
	}
}