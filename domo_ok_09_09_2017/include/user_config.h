#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "user_config_mqtt.h"

#define DEBUG_PRINT

#ifdef SWITCHQT1040
	#define QT1040_ENABLE
	#define SLEEP
	#define SEND_SWITCH_AT_START
#endif

#ifdef PRISE
	#define TLI4970_ENABLE
	#define TRIACK
#endif

#ifdef PRISEV2
	#define TLI4970_ENABLE
	#define TRIACK
#endif

#ifdef METEO
	#define BME280
	#define SLEEP
	#define ON_BATTERY
#endif

#ifdef PWM_12V_AND_SWITCH_PCF8885
	#define PWM_12V
	#define SWITCH_PCF8885
#endif

#ifdef PWM_12V
	#define PWM
	#define NB_PWM 2
	#define BIT_PWM_TEST 4,5
#endif

#ifdef PWM_TRIACK
	#define PWM
	#define TLI4970_ENABLE
	#define TRIACK
	#define NB_PWM 1
	#define BIT_PWM_TEST 5
	#define TRIACK_ON_OFF 
#endif

#ifdef SWITCH_PCF8885
#ifndef PWM
	#define SLEEP
	#define SWITCH_ONLY
	//#define ON_BATTERY
#endif
	#define PCF8885
	#define I2C_MASTER_SDA_MUX PERIPHS_IO_MUX_MTMS_U
	#define I2C_MASTER_SCL_MUX PERIPHS_IO_MUX_MTDI_U
	#define I2C_MASTER_SDA_GPIO 14
	#define I2C_MASTER_SCL_GPIO 12
	#define I2C_MASTER_SDA_FUNC FUNC_GPIO14
	#define I2C_MASTER_SCL_FUNC FUNC_GPIO12
#ifndef MASQUE_SWITCH
	#define MASQUE_SWITCH 0x0F
#endif
#endif

#ifdef SLEEP
	#define MQTT_TIME_TRY_CONNECT 50 /*ms*/
#else
	#define MQTT_TIME_TRY_CONNECT 500 /*ms*/
#endif

#define T_LONG_SLEEP (30*60) //si la batery est faible sleep de 30min
//#define SEND_MQTT_ON_UART

#define TRIAC_MIN 900
#define TRIAC_SCALL 83
#define MAX_TOPIC_LENGTH 128
#define PUISSANCE_MAX 100000
#define TIMOUT_CONNECTION 40000

#define PWM_SCALL 5
#define PWM_SCALL_SLOW 20

#define MAX_ID_LENGTH 16

#define ADRESSE_CONFIG_IN_FLASH 0x3C000

#define PCF8885_ADRESSE 0x40
#define I2C_READ 0x01
#define I2C_WRITE 0x00

#endif

