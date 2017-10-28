#ifndef WS2812
#define WS2812

#define MAX_LED 20

typedef union
{
	unsigned char rvb[3];
	int integer;
}t_color;

void WS2812_send(t_color* color_leds,unsigned int nb_led);

#endif