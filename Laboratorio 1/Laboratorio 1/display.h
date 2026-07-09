#ifndef DISPLAY_H
#define DISPLAY_H
 
/* ========================================================
   Libreria: Display 7 segmentos (anodo comun)
   ========================================================
   Segmentos conectados asi (Arduino Nano):
     a -> D10   b -> D11   c -> D12   d -> D13
     e -> A3    f -> A4    g -> A5
   ======================================================== */
 
/* Configura los pines del display como salidas y lo apaga.
   Debe llamarse una sola vez, antes de usar las demas funciones. */
void display_init(void);
 
/* Apaga todos los segmentos del display */
void display_apagar(void);
 
/* Muestra un digito (0-9) en el display.
   Si 'd' esta fuera de ese rango, apaga el display. */
void display_mostrar_digito(int d);
 
#endif /* DISPLAY_H */