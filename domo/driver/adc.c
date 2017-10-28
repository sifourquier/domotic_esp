//trouvé sur https://github.com/mattcallow/esp8266-sdk/blob/master/apps/07switch/driver/adc.c
//semble plus rapide que la fonction d'origine

/*
 * adc.c
 *
 *  Created on: 13/02/2015
 *      Author: PV`
 *
 */

#include "ets_sys.h"
#include "osapi.h"

uint16 adc_read(void)
{
  uint8 i;
  uint16 sar_dout, tout, sardata[8];
 
  rom_i2c_writeReg_Mask(0x6C,2,0,5,5,1);
 
  // disable interrupts?
  SET_PERI_REG_MASK(0x60000D5C, 0x200000);
  while (GET_PERI_REG_BITS(0x60000D50, 26, 24) > 0); //wait r_state == 0
  sar_dout = 0;
  CLEAR_PERI_REG_MASK(0x60000D50, 0x02); //force_en=0
  SET_PERI_REG_MASK(0x60000D50, 0x02); //force_en=1
 
  os_delay_us(2);
  while (GET_PERI_REG_BITS(0x60000D50, 26, 24) > 0); //wait r_state == 0
  read_sar_dout(sardata);
 
  // Could this be averaging 8 readings? If anyone has any info please comment.
  for (i = 0; i < 8; i++) {
    sar_dout += sardata[i];
  }
  tout = (sar_dout + 8) >> 4; //tout is 10 bits fraction
 
  // I assume this code re-enables interrupts
  rom_i2c_writeReg_Mask(0x6C,2,0,5,5,1);
  while (GET_PERI_REG_BITS(0x60000D50, 26, 24) > 0); //wait r_state == 0
  CLEAR_PERI_REG_MASK(0x60000D5C, 0x200000);
  SET_PERI_REG_MASK(0x60000D60, 0x1); //force_en=1
  CLEAR_PERI_REG_MASK(0x60000D60, 0x1); //force_en=1
  return tout; //tout is 10 bits fraction
}