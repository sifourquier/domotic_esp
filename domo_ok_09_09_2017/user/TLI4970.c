/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *df
 */

/* modified by Simon Fourquier
 * 12/05/2015
 */

#include "TLI4970.h"
#include "version.h"

#define BIT_TLI BIT2

int TLI4970_get_curent()
{   
	bool transfer_succeeded = true;
	int reply=1<<15;

	//spi_init(HSPI,20); //configure SPI 1=20MHZ 2=10MHZ 20=2MHZ

	while(reply&(1<<15)) //lit jusque a avoir le courant
	{
		gpio_output_set(0, BIT_TLI, BIT_TLI, 0);

		reply=spi_transaction(HSPI,0,0x00,0,0x00,0,0x00,16,0); 
		
		gpio_output_set(BIT_TLI, 0, BIT_TLI, 0);
	}

	return (((reply&0x1FFF)-0x1000)*25/2);
}

void gpio16_output_conf()
{
    WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
                   (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1); 	// mux configuration for XPD_DCDC to output rtc_gpio0

    WRITE_PERI_REG(RTC_GPIO_CONF,
                   (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);	//mux configuration for out enable

    WRITE_PERI_REG(RTC_GPIO_ENABLE,
                   (READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe) | (uint32)0x1);	//out enable
}

void gpio16_output_set(uint8 value)
{
    WRITE_PERI_REG(RTC_GPIO_OUT,
                   (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (uint32)(value & 1));
}

void gpio16_input_conf(void)
{
    WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
                   (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1); 	// mux configuration for XPD_DCDC and rtc_gpio0 connection

    WRITE_PERI_REG(RTC_GPIO_CONF,
                   (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);	//mux configuration for out enable

    WRITE_PERI_REG(RTC_GPIO_ENABLE,
                   READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe);	//out disable
}

uint8 gpio16_input_get(void)
{
    return (uint8)(READ_PERI_REG(RTC_GPIO_IN_DATA) & 1);
}

void TLI4970_init()
{
	gpio16_output_conf();
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
}

void TLI4970_enable()
{
#ifndef V1
	gpio16_output_set(0);
	gpio_output_set(BIT_TLI, 0, BIT_TLI, 0);//nCS a 1
#endif
}

void TLI4970_disable()
{
#ifndef V1
	gpio_output_set(0, BIT_TLI, BIT_TLI, 0); //nCS a 0 pour ne pas alimenter le composant ar CS
	gpio16_output_set(1);
#endif
}
