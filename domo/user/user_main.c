/******************************************************************************
	* Copyright 2013-2014 Espressif Systems (Wuxi)
	*
	* FileName: user_main.c
	*
	* Description: entry file of user application
	*
	* Modification history:
	*     2014/1/1, v1.0 create this file.
	*******************************************************************************/
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "gpio.h"
#include "user_mqtt.h"
#include "../mqtt/include/mqtt.h"
#include "driver/adc.h"
#include "driver/timer.h"
#include "driver/spi.h"
#include "user_config_mqtt.h"
#include "TLI4970.h"
#include "bme280.h"

#include "espconn.h"
#include "driver/uart.h"
#include "driver/ws2812.h"
#include "sin.h"

#ifdef I2C_MASTER_SDA_MUX
	#include "driver/i2c_master.h"
#endif 

#define RECEIVEGPIO_STORE  12
#define FUNC_GPIO_STORE  FUNC_GPIO12
#define PERIPHS_IO_MUX_STORE PERIPHS_IO_MUX_MTDI_U

#define RECEIVEGPIO_STORE2  13
#define FUNC_GPIO_STORE2  FUNC_GPIO13
#define PERIPHS_IO_MUX_STORE2 PERIPHS_IO_MUX_MTCK_U

#define RECEIVEGPIO_ZERO_CROSS  4
#define FUNC_GPIO_ZERO_CROSS  FUNC_GPIO4
#define PERIPHS_IO_MUX_ZERO_CROSS PERIPHS_IO_MUX_GPIO4_U
#define BIT_TRIAC 15

#define RECEIVEGPIO_IRQ_PCF8885  13
#define FUNC_GPIO_IRQ_PCF8885  FUNC_GPIO13
#define PERIPHS_IO_MUX_IRQ_PCF8885 PERIPHS_IO_MUX_MTCK_U3

#define POWER_ON_ADS786x /*GPIO_OUTPUT_SET(0, 0);*/ GPIO_OUTPUT_SET(5, 0);
#define POWER_OFF_ADS786x /*GPIO_OUTPUT_SET(5, 1); GPIO_OUTPUT_SET(0, 1);*/

#define CS_ADS786x GPIO_OUTPUT_SET(5, 0);
#define nCS_ADS786x GPIO_OUTPUT_SET(5, 1);

#ifdef PWM
char BIT_PWM[NB_PWM]={BIT_PWM_VAL};
#ifdef STORE
volatile unsigned int puissance_pwm[NB_PWM]={0};
#else
volatile unsigned int puissance_pwm[NB_PWM]={100};
#endif
#endif
//#define TIME_ANTI_REBOND 50 //ms
#define TIME_SAMPLE_CURENT 500 //us si modifier modifier sin.h en consequance
#define NB_MESURE_ADC_BEFOR_SEND 2000 //1 secondes
#define FREQUANCE_RESEAUX 50
#define NB_ECHANTILLON_FILTRE ((1000000/TIME_SAMPLE_CURENT/FREQUANCE_RESEAUX))

#ifdef SEND_SWITCH_AT_START
const int PORT_QT1040[4]={PERIPHS_IO_MUX_GPIO0_U,PERIPHS_IO_MUX_U0TXD_U,PERIPHS_IO_MUX_GPIO2_U,PERIPHS_IO_MUX_U0RXD_U};
const int PORT_FUNC_QT1040[4]={FUNC_GPIO0,FUNC_GPIO1,FUNC_GPIO2,FUNC_GPIO3};
#endif

#ifdef SLEEP
static os_timer_t timer_DisconnectMQTT;
#endif

static os_timer_t timer_mesure;
static os_timer_t timer_time_out_connection;

bool config_ap();
void save_and_restart();

#ifdef STORE
os_timer_t timer_timout_store;
#endif

#ifdef TLI4970_ENABLE
volatile int moyen_power=0;
os_timer_t timer_send_mesures;
volatile unsigned char TLI4970_is_enable=0;
#endif

MQTT_Client mqttClient;
#ifdef TRIACK
volatile signed int puissance_triac=0;
volatile bool timer_triack_on=0;
#endif

#ifdef STORE
volatile signed int niveau=0,consinge=0;
#endif

#ifdef DETECT_0V
os_timer_t timer_0V;
#endif 

volatile unsigned int temp_from_zero_cross;

#define MAX_SSID_LENGTH 32 //ne pas depasser 32 risque d'over flow dans le SDK
#define MAX_PASSWORD_LENGTH 64 //ne pas depasser 64 risque d'over flow dans le SDK
#define MAX_ADRESSE_LENGTH 16 //XXX.XXX.XXX.XXX
#define MAX_LENGTH_STATION_NAME 32

uint32 ssid32bit[MAX_SSID_LENGTH/4]; //focé alingement sur 4 byte
char* ssid=(char*)ssid32bit; // Wifi SSID
uint32 MYID32bit[MAX_ID_LENGTH/4];
char* MYID=(char*)MYID32bit; // Wifi MYID
uint32 password32bit[MAX_PASSWORD_LENGTH/4];
char* password=(char*)password32bit; // Wifi Password
#define NB_ADRESSE 4 //ip, masq, gateway, serveurMQTT
uint32 adresse32bit[NB_ADRESSE][MAX_ADRESSE_LENGTH/4];
char* adresse[NB_ADRESSE]={(char*)&(adresse32bit[0]),(char*)&(adresse32bit[1]),(char*)&(adresse32bit[2]),(char*)&(adresse32bit[3])};
uint32 StationName32bit[MAX_LENGTH_STATION_NAME/4];
char *StationName=(char*)StationName32bit;
char topic[MAX_TOPIC_LENGTH];

#define echelle t_sleep //nombre de tour pour ouvrir le store
unsigned int t_sleep;


void ICACHE_FLASH_ATTR DisconnectMQTT();

short ubat;

void ZeroCrossIRQ();
void storeIRQ();
void pcf8885func();


#define TO_HEX(i) (i <= 9 ? '0' + i : 'A' - 10 + i)

#define ABS(x) x<0 ? x : -x

typedef enum 
{
	TEMPERATURE=0,
	TEMPERATURE_MIN,
	TEMPERATURE_MAX,
	CURRENT,
	POWER,
	HUMIDITY,
	LIGHT,
	VOLTAGE,
	BATTERY,
	PRESSURE,
	SPEED,
	DIRECTION,
	ICON,
	SWITCH,
	POURCENT,
}sensor_type_t;

char* ICACHE_FLASH_ATTR ID(char* x) //ajoute ID aux string x
{
	static char string[100];
	char* pt_string=string;
	
	while(*x)
	{
		*pt_string=*x;
		pt_string++;
		x++;
	}
	
	x=MYID;
	while(*x)
	{
		*pt_string=*x;
		pt_string++;
		x++;
	}
	*pt_string='\0';
	return string;
}

void ICACHE_FLASH_ATTR print(char* string)
{
//#ifndef STORE
	uart0_sendStr(string);
//#endif
	//mqtt_print(&mqttClient,string);
}

void print_debug(char* string)
{
	#ifdef DEBUG_PRINT
	print(string);
	#endif
}

//paquet = {S,type,id,id,val,val,val,val)
//buffer = buffer de 8 byte
void ICACHE_FLASH_ATTR send_paquet_sensor(unsigned char* topic,sensor_type_t type, int val, int retain,char* name)
{
	int l;
	unsigned char buffer[8+MAX_LENGTH_STATION_NAME];
	if(StationName[0])
	{
		buffer[0]='S';
		buffer[1]=type;
		for(l=0;l<4;l++)
		{
			buffer[2+l]=val>>((3-l)*8);
		}
		l=0;
		while(StationName[l] && l<MAX_LENGTH_STATION_NAME)
		{
			buffer[6+l]=StationName[l];
			l++;
		}
		int l2=0;
		while(name && name[l2] && l<MAX_LENGTH_STATION_NAME)
		{
			buffer[6+l]=name[l2];
			l++;
			l2++;
		}
		buffer[6+l]=0;
		
		MQTT_Publish(&mqttClient, topic, (char*) buffer, 7+l, 0, retain);
	}
}

#ifdef PCF8885
unsigned char old_stat_switch;
unsigned char send_PCF8885()
{
	unsigned char stat_switch; 
	i2c_master_start();
	i2c_master_writeByte(PCF8885_ADRESSE|I2C_READ);
	i2c_master_getAck();
	stat_switch=i2c_master_readByte();
	i2c_master_send_nack();
	i2c_master_stop();	https://www.1000ordi.ch/logitech-wireless-trackball-mx-ergo-910-005179-106244.html
	
	stat_switch=stat_switch&MASQUE_SWITCH;
	
	char name[]="_0";
	
	int l;
	for(l=0;l<sizeof(stat_switch)*8;l++)
	{
		if(((stat_switch>>l)&0x01) != ((old_stat_switch>>l)&0x01))
		{
			name[1]='1'+l;
			send_paquet_sensor(ID("/sensors/switch/"),SWITCH,((stat_switch>>l)&0x01)*100,0,name);
			#ifdef ON_BATTERY
			send_paquet_sensor(ID("/sensors/batterys/"),BATTERY,ubat,1,name);
			#endif
		}
	}
	old_stat_switch=stat_switch;
	return stat_switch;
	
#ifdef SLEEP
	
#endif
}

#endif

char* ICACHE_FLASH_ATTR ad_name(char* x) //ajoute ID aux string x
{
	static char string[100];
	char* pt_string=string;
	
	while(*x)
	{
		*pt_string=*x;
		pt_string++;
		x++;
	}
	
	x=StationName;
	while(*x)
	{
		*pt_string=*x;
		pt_string++;
		x++;
	}
	*pt_string='\0';
	return string;
}

unsigned int stringToUint(char* str)
{
	int val=0;
	while(((*str)>='0') && ((*str)<='9'))
	{
		val=val*10+((*str)-'0');
		str++;
	}
	return val;
}

void ICACHE_FLASH_ATTR int_to_srthex(int val, char* str) //str=tab 10 char
{
	int l;
	
	for(l=0;l<8;l++)
	{
		char temp=(val>>(l*4))&0xF;
		temp=TO_HEX(temp);
		str[8-l]=temp;
	}
	str[0]=' ';
	for(l=0;l<7;l++)
	{
		if(str[l+1]!='0')
			break;
		str[l+1]=' ';
	}
	str[9]=0;
}


typedef enum
{
	WAIT,
	CONFIG_SSID,
	CONFIG_TOPIC,
	CONFIG_T_SLEEP,
	CONFIG_NAME,
	CONFIG_PASSWORD,
	CONFIG_ADDRESSE
} stateRxUart;

void ICACHE_FLASH_ATTR print_menu()
{
	print("\n\ns) modifie ssid\np) modifie password\na) modifie addresse\nn) modifie netmask\ng) modifie gateway\nm) modifie MQTT serveur\ni) modifie name\nx) modifie sleep time [s]\nc) Configure and save\n");
}

void ICACHE_FLASH_ATTR rx_uart0(char c)
{
	static stateRxUart state=WAIT;
	static unsigned char pointeur;
	static int num_adresse;
	static char str_number[10];
	#ifdef SLEEP
	os_timer_disarm(&timer_DisconnectMQTT);
	os_timer_setfn(&timer_DisconnectMQTT, (os_timer_func_t *)DisconnectMQTT, NULL);
	os_timer_arm(&timer_DisconnectMQTT, 3000, 0);
	#endif
	
	switch (state)
	{
		case WAIT:
			switch(c)
			{
				case 's':
				case 'S':
					state=CONFIG_SSID;
					pointeur=0;
					print("SSID=");
					break;
				case 'p':
				case 'P':
					state=CONFIG_PASSWORD;
					pointeur=0;
					print("PASSWORD=");
					break;
				case 'a':
				case 'A':
					state=CONFIG_ADDRESSE;
					pointeur=0;
					num_adresse=0; //IP
					print("empy for DHCP\nIP=");
					break;
				case 'n':
				case 'N':
					state=CONFIG_ADDRESSE;
					pointeur=0;
					num_adresse=1; //Net mask
					print("MASK=");
					break;
				case 'g':
				case 'G':
					state=CONFIG_ADDRESSE;
					pointeur=0;
					num_adresse=2; //Net mask
					print("GW=");
					break;
				case 'm':
				case 'M':
					state=CONFIG_ADDRESSE;
					pointeur=0;
					num_adresse=3; //Net mask
					print("MQTT=");
					break;
				case 'i':
				case 'I':
					state=CONFIG_NAME;
					pointeur=0;
					print("Name (no Name => MQTT disable)=");
					break;
				case 'c':
				case 'C':
					save_and_restart();
					break;
				default:
					print_menu();
					break;
				case 't':
				case 'T':
					state=CONFIG_TOPIC;
					pointeur=0;
					break;
				case 'x':
				case 'X':
					state=CONFIG_T_SLEEP; //can be use for other parametre of sleep need be rename
					pointeur=0;
					print("Time=");
					break;
					
			}
			break;
				case CONFIG_T_SLEEP: //can be use for other parametre of sleep need be rename
					switch(c)
					{
						case '\n':
						case '\r':
							str_number[pointeur]=0;
							state=WAIT;
							t_sleep=stringToUint(str_number);
#ifdef sleep
							if(t_sleep>T_LONG_SLEEP)
								t_sleep=T_LONG_SLEEP;
#endif
							print("\n0x");
							int_to_srthex(t_sleep,str_number);
							print(str_number);
							break;
							
						default:
							str_number[pointeur]=c;
							if(pointeur<sizeof(str_number)-1)
								pointeur++;
							else
								print("Number to high");
							break;
					}
					break;
			
				case CONFIG_TOPIC:
					switch(c)
					{
						case '\n':
						case '\r':
							topic[pointeur]=0;
							state=WAIT;
							//system_rtc_mem_write(64,topic,MAX_TOPIC_LENGTH);
							MQTT_Subscribe(&mqttClient, topic, 0);
#ifdef SLEEP
							os_timer_arm(&timer_DisconnectMQTT, 800, 0);
#endif
							break;
							
						default:
							topic[pointeur]=c;
							if(pointeur<MAX_TOPIC_LENGTH-1)
								pointeur++;
							else
								print("Topic to long");
							break;
					}
					break;
					
						case CONFIG_SSID:
							switch(c)
							{
								case '\n':
								case '\r':
									ssid[pointeur]=0;
									state=WAIT;
									print_menu();
									break;
									
								default:
									ssid[pointeur]=c;
									if(pointeur<MAX_SSID_LENGTH-1)
										pointeur++;
									else
										print("SSID to long");
									break;
							}
							break;
							
								case CONFIG_NAME:
									switch(c)
									{
										case '\n':
										case '\r':
											StationName[pointeur]=0;
											state=WAIT;
											print_menu();
											break;
											
										default:
											StationName[pointeur]=c;
											if(pointeur<MAX_LENGTH_STATION_NAME -1)
												pointeur++;
											else
												print("Name to long");
											break;
									}
									break;
									
										case CONFIG_PASSWORD:
											switch(c)
											{
												case '\n':
												case '\r':
													password[pointeur]=0;
													state=WAIT;
													print_menu();
													break;
													
												default:
													password[pointeur]=c;
													if(pointeur<MAX_PASSWORD_LENGTH-1)
														pointeur++;
													else
														print("Password to long");
													break;
											}
											break;
											
												case CONFIG_ADDRESSE:
													switch(c)
													{
														case '\n':
														case '\r':
															adresse[num_adresse][pointeur]=0;
															state=WAIT;
															print_menu();
															break;
															
														default:
															adresse[num_adresse][pointeur]=c;
															if(pointeur<MAX_PASSWORD_LENGTH-1)
																pointeur++;
															else
																print("Adresse to long");
															break;
													}
													break;
	}
}

void ICACHE_FLASH_ATTR print_config()
{
	print_debug("\nSSID=");
	print_debug(ssid);
	print_debug("\npassword=");
	print_debug(password);
	print_debug("\naddresse=");
	print_debug(adresse[0]);
	print_debug("\nMask=");
	print_debug(adresse[1]);
	print_debug("\nGW=");
	print_debug(adresse[2]);
	print_debug("\nMQTT=");
	print_debug(adresse[3]);
	print_debug("\nNAME=");
	print_debug(StationName);
	print_debug("\nID=");
	print_debug(MYID);
	print_debug("\nSLEEP_TIME=");
	char string[10];
	int_to_srthex(t_sleep,string);
	print_debug(string);
	print_debug("\n");
	
}

void save_and_restart()
{
	print_config();
	
	ETS_INTR_LOCK(); //desactivé tout les interruption quand on fait une comunication sur le SPI
	spi_flash_erase_protect_disable();
	spi_flash_erase_sector(ADRESSE_CONFIG_IN_FLASH>>12);
	spi_flash_write(ADRESSE_CONFIG_IN_FLASH,ssid32bit,MAX_SSID_LENGTH);
	spi_flash_write(ADRESSE_CONFIG_IN_FLASH+MAX_SSID_LENGTH,password32bit,MAX_PASSWORD_LENGTH);
	spi_flash_write(ADRESSE_CONFIG_IN_FLASH+MAX_SSID_LENGTH+MAX_PASSWORD_LENGTH,(uint32*)adresse32bit,MAX_ADRESSE_LENGTH*NB_ADRESSE);
	spi_flash_write(ADRESSE_CONFIG_IN_FLASH+MAX_SSID_LENGTH+MAX_PASSWORD_LENGTH+MAX_ADRESSE_LENGTH*NB_ADRESSE,StationName32bit,MAX_LENGTH_STATION_NAME);
	spi_flash_write(ADRESSE_CONFIG_IN_FLASH+MAX_SSID_LENGTH+MAX_PASSWORD_LENGTH+MAX_ADRESSE_LENGTH*NB_ADRESSE+MAX_LENGTH_STATION_NAME,&t_sleep,sizeof(t_sleep));
	
	print_debug("\n\nCONFIG WRITED\n\n");
	
	ETS_INTR_UNLOCK();
	system_restart();
	
}

struct station_config stationConf; // Station conf struct

bool ICACHE_FLASH_ATTR config_ap()
{
	//wifi_station_disconnect();
	
	print_config();
	
	if(adresse[0][0]) //si adresse non null (pas de DHCP)
	{
		struct ip_info info;
		wifi_station_dhcpc_stop();
		info.ip.addr = ipaddr_addr(adresse[0]);
		info.netmask.addr = ipaddr_addr(adresse[1]);
		info.gw.addr = ipaddr_addr(adresse[2]);
		wifi_set_ip_info(STATION_IF, &info);
		
	}
	else//DHCP
	{
		wifi_set_ip_info(STATION_IF, NULL);
		wifi_station_dhcpc_start();
	}
	
	ssid[MAX_SSID_LENGTH-1]=0; //evite les risque d'overflow
	password[MAX_PASSWORD_LENGTH-1]=0;
		
	wifi_set_opmode(0x01); // Set station mode
	os_memcpy(&stationConf.ssid, ssid32bit, MAX_SSID_LENGTH); // Set settings
	os_memcpy(&stationConf.password, password32bit, MAX_PASSWORD_LENGTH); // Set settings 
	stationConf.bssid_set = 0;
	wifi_station_set_config(&stationConf); // Set wifi conf
	return 1;
}

#ifdef TLI4970_ENABLE
void timerMesureSampleAdc(void)//ne pas apeler de fonction en flash dans une fonction du timer
{	
	if(StationName[0] && mqttIsConected())
	{
		static unsigned int num_mesure=0;
		static long int sum=0;
		
		//static int tab_filtre[NB_ECHANTILLON_FILTRE];
		//adc_value += system_adc_read();
		//DEBUG("send1\n\r");
		
		int val=0;
		
		if(TLI4970_is_enable)
		{
			if(TLI4970_is_enable==2)//si il est alumer depui un moment
			{
				val=TLI4970_get_curent();
			}
			else//si on vien de l'alumer
			{
				TLI4970_is_enable=2;
			}
		}
		else
		{
			TLI4970_disable();
		}
		temp_from_zero_cross++;
		if(temp_from_zero_cross>=40) //fixme use sizeof
			temp_from_zero_cross=0;
		sum+=val*tab_sin[temp_from_zero_cross];
		num_mesure++;
		
		if(num_mesure>=NB_MESURE_ADC_BEFOR_SEND)
		{
			moyen_power=sum/NB_MESURE_ADC_BEFOR_SEND;
			num_mesure=0;
			sum=0;
		}
	}
}
#endif

#ifdef PWM
void timerPwm1()
{
	static unsigned char on=0;
	if(puissance_pwm[0]<2)
	{
		 GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1<<BIT_PWM[0]);
		 timer_change(10000,(t_call_back)timerPwm1);
	}
	else if(puissance_pwm[0]>98)
	{
		GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1<<BIT_PWM[0]);
		timer_change(10000,(t_call_back)timerPwm1);
	}
	else if(on)
	{
		on=0;
		GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1<<BIT_PWM[0]);
		if((puissance_pwm[0]>90)||puissance_pwm[0]<10)
			timer_change((100-puissance_pwm[0])*PWM_SCALL_SLOW,(t_call_back)timerPwm1);// for avert small time (bug)
		else
			timer_change((100-puissance_pwm[0])*PWM_SCALL,(t_call_back)timerPwm1);
	}
	else
	{
		on=1;
		GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1<<BIT_PWM[0]);
		if((puissance_pwm[0]>90)||puissance_pwm[0]<10)
			timer_change(puissance_pwm[0]*PWM_SCALL_SLOW,(t_call_back)timerPwm1);
		else
			timer_change(puissance_pwm[0]*PWM_SCALL,(t_call_back)timerPwm1);
	}
}

void timerPwm2()
{
	static unsigned char on=0;
	if(puissance_pwm[1]<2)
	{
		 GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1<<BIT_PWM[1]);
		 timer_change(10000,(t_call_back)timerPwm2);
	}
	else if(puissance_pwm[1]>98)
	{
		GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1<<BIT_PWM[1]);
		timer_change(10000,(t_call_back)timerPwm2);
	}
	else if(on)
	{
		on=0;
		GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1<<BIT_PWM[1]);
		if((puissance_pwm[1]>90)||puissance_pwm[1]<10)
			timer_change((100-puissance_pwm[1])*PWM_SCALL_SLOW,(t_call_back)timerPwm2);// for avert small time (bug)
		else
			timer_change((100-puissance_pwm[1])*PWM_SCALL,(t_call_back)timerPwm2);
	}
	else
	{
		on=1;
		GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1<<BIT_PWM[1]);
		if((puissance_pwm[1]>90)||puissance_pwm[1]<10)
			timer_change(puissance_pwm[1]*PWM_SCALL_SLOW,(t_call_back)timerPwm2);
		else
			timer_change(puissance_pwm[1]*PWM_SCALL,(t_call_back)timerPwm2);
	}
}
#endif

void IRQ()
{
		//print_debug("i\n");
	uint32 gpio_status;
	do
	{
		gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
		GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);//clear all irq

#ifdef TRIACK
		if(gpio_status & BIT(RECEIVEGPIO_ZERO_CROSS))
		{
			gpio_pin_intr_state_set(GPIO_ID_PIN(RECEIVEGPIO_ZERO_CROSS), GPIO_PIN_INTR_DISABLE); // Interr	
			ZeroCrossIRQ();
		}
#endif

#ifdef STORE
		if(gpio_status & BIT(RECEIVEGPIO_STORE))
			storeIRQ();
#endif
		
#ifdef PCF8885
		if(gpio_status & BIT(RECEIVEGPIO_IRQ_PCF8885))
			pcf8885func();
#endif
	}while(gpio_status);
}

#ifdef TRIACK
void timerEnableTriack()
{
	GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1<<BIT_TRIAC);
}

#ifdef DETECT_0V
void no_zero_cross() //detect if no zero crossing occure (230v stoped)
{
	send_paquet_sensor(ID("/sensors/switch/"),SWITCH,100,0,"_1");
	send_paquet_sensor(ID("/sensors/switch/"),SWITCH,0,0,"_sensor");
	os_timer_disarm(&timer_0V); // dis_arm the timer
}
#endif

#if defined(HALF_SIN_POS) || defined(HALF_SIN_NEG)
void timerDisableTriack()
{
	GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1<<BIT_TRIAC);
	timer_change(1000000,(t_call_back)timerDisableTriack);//1 seconde (disable)
}

void ZeroCross()//après iRQ + après 10ms
{
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(RECEIVEGPIO_ZERO_CROSS));//ici clear uniquement bon gpio
	//gpio_pin_intr_state_set(GPIO_ID_PIN(RECEIVEGPIO_ZERO_CROSS), GPIO_PIN_INTR_NEGEDGE); // Interr	


	if(puissance_triac>0)
	{
		GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1<<BIT_TRIAC);//enable triac
	}
	timer_change(9000,(t_call_back)timerDisableTriack);//9ms
	timer_change(100000,(t_call_back)ZeroCross);

}
#else

void ZeroCross()//après iRQ + après 10ms
{
	#ifdef TLI4970_ENABLE

	if(puissance_triac>0)
	{
		if(!TLI4970_is_enable)
		{
			TLI4970_enable();
			TLI4970_is_enable=1;
		}
	}
	else
	{
		TLI4970_is_enable=0;
	}
	#endif
	
	if(puissance_triac>=PUISSANCE_MAX)
	{
		
		if(timer_triack_on)
		{
			timer_remove_callback((t_call_back)timerEnableTriack);
			timer_triack_on=0;
		}
		//GPIO_OUTPUT_SET(BIT_TRIAC,1); //toujour a 1
 		GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, 1<<BIT_TRIAC);
				//print_debug("0");


	}
	else if(puissance_triac>0)
	{
		//GPIO_OUTPUT_SET(BIT_TRIAC,0);
		GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1<<BIT_TRIAC);


		if(timer_triack_on)
		{
			timer_change(10000-puissance_triac,(t_call_back)timerEnableTriack);
		}
		else
		{
			timer_add_callback(10000-puissance_triac,(t_call_back)timerEnableTriack);
			timer_triack_on=1;
		}
	}
	else
	{
		if(timer_triack_on)
		{
			timer_remove_callback((t_call_back)timerEnableTriack);
			timer_triack_on=0;
		}
		GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, 1<<BIT_TRIAC);
		
	}
}
#endif

void timerZeroCross() //reactivez l'ibnteruption et simule le 2eme zero cross
{	
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(RECEIVEGPIO_ZERO_CROSS));//ici clear uniquement bon gpio
	gpio_pin_intr_state_set(GPIO_ID_PIN(RECEIVEGPIO_ZERO_CROSS), GPIO_PIN_INTR_NEGEDGE); // enable IRQ	*/
	
	timer_change(10000,(t_call_back)timerZeroCross);
	ZeroCross();
}

void ZeroCrossIRQ()
{
	temp_from_zero_cross=0;

#ifdef HALF_SIN_POS
	timer_change(9800,(t_call_back)ZeroCross);
#elif defined(HALF_SIN_NEG)
	timer_change(19800,(t_call_back)ZeroCross);
#else
	timer_change(9800,(t_call_back)timerZeroCross);
#endif
	
#ifdef DETECT_0V
	os_timer_disarm(&timer_0V); // dis_arm the timer
	os_timer_setfn(&timer_0V, (os_timer_func_t *)no_zero_cross, NULL); // set the timer function, dot get os_timer_func_t to force function convert
	os_timer_arm(&timer_0V, 45, 1);//tout les 2seconde
#endif
}


void ICACHE_FLASH_ATTR init_IRQ_zero_cross()
{
	ETS_GPIO_INTR_DISABLE();	
	ETS_GPIO_INTR_ATTACH(IRQ, RECEIVEGPIO_ZERO_CROSS);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_ZERO_CROSS, FUNC_GPIO_ZERO_CROSS);
	//gpio_output_set(0, 0, 0, BIT(RECEIVEGPIO_ZERO_CROSS));
	GPIO_DIS_OUTPUT(RECEIVEGPIO_ZERO_CROSS);
	//PIN_PULLDWN_DIS(PERIPHS_IO_MUX_ZERO_CROSS);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_ZERO_CROSS);
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(RECEIVEGPIO_ZERO_CROSS));
	gpio_pin_intr_state_set(GPIO_ID_PIN(RECEIVEGPIO_ZERO_CROSS), GPIO_PIN_INTR_NEGEDGE); // Interr	

	ETS_GPIO_INTR_ENABLE();
}
#endif

#ifdef STORE
signed int niveau_100=-1;
void storeIRQ()
{
	int delta=consinge-niveau;
	if(puissance_pwm[0])
	{
		if(delta>=0)
		{
			puissance_pwm[0]=0;
#ifdef TRIACK
			puissance_triac=0;
#endif
		}
		else
		{
			if(delta<-(3*echelle/100))
				puissance_pwm[0]=100;
			else
				puissance_pwm[0]=P_MIN_STORE_DOWN;
		}
	}
	if(puissance_pwm[1])
	{
		if(delta<=0)
		{
			puissance_pwm[1]=0;
#ifdef TRIACK
			puissance_triac=0;
#endif
		}
		else
		{
			if(delta>(3*echelle/100))
				puissance_pwm[1]=100;
			else
				puissance_pwm[1]=P_MIN_STORE_UP;
		}
	}
		
	if(GPIO_INPUT_GET(GPIO_ID_PIN(RECEIVEGPIO_STORE))==0) //anti rebond
	{
		if(STORE_DIR_SENSOR GPIO_INPUT_GET(GPIO_ID_PIN(RECEIVEGPIO_STORE2)))
		{
			if(niveau<consinge) //si on tourne dans le bon sense
			{
				os_timer_disarm(&timer_timout_store);
				os_timer_arm(&timer_timout_store, TIMOUT_STORE, 1); //reset timeout
			}
			//print_debug("-\n");
			niveau++;
		}
		else
		{
			if(niveau>consinge) //si on tourne dans le bon sense
			{
				os_timer_disarm(&timer_timout_store);
				os_timer_arm(&timer_timout_store, TIMOUT_STORE, 1); //reset timeout
			}
			//print_debug(" +\n");
			niveau--;
		}
	
		
		signed int niveau_100_temp=(signed int)niveau*100/(signed int)echelle;
		if(niveau_100_temp>100)
			niveau_100_temp=100;
		else if (niveau_100_temp<0)
			niveau_100_temp=0;
		if(niveau_100_temp!=niveau_100)
		{
			//char test[10];
			niveau_100=niveau_100_temp;
			/*int_to_srthex(niveau,test);
			print_debug("\n");
			print_debug(test);
			int_to_srthex(niveau_100,test);
			print_debug("\na");
			print_debug(test);*/

			send_paquet_sensor(ID("/sensors/switch/"),SWITCH,niveau_100,0,"_MsenS_");
		}
	}
}

void ICACHE_FLASH_ATTR init_IRQ_store()
{
	ETS_GPIO_INTR_DISABLE();	
	ETS_GPIO_INTR_ATTACH(IRQ, RECEIVEGPIO_STORE);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_STORE, FUNC_GPIO_STORE);
	GPIO_DIS_OUTPUT(RECEIVEGPIO_STORE);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_STORE);
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(RECEIVEGPIO_STORE));
	gpio_pin_intr_state_set(GPIO_ID_PIN(RECEIVEGPIO_STORE), GPIO_PIN_INTR_NEGEDGE); // Interr	
	
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_STORE2, FUNC_GPIO_STORE2);
	GPIO_DIS_OUTPUT(RECEIVEGPIO_STORE2);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_STORE2);
	
	ETS_GPIO_INTR_ENABLE();
}
#endif

#ifdef PCF8885

void pcf8885func()
{
	//print_debug("I\n");
	unsigned char ret_switch=send_PCF8885();
#ifdef SWITCH_ONLY
	//char test[10]="p";
	//int_to_srthex(ret_switch,test+1);
	//print_debug(test);
	if(ret_switch==0)
	{
		print_debug("off\n");
		os_timer_disarm(&timer_mesure);
		GPIO_DIS_OUTPUT(RECEIVEGPIO_IRQ_PCF8885);
		os_timer_disarm(&timer_mesure); // dis_arm the timer
		os_timer_setfn(&timer_mesure, (os_timer_func_t *)DisconnectMQTT, NULL); // set the timer function, dot get os_timer_func_t to force function convert
		os_timer_arm(&timer_mesure, 200, 1);	
	}
#else
	//GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(RECEIVEGPIO_IRQ_PCF8885));//fait dans irq
#endif
}

void ICACHE_FLASH_ATTR init_IRQ_pcf8885()
{
	ETS_GPIO_INTR_DISABLE();	
	ETS_GPIO_INTR_ATTACH(IRQ, RECEIVEGPIO_IRQ_PCF8885);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_IRQ_PCF8885, FUNC_GPIO_IRQ_PCF8885);
	//gpio_output_set(0, 0, 0, BIT(RECEIVEGPIO_IRQ_PCF8885));
	GPIO_DIS_OUTPUT(RECEIVEGPIO_IRQ_PCF8885);
	//PIN_PULLDWN_DIS(PERIPHS_IO_MUX_IRQ_PCF8885);
	PIN_PULLUP_EN(PERIPHS_IO_MUX_IRQ_PCF8885);
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(RECEIVEGPIO_IRQ_PCF8885));
	gpio_pin_intr_state_set(GPIO_ID_PIN(RECEIVEGPIO_IRQ_PCF8885), GPIO_PIN_INTR_LOLEVEL); // Interr	
	
	ETS_GPIO_INTR_ENABLE();
}
#endif

#ifdef BME280

void timer_mesure_handler(void *arg)
{
	unsigned int pressure;
	int temperature;
	int humidity;
	
	#ifdef ADS786x
		CS_ADS786x;
		signed short val_adc;
		unsigned char tab[2];
		spi_read(tab,2);
		val_adc=tab[0]<<8|tab[1];
		char test[10];
		int_to_srthex(val_adc,test);
		print_debug("\n ADC=");
		val_adc=3200-val_adc;
		val_adc=val_adc/2;
		if(val_adc<0)
			val_adc=0;
		if(val_adc>1000)
			val_adc=1000;
		send_paquet_sensor(ID("/sensors/humiditys/"),HUMIDITY,val_adc*10,1,"_sol");
		print_debug(test);
		nCS_ADS786x;
	#endif
		
	bme280_read_pressure_temperature_humidity(&pressure,&temperature, &humidity);
	bme280_set_power_mode(BME280_SLEEP_MODE);
	
	send_paquet_sensor(ID("/sensors/temperatures/"),TEMPERATURE,temperature,1,NULL);
	send_paquet_sensor(ID("/sensors/humiditys/"),HUMIDITY,humidity/10,1,NULL);
	send_paquet_sensor(ID("/sensors/pressures/"),PRESSURE,pressure,1,NULL);
	send_paquet_sensor(ID("/sensors/batterys/"),BATTERY,ubat,1,NULL);
}
#endif

#ifdef SLEEP
void ICACHE_FLASH_ATTR go_sleep()
{
	print_debug("sleep\n");
	//uart0_sendStr("S\n");
#ifdef ADS786x
	POWER_OFF_ADS786x
#endif
	system_deep_sleep(t_sleep*1000*1000);
}

void ICACHE_FLASH_ATTR diconnectedCB()
{
	//uart0_sendStr("diconnectedCB\n");
}

void ICACHE_FLASH_ATTR DisconnectMQTT()
{
	MQTT_OnDisconnected(&mqttClient,diconnectedCB);
	//uart0_sendStr("diconnect\n");
	MQTT_Disconnect(&mqttClient); 
	#ifndef METEO
	if(topic[0]!=0) 
	{
		uart0_sendStr("\1stE");//le module passe en veille
	}
	#endif
	uart0_sendStr("DisconnectMQTT");

	os_timer_disarm(&timer_DisconnectMQTT);
	os_timer_setfn(&timer_DisconnectMQTT, (os_timer_func_t *)go_sleep, NULL);
#ifdef PCF8885
	os_timer_arm(&timer_DisconnectMQTT, 50, 0); //temp pour envoyer dernier paquet
#else
	os_timer_arm(&timer_DisconnectMQTT, 20, 0); //temp pour envoyer deconnection
#endif
}
#endif



void ICACHE_FLASH_ATTR connectedCB(void *arg)
{
	//uart0_sendStr(topic);
	//if(topic[0])
		//MQTT_Subscribe(&mqttClient, topic, 0);
	print_debug("connectedCB\n");
	
	#ifdef STORE
	os_install_putc1(NULL);
	init_IRQ_store();
	#endif
	
	#ifdef METEO
	/*os_timer_disarm(&timer_mesure);
*	os_timer_setfn(&timer_mesure, (os_timer_func_t *)timer_mesure_handler, NULL);
*	os_timer_arm(&timer_mesure, 20, 0);*/

	os_timer_disarm(&timer_DisconnectMQTT);
	os_timer_setfn(&timer_DisconnectMQTT, (os_timer_func_t *)DisconnectMQTT, NULL);
	if(topic[0]==0)
		os_timer_arm(&timer_DisconnectMQTT, 200, 0);
	else
		os_timer_arm(&timer_DisconnectMQTT, 800, 0);
	#endif
#ifdef PCF8885
#ifdef SWITCH_ONLY
	//timer_add_callback(100*1000,(t_call_back)pcf8885func); //plus d'irq disponible donc scrutation
	os_timer_disarm(&timer_mesure); // dis_arm the timer
	os_timer_setfn(&timer_mesure, (os_timer_func_t *)pcf8885func, NULL); // set the timer function, dot get os_timer_func_t to force function convert
	os_timer_arm(&timer_mesure, 200, 1);	
#endif
#endif
}

#ifdef TLI4970_ENABLE
void ICACHE_FLASH_ATTR time_send_mesures(void *arg)
{
	send_paquet_sensor(ID("/sensors/powers/"),POWER,moyen_power,0,NULL);
}
#endif

#ifdef STORE
void ICACHE_FLASH_ATTR timeout_store(void* arg)
{
	os_timer_disarm(&timer_timout_store); // dis_arm the timer
	if(puissance_pwm[0] || puissance_pwm[1])
	{
		print_debug("\ntimeout");
		puissance_pwm[0]=0;
		puissance_pwm[1]=0;
		if(niveau<consinge)
			niveau=consinge+echelle/500; //+ 0.2% pour que la prochaine foix on stop avant le bloquage
		else
			niveau=consinge-echelle/500; //- 0.2% pour que la prochaine foix on stop avant le bloquage
			
	}
	#ifdef TRIACK
		puissance_triac=0;
	#endif
	signed int niveau_100_temp=(signed int)niveau*100/(signed int)echelle;
	send_paquet_sensor(ID("/sensors/switch/"),SWITCH,niveau_100,0,"_MsenS_");
}
#endif
void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	int l;
	#ifdef SLEEP //si on recoi des donné on reset le timer qui passe en sleep
	os_timer_disarm(&timer_DisconnectMQTT);
	os_timer_setfn(&timer_DisconnectMQTT, (os_timer_func_t *)DisconnectMQTT, NULL);
	os_timer_arm(&timer_DisconnectMQTT, 200, 0);
	#endif
	
	//char *topicBuf = (char*)os_zalloc(topic_len+1),
	//char* dataBuf = (char*)os_zalloc(data_len+1);
	//os_memcpy(dataBuf,data,data_len);
	//dataBuf[data_len]=0;

	//MQTT_Client* client = (MQTT_Client*)args;
	
	print_debug("mqttDataCb");
	//print_debug(dataBuf);
	
#ifdef TRIACK
#ifndef STORE

	//if(topicCmp(topicLight,topicBuf))
	{
		if(data[0]=='S')
		{
#ifdef TRIACK_ON_OFF_INVERSE
			if(data[1]>0)
			{
				puissance_triac=0;
			}else{
				puissance_triac=TRIACK_ON_OFF_PUISSANCE;
			}
#else
			
#ifdef	TRIACK_ON_OFF
			if(data[1]>0)
			{
				puissance_triac=TRIACK_ON_OFF_PUISSANCE;
			}
#else
			if(data[1]>0)
			{
				if((unsigned char)data[1]>=100)
					puissance_triac=PUISSANCE_MAX;
				else
					puissance_triac=(data[1]*TRIAC_SCALL)+TRIAC_MIN;
			}
#endif
			else
				puissance_triac=0;
#endif

		}
		if(data[0]=='T')
		{
			if(puissance_triac)
				puissance_triac=0;
			else
				puissance_triac=PUISSANCE_MAX;
		}
	}
#endif
#endif

#ifdef PWM

#ifdef STORE
	if(data[0]=='S')
	{
		int val_100 = data[1];
		if(val_100>100)
			val_100=100;
		consinge=val_100*echelle/100; //t_sleep utilise commen echelle
		char test[10];
		print_debug("\n niveau=");
		int_to_srthex(niveau,test);
		print_debug(test);
		print_debug(" consinge=");
		int_to_srthex(consinge,test);
		print_debug(test);
		int delta=consinge-niveau;
		if(delta>0)
		{
			print_debug("---------------------PWM1");
			os_timer_disarm(&timer_timout_store); // dis_arm the timer
			os_timer_setfn(&timer_timout_store, (os_timer_func_t *)timeout_store, NULL); // set the timer function, dot get os_timer_func_t to force function convert
			os_timer_arm(&timer_timout_store, TIMOUT_STORE_START, 1);//detecte si on est blocke
			puissance_pwm[0]=0;
			puissance_pwm[1]=100;

#ifdef TRIACK
			puissance_triac=PUISSANCE_MAX;
#endif
		}
		else if(delta<0)
		{
			print_debug("---------------------PWM0");
			os_timer_disarm(&timer_timout_store); // dis_arm the timer
			os_timer_setfn(&timer_timout_store, (os_timer_func_t *)timeout_store, NULL); // set the timer function, dot get os_timer_func_t to force function convert
			os_timer_arm(&timer_timout_store, TIMOUT_STORE_START, 1);//detecte si on est blocke /si on est blocke stop le moteur et on dit qu'il est en possition
			
			print_debug("\n delta=");
			int_to_srthex(-delta,test);
			print_debug(test);
			
			puissance_pwm[0]=100;
			puissance_pwm[1]=0;
			
			print_debug("\n puissance_pwm=");
			int_to_srthex(puissance_pwm[0],test);
			print_debug(test);
			
#ifdef TRIACK
			puissance_triac=PUISSANCE_MAX;

#endif
		}
		else
		{
			//print_debug("---------------------OFF");
			puissance_pwm[0]=0;
			puissance_pwm[1]=0;
#ifdef TRIACK
			puissance_triac=0;
#endif
		}
	}
#else
	for(l=0;l<NB_PWM && (l+1)<data_len;l++)
	{
		if(data[0]=='S')
		{
			if((unsigned char)data[1+l]>=100)
				puissance_pwm[l]=100;
			else
				puissance_pwm[l]=data[1+l];
		}
		if(data[0]=='T')
		{
			if(puissance_pwm)
				puissance_pwm[l]=0;
			else
				puissance_pwm[l]=100;
		}
	}
#endif
#endif

#ifdef SET_GPIO
SET_GPIO;
#endif

#ifdef RELAY
	if(data[1])
		GPIO_OUTPUT_SET(BIT_RELAY,1);
	else
		GPIO_OUTPUT_SET(BIT_RELAY,0);
#endif

#ifdef SEND_MQTT_ON_UART
	/*char *dataBuf = (char*)os_zalloc(data_len+1);
// 	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;*/
	uart0_sendStr("\1stS");
	uart0_write_char(data_len);
	uart0_write_char(data_len>>8);
	uart0_tx_buffer((uint8*)data,data_len);
	//os_free(dataBuf);
#endif
	

}


void ICACHE_FLASH_ATTR user_init(void)
{

	uart_init(BIT_RATE_115200,BIT_RATE_115200);


	volatile int l;
	
	system_deep_sleep_set_option(2); //1rf calibration 2no rf calibration
	
#ifdef SWITCH_ONLY
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_IRQ_PCF8885, FUNC_GPIO_IRQ_PCF8885);
	GPIO_OUTPUT_SET(RECEIVEGPIO_IRQ_PCF8885,1); //"disable" the reset 
#endif

		
	//os_install_putc1(NULL);
	
	/*char string[15]="\nubat";
	int_to_srthex(ubat,string+5);
	print(string);*/
	/*char string2[15]="\nstat";
	int_to_srthex(wifi_station_get_connect_status(),string2);
	print(string2);*/

#ifdef SLEEP
	ubat=( (int)system_adc_read()<<6)/21;
#endif
#ifdef SLEEP_IF_BATTERY_LOW

	if(ubat<1200) //si vbat trop faible
	{
		print_debug("lowbat\n");
		system_deep_sleep_set_option(4);
		system_deep_sleep(T_LONG_SLEEP*1000*1000);
 	}
#endif
	
	
	#if defined(BME280) || defined(TLI4970_ENABLE)
	spi_init(HSPI,9); //configure SPI Fcpu /4/(x-1)
	#endif
	#ifdef BME280
	//SPI_routine();
	//bme280_set_power_mode(BME280_NORMAL_MODE);
	//os_delay_us(10000);
	//bme280_read_pressure_temperature_humidity(&old_pressure,&old_temperature, &old_humidity);//recupere les ancienne valeur
	bme280_init_user();
	#ifdef SLEEP
	os_timer_disarm(&timer_mesure);
	os_timer_setfn(&timer_mesure, (os_timer_func_t *)timer_mesure_handler, NULL);
	os_timer_arm(&timer_mesure, 100, 0);
	#endif
	#endif
	
	#ifdef ADS786x
	POWER_ON_ADS786x; //power suply ADS786x
#endif
	
	ETS_UART_INTR_DISABLE(); //desactive IRQ uart durant l'ecriture (recomandé)
	spi_flash_read(ADRESSE_CONFIG_IN_FLASH,ssid32bit,MAX_SSID_LENGTH);
	spi_flash_read(ADRESSE_CONFIG_IN_FLASH+MAX_SSID_LENGTH,password32bit,MAX_PASSWORD_LENGTH);
	spi_flash_read(ADRESSE_CONFIG_IN_FLASH+MAX_SSID_LENGTH+MAX_PASSWORD_LENGTH,(uint32*)adresse32bit,MAX_ADRESSE_LENGTH*NB_ADRESSE);
	spi_flash_read(ADRESSE_CONFIG_IN_FLASH+MAX_SSID_LENGTH+MAX_PASSWORD_LENGTH+MAX_ADRESSE_LENGTH*NB_ADRESSE,StationName32bit,MAX_LENGTH_STATION_NAME);
	spi_flash_read(ADRESSE_CONFIG_IN_FLASH+MAX_SSID_LENGTH+MAX_PASSWORD_LENGTH+MAX_ADRESSE_LENGTH*NB_ADRESSE+MAX_LENGTH_STATION_NAME,&t_sleep,sizeof(t_sleep));
	StationName[MAX_LENGTH_STATION_NAME-1]='\0';
	int_to_srthex(system_get_chip_id(),MYID);
	
	for(l=0;l<NB_ADRESSE;l++)
		adresse[l][MAX_ADRESSE_LENGTH-1]=0;
	password[MAX_PASSWORD_LENGTH-1]=0;
	ssid[MAX_SSID_LENGTH-1]=0;
	
	//system_rtc_mem_read(64,topic,MAX_TOPIC_LENGTH);
	//print(topic);
	
	#ifdef PWM
	PIN_FUNC_SELECT_PWM;


	for(l=0;l<NB_PWM;l++)
	{
		GPIO_OUTPUT_SET(BIT_PWM[l],0);
		print_debug("set1\n");

	}
	#endif
	
	#ifdef RELAY
		PIN_FUNC_SELECT_RELAY;
		GPIO_OUTPUT_SET(BIT_RELAY,1);
	#endif
		
	print_debug("start\n");
	
	config_ap();
	ETS_UART_INTR_ENABLE(); //reactiver IRQ
	
	if(StationName[0])
		mqttInit(&mqttClient,adresse[3],(t_call_back_connected)connectedCB);
	
	#ifdef TLI4970_ENABLE
	os_timer_disarm(&timer_send_mesures); // dis_arm the timer
	os_timer_setfn(&timer_send_mesures, (os_timer_func_t *)time_send_mesures, NULL); // set the timer function, dot get os_timer_func_t to force function convert
	os_timer_arm(&timer_send_mesures, 2000, 1);//tout les 2seconde
	#endif
	
	#if defined(TRIACK) || defined(TLI4970_ENABLE) || defined(PWM)
	timer_init(PRECISION_TIMER);
	#endif
		
	#ifdef METEO
	wifi_set_sleep_type(LIGHT_SLEEP_T); //impossible de comuniquer en uart durant les phase de sleep et le timer ne semble plus fonctioner
	#else
	wifi_set_sleep_type(NONE_SLEEP_T);
	#endif

	
#ifdef SLEEP
	os_timer_disarm(&timer_time_out_connection); // dis_arm the timer
	os_timer_setfn(&timer_time_out_connection, (os_timer_func_t *)DisconnectMQTT, NULL); // set the timer function, dot get os_timer_func_t to force function convert
	os_timer_arm(&timer_time_out_connection, TIMOUT_CONNECTION, 1);	
#endif 

#ifdef TRIACK
#if defined(HALF_SIN_POS) || defined(HALF_SIN_NEG)
	timer_add_callback(20000+1,(t_call_back)ZeroCross);
	timer_add_callback(100000,(t_call_back)timerDisableTriack);
#else
	timer_add_callback(20000+1,(t_call_back)timerZeroCross);
#endif
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U,FUNC_GPIO15); //triac
	gpio_output_set(0,BIT(BIT_TRIAC), BIT(BIT_TRIAC), 0);
	GPIO_OUTPUT_SET(BIT_TRIAC,0);
	print_debug("init_IRQ_zero_cross\n");
	init_IRQ_zero_cross();
#endif
	
	#ifdef PWM
	timer_add_callback(100*PWM_SCALL,(t_call_back)timerPwm1);
	#if NB_PWM > 1
	timer_add_callback(150*PWM_SCALL,(t_call_back)timerPwm2);
	#endif
	#endif
	
#ifdef PCF8885
#ifdef SWITCH_ONLY
	send_PCF8885();
#else
	init_IRQ_pcf8885();
#endif
#endif
	
#ifdef DETECT_0V
	//timer_add_callback(10000000,(t_call_back)no_zero_cross);
	os_timer_disarm(&timer_0V); // dis_arm the timer
	os_timer_setfn(&timer_0V, (os_timer_func_t *)no_zero_cross, NULL); // set the timer function, dot get os_timer_func_t to force function convert
#endif
	
	//uart0_sendStr("\1stB");//le module a demaré
	
#ifdef TLI4970_ENABLE
	TLI4970_init();
	TLI4970_disable();
	timer_add_callback(TIME_SAMPLE_CURENT,(t_call_back)timerMesureSampleAdc);
#endif
	
}

void user_rf_pre_init(void)
{
	#ifdef PCF8885
	i2c_master_gpio_init();
	#endif
	system_update_cpu_freq(160); //ne semble pas changer la consomation
	//a verifier si modifi le CLK du SPI
}

uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }
    return rf_cal_sec;
}
