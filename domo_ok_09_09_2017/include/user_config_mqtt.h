#ifndef _USER_CONFIG_MQTT_H_
#define _USER_CONFIG_MQTT_H_

#define MQTT_DEBUG_ON

//#define CLIENT_SSL_ENABLE

/*DEFAULT CONFIGURATIONS*/

//#define MQTT_HOST			"192.168.0.12" //or "mqtt.yourdomain.com"
//#define MQTT_HOST			"192.168.1.2"
#define MQTT_PORT			1883
#define MQTT_BUF_SIZE		0x500
#define MQTT_KEEPALIVE		120	 /*second*/

#define MQTT_CLIENT_ID		"DVES_%08X"
#define MQTT_USER			"DVES_USER"
#define MQTT_PASS			"DVES_PASS"

#define MQTT_RECONNECT_TIMEOUT 	5	/*second*/

#define DEFAULT_SECURITY	0
#define QUEUE_BUFFER_SIZE		 		512

#define PROTOCOL_NAMEv31	/*MQTT version 3.1 compatible with Mosquitto v0.15*/
//PROTOCOL_NAMEv311			/*MQTT version 3.11 compatible with https://eclipse.org/paho/clients/testing/*/

#endif
