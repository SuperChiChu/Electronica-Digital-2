#ifndef DISPLAY_H
#define DISPLAY_H

/* ========================================================
   Libreria: Display 7 segmentos (anodo comun)
   ========================================================
     a > D10   b > D11   c > D12   d > D13
     e > A3    f > A4    g > A5
   ======================================================== */

/* Configura los pines del display como salidas y lo apaga.*/
void display_init(void);

/* Apaga todos los segmentos del display */
void display_apagar(void);

void display_mostrar_digito(int d);

#endif