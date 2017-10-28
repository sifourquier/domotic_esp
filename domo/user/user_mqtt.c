#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "../mqtt/include/mqtt.h"
#include "mem.h"
#include "driver/ws2812.h"
#include "user_config_mqtt.h"
#include "user_mqtt.h"
#include "user_config.h"

#include "driver/uart.h"

#ifdef DEBUG_PRINT
	#define DEBUG(x) uart0_sendStr(x);
#else
	#define DEBUG(x) 
#endif

t_call_back_connected ConnectedCallBack;

extern char MYID[];

SYSCFG sysCfg;

os_timer_t timer_try_connect;

char topicLight[MAX_ID_LENGTH+10]={"actuators"};

_Bool connected=0;

char* ID(char* x);
char* ad_name(char* x);

void int_to_srthex(int val, char* str);


tryConnect(void* args) // define timer function
{
	MQTT_Client* mqttClient=args;
	//DEBUG("MQTT: tryConnect\r\n");
	MQTT_Connect(mqttClient);
}

/*void wifiConnectCb(uint8_t status)
{
	if(status == STATION_GOT_IP){
		MQTT_Connect(&mqttClient);
	} else {
		MQTT_Disconnect(&mqttClient);
	}
}*/
_Bool topicCmp(char* buff1, char* buff2)
{
	int l=0;
	while((buff1[l]!='\0')&&(buff2[l]!='\0'))
	{
		if(buff1[l]!=buff2[l])
			return false;
		l++;
	}
	return (buff1[l]==buff2[l]);
}

void ICACHE_FLASH_ATTR mqttInit(MQTT_Client* mqttClient,char* adresse,t_call_back_connected ConnectedCB)
{
	int length=0;
	while(topicLight[length])
		  length++;
	topicLight[length]='/';
	topicLight[length+1]=0;
	
	ConnectedCallBack=ConnectedCB;
	os_memset(&sysCfg, 0x00, sizeof sysCfg);

	os_sprintf(sysCfg.device_id, MQTT_CLIENT_ID, system_get_chip_id());
	os_sprintf(sysCfg.mqtt_host, "%s", adresse);
	sysCfg.mqtt_port = MQTT_PORT;
	os_sprintf(sysCfg.mqtt_user, "%s", MQTT_USER);
	os_sprintf(sysCfg.mqtt_pass, "%s", MQTT_PASS);
	
	sysCfg.security = DEFAULT_SECURITY;	/* default non ssl */

	sysCfg.mqtt_keepalive = MQTT_KEEPALIVE;


	MQTT_InitConnection(mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);
	//MQTT_InitConnection(&mqttClient, "192.168.11.122", 1880, 0);

	MQTT_InitClient(mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
	//MQTT_InitClient(&mqttClient, "client_id", "user", "pass", 120, 1);
	
	MQTT_InitLWT(mqttClient, "/lwt", "offline", 0, 0);
	MQTT_OnConnected(mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(mqttClient, mqttPublishedCb);
	MQTT_OnData(mqttClient, mqttDataCb);
	
	os_timer_disarm(&timer_try_connect); // dis_arm the timer
	os_timer_setfn(&timer_try_connect, (os_timer_func_t *)tryConnect, mqttClient); // set the timer function, dot get os_timer_func_t to force function convert
	os_timer_arm(&timer_try_connect, MQTT_TIME_TRY_CONNECT, 1); // arm the timer repeat always
	//tryConnect(mqttClient);
}

_Bool mqttIsConected()
{
	return connected;
}

void ICACHE_FLASH_ATTR mqtt_print(MQTT_Client* client,char * string)
{
	unsigned int l=0;
	while(string[l]!=0)
		l++;
	MQTT_Publish(client, "/Debug/", string, l, 0, 0);
}

void ICACHE_FLASH_ATTR mqttConnectedCb(uint32_t *args)
{
	unsigned int l;

	os_timer_disarm(&timer_try_connect); // dis_arm the timer
	
	MQTT_Client* client = (MQTT_Client*)args;
	DEBUG("MQTT: Connected\r\n");
	connected=1;
	
#if defined(TRIACK) || defined(PWM) || defined(RELAY)
	MQTT_Subscribe(client, ad_name(topicLight), 0);
	DEBUG(ad_name(topicLight));
	connected=1;
#endif
	
	MQTT_Publish(client, "/device", MYID, 1, 1, 1);
	ConnectedCallBack();
}

void ICACHE_FLASH_ATTR mqttDisconnectedCb(uint32_t *args)
{
	connected=0;
	os_timer_arm(&timer_try_connect, MQTT_TIME_TRY_CONNECT, 1); // arm the timer, 1000 repeat, repeat always
	MQTT_Client* client = (MQTT_Client*)args;
	DEBUG("MQTT: Disconnected\r\n");
	
}

void mqttPublishedCb(uint32_t *args)
{
	DEBUG("MQTT: mqttPublishedCb\r\n");
	MQTT_Client* client = (MQTT_Client*)args;
	//DEBUG("MQTT: Published\r\n");
}

