/*
****************************************************************************
* Copyright (C) 2014 - 2015 Bosch Sensortec GmbH
*
* bme280_support.c
* Date: 2015/03/27
* Revision: 1.0.5 $
*
* Usage: Sensor Driver support file for BME280 sensor
*
****************************************************************************
* License:
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*   Redistributions of source code must retain the above copyright
*   notice, this list of conditions and the following disclaimer.
*
*   Redistributions in binary form must reproduce the above copyright
*   notice, this list of conditions and the following disclaimer in the
*   documentation and/or other materials provided with the distribution.
*
*   Neither the name of the copyright holder nor the names of the
*   contributors may be used to endorse or promote products derived from
*   this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER
* OR CONTRIBUTORS BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
* OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
* ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*
* The information provided is believed to be accurate and reliable.
* The copyright holder assumes no responsibility
* for the consequences of use
* of such information nor for any infringement of patents or
* other rights of third parties which may result from its use.
* No license is granted by implication or otherwise under any patent or
* patent rights of the copyright holder.
**************************************************************************/
/*---------------------------------------------------------------------------*/
/* Includes*/
/*---------------------------------------------------------------------------*/
#include "bme280.h"
#include "ets_sys.h"
#include "os_type.h"
#include "driver/spi.h"
#include "bme280_user.h"
#include "gpio.h"


#define CS GPIO_OUTPUT_SET(4, 0);
#define nCS GPIO_OUTPUT_SET(4, 1);

/*----------------------------------------------------------------------------*
 *  struct bme280_t parameters can be accessed by using bme280
 *	bme280_t having the following parameters
 *	Bus write function pointer: BME280_WR_FUNC_PTR
 *	Bus read function pointer: BME280_RD_FUNC_PTR
 *	Delay function pointer: delay_msec
 *	I2C address: dev_addr
 *	Chip id of the sensor: chip_id
 *---------------------------------------------------------------------------*/
struct bme280_t bme280;
/* This function is an example for reading sensor data
 *	\param: None
 *	\return: communication result
 */
void ICACHE_FLASH_ATTR bme280_init_user(void)
{

	SPI_routine();
	bme280_init(&bme280);
	bme280_set_power_mode(BME280_NORMAL_MODE);
	bme280_set_oversamp_humidity(BME280_OVERSAMP_1X);
	bme280_set_oversamp_pressure(BME280_OVERSAMP_1X);
	bme280_set_oversamp_temperature(BME280_OVERSAMP_1X);
	bme280_set_standby_durn(BME280_STANDBY_TIME_1_MS);
	//com_rslt += bme280_get_standby_durn(&v_stand_by_time_u8);
	bme280_set_power_mode(BME280_FORCED_MODE);
}

#define MASK_DATA1	0xFF
#define MASK_DATA2	0x80
#define MASK_DATA3	0x7F
/*--------------------------------------------------------------------------*
*	The following function is used to map the I2C bus read, write, delay and
*	device address with global structure bme280
*-------------------------------------------------------------------------*/
//s8 I2C_routine(void) {
/*--------------------------------------------------------------------------*
 *  By using bme280 the following structure parameter can be accessed
 *	Bus write function pointer: BME280_WR_FUNC_PTR
 *	Bus read function pointer: BME280_RD_FUNC_PTR
 *	Delay function pointer: delay_msec
 *	I2C address: dev_addr
 *--------------------------------------------------------------------------*/
/*	bme280.bus_write = BME280_I2C_bus_write;
	bme280.bus_read = BME280_I2C_bus_read;
	bme280.dev_addr = BME280_I2C_ADDRESS2;
	bme280.delay_msec = BME280_delay_msek;

	return BME280_INIT_VALUE;
}

/*---------------------------------------------------------------------------*
 * The following function is used to map the SPI bus read, write and delay
 * with global structure bme280
 *--------------------------------------------------------------------------*/
s8 SPI_routine(void) {
/*--------------------------------------------------------------------------*
 *  By using bme280 the following structure parameter can be accessed
 *	Bus write function pointer: BME280_WR_FUNC_PTR
 *	Bus read function pointer: BME280_RD_FUNC_PTR
 *	Delay function pointer: delay_msec
 *--------------------------------------------------------------------------*/

	bme280.bus_write = BME280_SPI_bus_write;
	bme280.bus_read = BME280_SPI_bus_read;
	bme280.delay_msec = BME280_delay_msek;

	return BME280_INIT_VALUE;
}

/************** I2C/SPI buffer length ******/
#define SPI_BUFFER_LEN 64

/*-------------------------------------------------------------------*
*	This is a sample code for read and write the data by using I2C/SPI
*	Use either I2C or SPI based on your need
*	The device address defined in the bme280.h file
*-----------------------------------------------------------------------*/
 /*	\Brief: The function is used as I2C bus write
 *	\Return : Status of the I2C write
 *	\param dev_addr : The device address of the sensor
 *	\param reg_addr : Address of the first register, will data is going to be written
 *	\param reg_data : It is a value hold in the array,
 *		will be used for write the value into the register
 *	\param cnt : The no of byte of data to be write
 */
/*s8 BME280_I2C_bus_write(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt)
{
	s32 iError = BME280_INIT_VALUE;
	u8 array[I2C_BUFFER_LEN];
	u8 stringpos = BME280_INIT_VALUE;
	array[BME280_INIT_VALUE] = reg_addr;
	for (stringpos = BME280_INIT_VALUE; stringpos < cnt; stringpos++) {
		array[stringpos + BME280_ONE_U8X] = *(reg_data + stringpos);
	}
	/*
	* Please take the below function as your reference for
	* write the data using I2C communication
	* "IERROR = I2C_WRITE_STRING(DEV_ADDR, ARRAY, CNT+1)"
	* add your I2C write function here
	* iError is an return value of I2C read function
	* Please select your valid return value
	* In the driver SUCCESS defined as 0
    * and FAILURE defined as -1
	* Note :
	* This is a full duplex operation,
	* The first read data is discarded, for that extra write operation
	* have to be initiated. For that cnt+1 operation done in the I2C write string function
	* For more information please refer data sheet SPI communication:
	*/
	/*return (s8)iError;
}

 /*	\Brief: The function is used as I2C bus read
 *	\Return : Status of the I2C read
 *	\param dev_addr : The device address of the sensor
 *	\param reg_addr : Address of the first register, will data is going to be read
 *	\param reg_data : This data read from the sensor, which is hold in an array
 *	\param cnt : The no of data byte of to be read
 */
/*s8 BME280_I2C_bus_read(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt)
{
	s32 iError = BME280_INIT_VALUE;
	u8 array[I2C_BUFFER_LEN] = {BME280_INIT_VALUE};
	u8 stringpos = BME280_INIT_VALUE;
	array[BME280_INIT_VALUE] = reg_addr;
	/* Please take the below function as your reference
	 * for read the data using I2C communication
	 * add your I2C rad function here.
	 * "IERROR = I2C_WRITE_READ_STRING(DEV_ADDR, ARRAY, ARRAY, 1, CNT)"
	 * iError is an return value of SPI write function
	 * Please select your valid return value
     * In the driver SUCCESS defined as 0
     * and FAILURE defined as -1
	 */
	/*for (stringpos = BME280_INIT_VALUE; stringpos < cnt; stringpos++) {
		*(reg_data + stringpos) = array[stringpos];
	}
	return (s8)iError;
}

/*	\Brief: The function is used as SPI bus read
 *	\Return : Status of the SPI read
 *	\param dev_addr : The device address of the sensor
 *	\param reg_addr : Address of the first register, will data is going to be read
 *	\param reg_data : This data read from the sensor, which is hold in an array
 *	\param cnt : The no of byte of data to be read
 */
s8 ICACHE_FLASH_ATTR BME280_SPI_bus_read(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt)
{
	s32 iError=BME280_INIT_VALUE;
	u8 array[SPI_BUFFER_LEN]={MASK_DATA1};
	u8 stringpos;
	/*	For the SPI mode only 7 bits of register addresses are used.
	The MSB of register address is declared the bit what functionality it is
	read/write (read as 1/write as BME280_INIT_VALUE)*/
	array[BME280_INIT_VALUE] = reg_addr|MASK_DATA2;/*read routine is initiated register address is mask with 0x80*/

	CS;
	spi_read(array,cnt+1);
	nCS;
	/*
	* Please take the below function as your reference for
	* read the data using SPI communication
	* " IERROR = SPI_READ_WRITE_STRING(ARRAY, ARRAY, CNT+1)"
	* add your SPI read function here
	* iError is an return value of SPI read function
	* Please select your valid return value
	* In the driver SUCCESS defined as 0
    * and FAILURE defined as -1
	* Note :
	* This is a full duplex operation,
	* The first read data is discarded, for that extra write operation
	* have to be initiated. For that cnt+1 operation done in the SPI read
	* and write string function
	* For more information please refer data sheet SPI communication:
	*/
	for (stringpos = BME280_INIT_VALUE; stringpos < cnt; stringpos++) {
		*(reg_data + stringpos) = array[stringpos+1];
	}
	return (s8)iError;
}

/*	\Brief: The function is used as SPI bus write
 *	\Return : Status of the SPI write
 *	\param dev_addr : The device address of the sensor
 *	\param reg_addr : Address of the first register, will data is going to be written
 *	\param reg_data : It is a value hold in the array,
 *		will be used for write the value into the register
 *	\param cnt : The no of byte of data to be write
 */
s8 ICACHE_FLASH_ATTR BME280_SPI_bus_write(u8 dev_addr, u8 reg_addr, u8 *reg_data, u8 cnt)
{
	s32 iError = 0;
	u8 array[SPI_BUFFER_LEN * 2];
	u8 stringpos = BME280_INIT_VALUE;
	for (stringpos = BME280_INIT_VALUE; stringpos < cnt; stringpos++) {
		/* the operation of (reg_addr++)&0x7F done: because it ensure the
		   BME280_INIT_VALUE and 1 of the given value
		   It is done only for 8bit operation*/
		array[stringpos * 2] = (reg_addr++) & MASK_DATA3;
		array[stringpos * 2 + 1] = *(reg_data + stringpos);
	}
	
	CS;
	spi_write(array,cnt*2);
	nCS;

	/* Please take the below function as your reference
	 * for write the data using SPI communication
	 * add your SPI write function here.
	 * "IERROR = SPI_WRITE_STRING(ARRAY, CNT*2)"
	 * iError is an return value of SPI write function
	 * Please select your valid return value
	 * In the driver SUCCESS defined as 0
     * and FAILURE defined as -1
	 */
	
	return (s8)iError;
}

/*	Brief : The delay routine
 *	\param : delay in ms
*/
void ICACHE_FLASH_ATTR BME280_delay_msek(unsigned short msek)
{
	os_delay_us(msek*3);
	/*Here you can write your own delay routine*/
}
