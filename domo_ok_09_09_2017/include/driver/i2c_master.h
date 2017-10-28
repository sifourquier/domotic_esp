#ifndef __I2C_MASTER_H__
#define __I2C_MASTER_H__

#include "user_config.h"

//#define I2C_MASTER_SDA_MUX PERIPHS_IO_MUX_GPIO2_U
//#define I2C_MASTER_SCL_MUX PERIPHS_IO_MUX_GPIO0_U
//#define I2C_MASTER_SDA_GPIO 2
//#define I2C_MASTER_SCL_GPIO 0
//#define I2C_MASTER_SDA_FUNC FUNC_GPIO2
//#define I2C_MASTER_SCL_FUNC FUNC_GPIO0

#ifdef I2C_MASTER_SDA_MUX
#define I2C_MASTER_GPIO_SET(pin)  \
    gpio_output_set(1<<pin,0,1<<pin,0)

#define I2C_MASTER_GPIO_CLR(pin) \
    gpio_output_set(0,1<<pin,1<<pin,0)

#define I2C_MASTER_GPIO_OUT(pin,val) \
    if(val) I2C_MASTER_GPIO_SET(pin);\
    else I2C_MASTER_GPIO_CLR(pin)
#endif

#define I2C_MASTER_SDA_HIGH_SCL_HIGH()  \
    gpio_output_set(1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0)

#define I2C_MASTER_SDA_HIGH_SCL_LOW()  \
    gpio_output_set(1<<I2C_MASTER_SDA_GPIO, 1<<I2C_MASTER_SCL_GPIO, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0)

#define I2C_MASTER_SDA_LOW_SCL_HIGH()  \
    gpio_output_set(1<<I2C_MASTER_SCL_GPIO, 1<<I2C_MASTER_SDA_GPIO, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0)

#define I2C_MASTER_SDA_LOW_SCL_LOW()  \
    gpio_output_set(0, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 1<<I2C_MASTER_SDA_GPIO | 1<<I2C_MASTER_SCL_GPIO, 0)

void i2c_master_gpio_init(void);
void i2c_master_init(void);

//#define i2c_master_wait    os_delay_us
void i2c_master_stop(void);
void i2c_master_start(void);
void i2c_master_setAck(unsigned char level);
unsigned char i2c_master_getAck(void);
unsigned char i2c_master_readByte(void);
void i2c_master_writeByte(unsigned char wrdata);

unsigned char i2c_master_checkAck(void);
void i2c_master_send_ack(void);
void i2c_master_send_nack(void);

#endif
