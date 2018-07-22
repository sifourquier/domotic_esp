#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "user_config_mqtt.h"

#define DEBUG_PRINT
//#define SEND_MQTT_ON_UART

#define PUISSANCE_MAX 100000 //big value for special power (disable timer)


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

#ifdef METEO_ADS786X
	#define METEO
	#define ADS786x
#endif

#ifdef METEO
	#define BME280
	#define SLEEP
	#define ON_BATTERY
	#define SLEEP_IF_BATTERY_LOW
#endif

#ifdef PWM_12V_AND_SWITCH_PCF8885
	#define PWM_12V
	#define SWITCH_PCF8885
#endif

#ifdef PWM_12V
	#define PWM
	#define NB_PWM 2
	#define BIT_PWM_VAL 4,5
	#define PIN_FUNC_SELECT_PWM PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4); PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
#endif

#ifdef PWM_TRIACK
	#define PWM
	#define TLI4970_ENABLE
	#define TRIACK
	#define NB_PWM 1
	#define BIT_PWM_VAL 5
	#define TRIACK_ON_OFF 
	#define TRIACK_ON_OFF_PUISSANCE PUISSANCE_MAX
	#define PIN_FUNC_SELECT_PWM PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
#endif

#ifdef PWM_TRIACK_0V_DETECT //detect si le 230v est coupé durant + de 45ms 
	#define PWM
	#define TLI4970_ENABLE
	#define TRIACK
	#define NB_PWM 1
	#define BIT_PWM_VAL 5
	#define TRIACK_ON_OFF 
	#define TRIACK_ON_OFF_PUISSANCE PUISSANCE_MAX
	#define PIN_FUNC_SELECT_PWM PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	#define DETECT_0V
	//#define PRECISION_TIMER 42
	//#define NB_MAX_CALLBACK_TIME 4
#endif

#ifdef STORE
	#define PWM
	#define NB_PWM 2
	//#define TLI4970_ENABLE
	#define TRIACK
	#define STORE_DIR_SENSOR ! //! for inverse or nothing for normal
	#define BIT_PWM_VAL 5,0 //0,5 or 5,0 for change motor rotation
	#define TRIACK_ON_OFF 
	#define TRIACK_ON_OFF_PUISSANCE PUISSANCE_MAX
	#define PIN_FUNC_SELECT_PWM PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0); PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	#define PRECISION_TIMER 100
	#define NB_MAX_CALLBACK_TIME 4
	#define TIMOUT_STORE_START 1500
	#define TIMOUT_STORE 200
	#define PWM_SCALL 100
	#define P_MIN_STORE_UP 60
	#define P_MIN_STORE_DOWN 17
	//#define FACTEUR_P_STORE_UP 10
	//#define FACTEUR_P_STORE_DOWN 10
	/*#if defined(DEBUG_PRINT) || defined(SEND_MQTT_ON_UART)
		#error pour eviter d utiliser l uart alor que tx est utilisé pour le capteur
	#endif*/
#endif

#ifdef SONOFF
	#define RELAY
	#define BIT_RELAY 12
	#define PIN_FUNC_SELECT_RELAY PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
	#define SET_GPIO PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13); GPIO_OUTPUT_SET(13,1);
#endif

#ifdef RADIATOR //for radiator with pilot wire
	#define PRECISION_TIMER 100
	#define TRIACK
	#define TRIACK_ON_OFF 
	#define TRIACK_ON_OFF_PUISSANCE 9000
	#define TRIACK_ON_OFF_INVERSE
	//#define HALF_SIN_POS //off radiator
	#define HALF_SIN_NEG //anti-freeze radiator
#endif

#ifdef SWITCH_PCF8885
#ifndef PWM
	#define SLEEP
	#define SWITCH_ONLY
	#define ON_BATTERY
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
#define TRIAC_MIN 900
#define TRIAC_SCALL 83
#define MAX_TOPIC_LENGTH 128
#define TIMOUT_CONNECTION 40000

#ifndef PWM_SCALL
	#define PWM_SCALL 5
#endif
#ifndef PWM_SCALL_SLOW
	#define PWM_SCALL_SLOW PWM_SCALL*4
#endif

#define MAX_ID_LENGTH 16

#define ADRESSE_CONFIG_IN_FLASH 0x7A000

#define PCF8885_ADRESSE 0x40
#define I2C_READ 0x01
#define I2C_WRITE 0x00

#ifndef PRECISION_TIMER
	#define PRECISION_TIMER 21 //us a 17 on a des reset
#endif
#ifndef NB_MAX_CALLBACK_TIME
	#define NB_MAX_CALLBACK_TIME 3
#endif

#endif

