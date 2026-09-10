#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

#define btn1 PD2
#define btn2 PD3
#define btn3 PD4
#define btn4 PD5
#define btn5 PD6
#define btn6 PD7

void uart_init(void)
{
	UBRR0H = 0;
	UBRR0L = 103;

	UCSR0B = (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void uart_send(uint8_t data)
{
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = data;
}

int main(void)
{
	DDRD &= ~((1 << btn1) |
	(1 << btn2) |
	(1 << btn3) |
	(1 << btn4) |
	(1 << btn5) |
	(1 << btn6));

	PORTD |= (1 << btn1) |
	(1 << btn2) |
	(1 << btn3) |
	(1 << btn4) |
	(1 << btn5) |
	(1 << btn6);

	uart_init();

	uint8_t last = 0xFF;

	while (1)
	{
		uint8_t pind = PIND;

		if (!(pind & (1 << btn1)) && (last & (1 << btn1)))
		uart_send('1');

		if (!(pind & (1 << btn2)) && (last & (1 << btn2)))
		uart_send('2');

		if (!(pind & (1 << btn3)) && (last & (1 << btn3)))
		uart_send('3');

		if (!(pind & (1 << btn4)) && (last & (1 << btn4)))
		uart_send('4');

		if (!(pind & (1 << btn5)) && (last & (1 << btn5)))
		uart_send('5');

		if (!(pind & (1 << btn6)) && (last & (1 << btn6)))
		uart_send('6');

		last = pind;

		_delay_ms(50);
	}
}