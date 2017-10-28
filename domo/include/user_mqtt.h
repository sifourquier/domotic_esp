#include "../mqtt/include/mqtt.h"

typedef void (*t_call_back_connected) (void);

typedef struct{
	uint8_t device_id[16];

	uint8_t mqtt_host[64];
	uint32_t mqtt_port;
	uint8_t mqtt_user[32];
	uint8_t mqtt_pass[32];
	uint32_t mqtt_keepalive;
	uint8_t security;
} SYSCFG;


_Bool topicCmp(char* buff1, char* buff2);
void mqttConnectedCb(uint32_t *args);
void mqttDisconnectedCb(uint32_t *args);
void mqttPublishedCb(uint32_t *args);
void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len);
void ICACHE_FLASH_ATTR mqttInit(MQTT_Client* mqttClient,char* adresse,t_call_back_connected ConnectedCB);
void mqtt_print(MQTT_Client* client,char * string);
_Bool mqttIsConected();