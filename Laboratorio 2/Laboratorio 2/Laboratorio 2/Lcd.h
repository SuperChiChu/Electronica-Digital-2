/*
 * lcd.h
 *
 * Libreria para LCD HD44780 en modo 8 bits, solo escritura
 */

#ifndef LCD_H_
#define LCD_H_

#include <stdint.h>

/* Inicializa el LCD (secuencia de arranque HD44780) */
void lcd_init(void);

/* Envia un byte de comando (RS = 0) */
void lcd_command(uint8_t cmd);

/* Envia un byte de dato / caracter (RS = 1) */
void lcd_data(uint8_t data);

/* Escribe una cadena terminada en '\0' en la posicion actual del cursor */
void lcd_string(const char *str);

/* Ubica el cursor en columna (0..19) y fila (0..3) */
void lcd_gotoxy(uint8_t col, uint8_t row);

/* Limpia toda la pantalla y regresa el cursor a home */
void lcd_clear(void);

/* Regresa el cursor a la posicion home (0,0) sin borrar contenido */
void lcd_home(void);

#endif /* LCD_H_ */