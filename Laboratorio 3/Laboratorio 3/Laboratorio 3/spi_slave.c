/*
 * Archivo: spi_slave.c
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

#include "spi_slave.h"
#include <avr/interrupt.h>

#define SPI_COMANDO_LEER_ADC  0xA5
#define SPI_COMANDO_LEDS      0x5A

#define SPI_ESTADO_ESPERANDO  0
#define SPI_ESTADO_ENVIANDO   1
#define SPI_ESTADO_RECIBE_LED 2

/*
 * Trama:
 * [mv1_alto, mv1_bajo, mv2_alto, mv2_bajo]
 */
static volatile uint8_t trama[4] = {0, 0, 0, 0};

static volatile uint8_t indice = 0;
static volatile uint8_t estadoSPI = SPI_ESTADO_ESPERANDO;

static volatile uint8_t valorLEDRecibido = 0;
static volatile uint8_t nuevoValorLED = 0;


void SPI_Slave_Init(void)
{
	/*
	 * PB4 = MISO como salida.
	 * PB3 = MOSI como entrada.
	 * PB5 = SCK como entrada.
	 * PB2 = SS como entrada.
	 */
	DDRB |= (1 << PB4);

	DDRB &= ~(
		(1 << PB3) |
		(1 << PB5) |
		(1 << PB2)
	);

	indice = 0;
	estadoSPI = SPI_ESTADO_ESPERANDO;

	/*
	 * Valor inicial de respuesta.
	 */
	SPDR = 0x00;

	/*
	 * SPI habilitado en modo esclavo.
	 * Interrupci?n SPI habilitada.
	 * Modo SPI 0.
	 */
	SPCR = (1 << SPE) | (1 << SPIE);
}


void SPI_Slave_ActualizarTrama(
	uint8_t b0,
	uint8_t b1,
	uint8_t b2,
	uint8_t b3
)
{
	/*
	 * Evitamos que la ISR lea la trama mientras se est? actualizando.
	 */
	uint8_t estadoAnterior = SREG;

	cli();

	trama[0] = b0;
	trama[1] = b1;
	trama[2] = b2;
	trama[3] = b3;

	SREG = estadoAnterior;
}


uint8_t SPI_Slave_LeerValorLED(uint8_t *valor)
{
	uint8_t disponible = 0;
	uint8_t estadoAnterior = SREG;

	cli();

	if (nuevoValorLED)
	{
		*valor = valorLEDRecibido;
		nuevoValorLED = 0;
		disponible = 1;
	}

	SREG = estadoAnterior;

	return disponible;
}


ISR(SPI_STC_vect)
{
	uint8_t recibido = SPDR;

	switch (estadoSPI)
	{
		case SPI_ESTADO_ESPERANDO:

			if (recibido == SPI_COMANDO_LEER_ADC)
			{
				/*
				 * El maestro pidi? la trama de los ADC.
				 */
				indice = 0;
				estadoSPI = SPI_ESTADO_ENVIANDO;
				SPDR = trama[0];
			}
			else if (recibido == SPI_COMANDO_LEDS)
			{
				/*
				 * El pr?ximo byte ser? el n?mero para los LEDs.
				 */
				estadoSPI = SPI_ESTADO_RECIBE_LED;
				SPDR = 0x00;
			}
			else
			{
				SPDR = 0x00;
			}

			break;


		case SPI_ESTADO_ENVIANDO:

			indice++;

			if (indice < 4)
			{
				SPDR = trama[indice];
			}
			else
			{
				/*
				 * Ya se mandaron los cuatro bytes.
				 */
				indice = 0;
				estadoSPI = SPI_ESTADO_ESPERANDO;
				SPDR = 0x00;
			}

			break;


		case SPI_ESTADO_RECIBE_LED:

			/*
			 * Guardamos el valor de 8 bits enviado por el maestro.
			 */
			valorLEDRecibido = recibido;
			nuevoValorLED = 1;

			estadoSPI = SPI_ESTADO_ESPERANDO;
			SPDR = 0x00;

			break;


		default:

			estadoSPI = SPI_ESTADO_ESPERANDO;
			SPDR = 0x00;

			break;
	}
}