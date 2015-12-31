#include <c_types.h>
#include <ets_sys.h>
#include <osapi.h>
#include <gpio.h>
#include <os_type.h>
#include <user_interface.h>
#include <mem.h>

#include "main.h"
#include "config.h"
#include "uart.h"
#include "wifi.h"
#include "mqtt.h"
#include "ota.h"
#include "debug.h"

#include "ac.h"
#include "dht22.h"

#include "httpd.h"
#include "auth.h"
#include "cgi.h"
#include "httpdespfs.h"

MQTT_Client mqttClient;

void ICACHE_FLASH_ATTR wifiConnectCb(uint8_t status)
{
	if(status == STATION_GOT_IP){
		MQTT_Connect(&mqttClient);
	} else {
		MQTT_Disconnect(&mqttClient);
	}
}

void ICACHE_FLASH_ATTR mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Connected\r\n");

	MQTT_Subscribe(client, MQTT_TOPIC_AC, 0);
	MQTT_Subscribe(client, MQTT_TOPIC_SETTINGS, 0);
	MQTT_Subscribe(client, MQTT_TOPIC_UPDATE, 0);
	MQTT_Subscribe(client, MQTT_TOPIC_RESTART, 0);
}

void ICACHE_FLASH_ATTR mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Disconnected\r\n");
}

void ICACHE_FLASH_ATTR mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Published\r\n");
}

void ICACHE_FLASH_ATTR mqttPublishSettings(temp_humid) {
	char buff[200] = "";
	
	ac_get_settings(buff, temp_humid);
	MQTT_Publish(&mqttClient, MQTT_TOPIC_SETTINGS, buff, os_strlen(buff), 0, 0);
	INFO("MQTT send topic: %s, data: %s \r\n", MQTT_TOPIC_SETTINGS, buff);
}

void ICACHE_FLASH_ATTR mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	char *topicBuf = (char*)os_zalloc(topic_len + 1),
	     *dataBuf  = (char*)os_zalloc(data_len + 1);

	MQTT_Client* client = (MQTT_Client*)args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	INFO("Receive topic: %s, data: %s\r\n", topicBuf, dataBuf);

	if (os_strcmp(topicBuf, MQTT_TOPIC_AC) == 0)
	{
		if (os_strcmp(dataBuf, "settings") == 0) {
			mqttPublishSettings(1);
		} else {
			// try to parse and set
			// shift 1 byte left
			char *token, *endtoken;
			int t, apply = 0;
			char cmd[20], val[20];
			for (t = 1; t < os_strlen(dataBuf); t++) dataBuf[t - 1] = dataBuf[t];
			dataBuf[os_strlen(dataBuf) - 2] = '\0';
			token = strtok_r(dataBuf, ",", &endtoken);
			while (token != NULL) {
				int i = 0;
				char *tmp, *endtmp;
				//INFO("> %s\n", token);
				tmp = strtok_r(token, ":", &endtmp);
				while (tmp != NULL) {
					//INFO(">> %s\n", tmp);
					// trim "
					for (t = 1; t < os_strlen(tmp); t++) tmp[t - 1] = tmp[t];
					tmp[os_strlen(tmp) - 2] = '\0';
					if (i == 0) os_strcpy(cmd, tmp);
					if (i == 1) os_strcpy(val, tmp);

					tmp = strtok_r(NULL, ":", &endtmp);
					i++;
				}
				//INFO(">>> %s: %s\n", cmd, val);
				
				if (os_strcmp(cmd, "power") == 0) {
					apply = 1;
					ac_set_power(val);	
				} else
				if (os_strcmp(cmd, "mode") == 0) {
					apply = 1;
					ac_set_mode(val);
				} else
				if (os_strcmp(cmd, "temp") == 0) {
					apply = 1;
					ac_set_temp(val);
				} else
				if (os_strcmp(cmd, "fan") == 0) {
					apply = 1;
					ac_set_fan(val);
				} else
				if (os_strcmp(cmd, "swing") == 0) {
					apply = 1;
					ac_set_swing(val);
				}

				token = strtok_r(NULL, ",", &endtoken);
			}
			if (apply) {
				ir_send();
				mqttPublishSettings(0);
				CFG_Save();
			}
		}
	} else
	if (os_strcmp(topicBuf, MQTT_TOPIC_UPDATE) == 0)
	{
		OtaUpdate();
	} else
	if (os_strcmp(topicBuf, MQTT_TOPIC_RESTART) == 0)
	{
		system_restart();
	}
}

int ICACHE_FLASH_ATTR myPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass, int passLen)
{
	if (no == 0)
	{
		os_strcpy(user, MQTT_USER);
		os_strcpy(pass, MQTT_PASS);

		return 1;

		//Add more users this way. Check against incrementing no for each user added.
		//	} else if (no==1) {
		//		os_strcpy(user, "user1");
		//		os_strcpy(pass, "something");
		//		return 1;
	}

	return 0;
}

HttpdBuiltInUrl builtInUrls[]={
	{"/*", authBasic, myPassFn},
	{"/", cgiRedirect, "/index.html"},
	{"/save.cgi", saveCGI, NULL},
	{"/settings.cgi", settingsCGI, NULL},
	//{"/load.tpl", cgiEspFsTemplate, loadTpl},
	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};

void ICACHE_FLASH_ATTR
user_init()
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_delay_us(1000000);

	INFO("Starting up...\r\n");

	INFO("Loading config...\r\n");
	CFG_Load();

	gpio_init();

	INFO("Initializing MQTT...\r\n");
	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);
	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);

	INFO("Connect to WIFI...\r\n");
	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);

	// ir
	INFO("Setup IR...\r\n");
	ac_init();
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	gpio_output_set(0, BIT4, BIT4, 0);

	// web
	INFO("Setup WEB...\r\n");
	//stdoutInit();
	ioInit();
	DHTInit();
	httpdInit(builtInUrls, 80);

	INFO("Startup completed. Now running from rom %d...\r\n", rboot_get_current_rom());
}
