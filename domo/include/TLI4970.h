#ifndef __TLI4970_H__
#define __TLI4970_H__

#include "driver/spi.h"

int TLI4970_get_curent();
void TLI4970_init();
void TLI4970_enable();
void TLI4970_disable();
void gpio16_output_conf();
void gpio16_output_set(uint8 value);
void gpio16_input_conf(void);
uint8 gpio16_input_get(void);

#endif
