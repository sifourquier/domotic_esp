#ifndef __TIMER_H__
#define __TIMER_H__
typedef void (*t_call_back) (void);

void timer_init(uint16 _us);
void timer_add_callback(unsigned int period, t_call_back call_back);
void timer_change(unsigned int period, t_call_back call_back);
void timer_remove_callback(t_call_back call_back);
void timer_tim1_intr_handler(void);

#endif


