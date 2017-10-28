/******************************************************************************
* Copyright 2013-2014 Espressif Systems (Wuxi)
*
* FileName: pwm.c
*
* Description: pwm driver
*
* Modification history:
*     2014/5/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"

#include "driver/uart.h"

#define DEBUG(x) uart0_sendStr(x)

#include "user_interface.h"
#include "driver/timer.h"

#define NB_MAX_CALLBACK_TIME 3

#define FRC1_ENABLE_TIMER  BIT7

#define US_TO_RTC_TIMER_TICKS(t)          \
    ((t) ?                                   \
     (((t) > 0x35A) ?                   \
      (((t)>>2) * ((APB_CLK_FREQ>>4)/250000) + ((t)&0x3) * ((APB_CLK_FREQ>>4)/1000000))  :    \
      (((t) *(APB_CLK_FREQ>>4)) / 1000000)) :    \
     0)
     
     
typedef struct
{
	unsigned int period;
	int cpt;
	t_call_back call_back;
} callback_time;

callback_time tab_callback_time[NB_MAX_CALLBACK_TIME];

unsigned int us;
unsigned int ticks;

//TIMER PREDIVED MODE
typedef enum {
    DIVDED_BY_1 = 0,		//timer clock
    DIVDED_BY_16 = 4,	//divided by 16
    DIVDED_BY_256 = 8,	//divided by 256
} TIMER_PREDIVED_MODE;

typedef enum {			//timer interrupt mode
    TM_LEVEL_INT = 1,	// level interrupt
    TM_EDGE_INT   = 0,	//edge interrupt
} TIMER_INT_MODE;



/******************************************************************************
* FunctionName : pwm_period_timer
* Description  : pwm period timer function, output high level,
*                start each channel's high level timer
* Parameters   : NONE
* Returns      : NONE
*******************************************************************************/
void timer_tim1_intr_handler(void)
{
	unsigned int l;
	//RTC_CLR_REG_MASK(FRC1_INT_ADDRESS, FRC1_INT_CLR_MASK);
	RTC_REG_WRITE(FRC1_LOAD_ADDRESS, ticks);
	for(l=0;l<NB_MAX_CALLBACK_TIME;l++)
	{
		if(tab_callback_time[l].call_back)
		{
			tab_callback_time[l].cpt--;
			if(!tab_callback_time[l].cpt)
			{
				tab_callback_time[l].cpt=tab_callback_time[l].period;
				tab_callback_time[l].call_back();
			}
		}
	}
}
void timer_change(unsigned int period, t_call_back call_back)
{
	unsigned int l;
	for(l=0;l<NB_MAX_CALLBACK_TIME;l++)
	{
		if(tab_callback_time[l].call_back==call_back)
		{
			tab_callback_time[l].period=period/us;
			tab_callback_time[l].cpt=tab_callback_time[l].period;
			return;
		}
	}
}

void timer_remove_callback(t_call_back call_back)
{
	unsigned int l;
	for(l=0;l<NB_MAX_CALLBACK_TIME;l++)
	{
		if(tab_callback_time[l].call_back==call_back)
		{
			tab_callback_time[l].cpt=1000000;
			tab_callback_time[l].call_back=NULL;
			return;
		}
	}
}

void timer_add_callback(unsigned int period, t_call_back call_back)
{
	unsigned int l;
	for(l=0;l<NB_MAX_CALLBACK_TIME;l++)
	{
		if(tab_callback_time[l].call_back==NULL)
		{
			tab_callback_time[l].period=period/us;
			tab_callback_time[l].cpt=tab_callback_time[l].period;
			tab_callback_time[l].call_back=call_back;
			return;
		}
	}
}

void ICACHE_FLASH_ATTR
timer_init(uint16 _us)
{
    uint8 i;
	unsigned int l;

	for(l=0;l<NB_MAX_CALLBACK_TIME;l++)
		tab_callback_time[l].call_back=NULL;
		
	us=_us;

	RTC_REG_WRITE(FRC1_CTRL_ADDRESS,
                  DIVDED_BY_16
                  | FRC1_ENABLE_TIMER
                  | TM_EDGE_INT);
	
	ticks=US_TO_RTC_TIMER_TICKS(us);
	ticks-=5;//temp irq;
    RTC_REG_WRITE(FRC1_LOAD_ADDRESS, ticks);	//first update finished,start


    ETS_FRC_TIMER1_NMI_INTR_ATTACH(timer_tim1_intr_handler);
    TM1_EDGE_INT_ENABLE();
    ETS_FRC1_INTR_ENABLE();
}

