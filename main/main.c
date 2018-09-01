
/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#define LED_BLINK(LED,delay) \
			gpio_set_level(LED, 1); \
			vTaskDelay(delay / portTICK_RATE_MS); \
			gpio_set_level(LED, 0); \
			vTaskDelay(delay / portTICK_RATE_MS);
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "include/hxd019_drv.h"
#include "include/wifi.h"
#include "include/key.h"
#include "include/led.h"
#include "include/wifi.h"
#include "include/MQTT_esp32_client.h"
#include "include/MQTT_timer.h"
#include "include/cJSON.h"
const int SEND_IRDA_BIT = BIT6;
const int LEARN_IRDA_BIT = BIT7;
TaskHandle_t xMQTTHandle, xLedHandle, xKeyHandle, xUDPipHandle, xUDPDeviceMHandle;

//IR
uint8_t ir_learned_data[232];
char ir_name[256] = "IRTransmitter";

void _read_ir_data(uint8_t* buffer, __OUT__ (int ) flag,__OUT__ (int) ff_count );
bool _learn_ir_data(uint8_t* buffer);

void task_keypress_ir_send(void * pvParameters);
void task_keypress_ir_learning(void * pvParameters);
void task_tcpservice_ir_learning(void *pvParameters);
void task_tcpservice_ir_send(void *pvParameters);
void task_tcp_service(void *pvParameters);
void task_discoverservice(void *pvParameters);

void vTaskKEY(void *pvParameters);
void vTaskLED(void *pvParameters);
void vTaskMQTT(void *pvParameters);

void msg_success_0xbf(ref_client_msg msg,void* data);
void msg_success_0xaf(ref_client_msg msg,void* data);

bool server_recv_accept_send_process(void *server, ref_so_client client);
bool server_client_msg_process(void *server, ref_so_client client);

unsigned char HexToChar(unsigned char bChar);
unsigned char CharToHex(unsigned char bHex);
unsigned char Char2ToHex(unsigned char aChar, unsigned char bChar);
//IR SEND
void mqtt_learning_msg_callback(struct _mqtt_context * context,OnMqttLearningCallback callback);
void mqtt_sending_msg_callback(struct _mqtt_context * context,uint8_t *data,int datalength);
//IR LEARN
 int pressSecond=0;
// status led: blue led,red led
void app_main()
{
	mqtt_context_str.onMqttLearningMsg=mqtt_learning_msg_callback;
	mqtt_context_str.onMqttSendingMsg=mqtt_sending_msg_callback;
//	wifi_config_t wifi_config = {
//		.sta = {
//			.ssid = "ETOR-2.4G",
//			.password = "Admin2018",
////			.ssid = "Yitagy",
////			.password = "admin2018",
////			.ssid = "iottest155",
//			.scan_method = WIFI_ALL_CHANNEL_SCAN,
//			.sort_method = WIFI_CONNECT_AP_BY_SIGNAL}};

	ref_wifi_context context = new_wifi_context();
	global_wifi_context=context;
//	context->wifi_config = &wifi_config;
//	context->ssid = "Yitagy";
//	context->ssid = "ETOR-2.4G";

	// nvs_flash_init();
	// xTaskCreate(&udp_Device_Management, "udp_Device_Management", 4096 ,NULL , XTASK4, &xUDPDeviceMHandle);
	// // // SeekDevice UDP PORT 40400
	// // xTaskCreate(&udp_send_ip, "udp_send_ip", 4096 ,NULL , XTASK4, &xUDPipHandle);
	// // // Recceive ETOR Cloud Command
	ESP_ERROR_CHECK(nvs_flash_init());
	IRDA_INIT();
	initialise_led();
	ESP_LOGI("INFO", "initialise_wifi begin");
	initialise_wifi(context);
	ESP_LOGI("INFO", "initialise_wifi end");

	ESP_LOGI("INFO", "xTaskCreate vTaskLED begin");
	xTaskCreate(&vTaskLED, "vTaskLED", 2048, NULL, XTASK9, &xLedHandle);
	ESP_LOGI("INFO", "xTaskCreate vTaskLED end");

	ESP_LOGI("INFO", "xTaskCreate vTaskKEY begin");
	xTaskCreate(&vTaskKEY, "vTaskKEY", 2048, NULL, XTASK9, &xKeyHandle);
	ESP_LOGI("INFO", "xTaskCreate vTaskKEY end");

	ESP_LOGI("INFO", "xTaskCreate vTaskMQTT begin");
	xTaskCreate(&vTaskMQTT, "vTaskMQTT", 8192, context, XTASK9, &xMQTTHandle);
	ESP_LOGI("INFO", "xTaskCreate vTaskMQTT end");

	ESP_LOGI("INFO", "xTaskCreate task_tcp_service begin");
	xTaskCreate(&task_tcp_service, "task_tcp_service", 2048, context, XTASK9, &xKeyHandle);
	ESP_LOGI("INFO", "xTaskCreate task_tcp_service end");

	ESP_LOGI("INFO", "xTaskCreate task_discoverservice begin");
	xTaskCreate(&task_discoverservice, "task_discoverservice", 2048, context, XTASK9, &xUDPDeviceMHandle);
	ESP_LOGI("INFO", "xTaskCreate task_discoverservice end");

	while(1){
		vTaskDelay(10000 / portTICK_RATE_MS);
		LOG("MAIN","heapSize:%d",esp_get_free_heap_size());
	}
}
void task_discoverservice(void*parameter){
	ESP_LOGI("INFO", "task_discoverservice start");
	struct sockaddr_in udpEndPoint, broadcast;
	newIP(&udpEndPoint, "0.0.0.0", 40400);
	newIP(&broadcast, "255.255.255.255", 40400);
	int so_udp = new_socket(&udpEndPoint, IPPROTO_UDP);;
	FD_SET_NONBLOCK((so_udp));
	ref_wifi_context context=(ref_wifi_context)parameter;
	int recvdCount=0;
	char* buffer=NULL;
	uint8_t recvBuff[15];
	cJSON * root = NULL;
	bool need_fresh=true;
	while (true)
	{
		vTaskDelay(1000 / portTICK_RATE_MS);
		if (context->wifi_connected)
		{
			if(need_fresh){
				need_fresh=false;
				if(root!=NULL){
					cJSON_Delete(root);
					root=NULL;
				}
				if(buffer!=NULL){
					free(buffer);
					buffer=NULL;
				}
				root = cJSON_CreateObject();
				cJSON_AddStringToObject(root,"ip",WIFI_IP_ADDR);
				cJSON_AddStringToObject(root,"id",WIFI_MAC_ADDR);
				cJSON_AddStringToObject(root,"type","IRT");
				buffer=cJSON_Print(root);
				LOG("UDP","buffer:\t%s",buffer);
			}
			recvdCount=recvfrom(so_udp,recvBuff,15,0,NULL,NULL);
			recvBuff[12]='\0';
			//LOG("UDP","DEVDISC\trecvd:%d\terrno:%d\tcontent:%s",recvdCount,get_error_code(so_udp),recvBuff);
			if(recvdCount>0 && 0==strcmp((char*)recvBuff,(const char*)"seekdevice\r\n"))
			{
				recvdCount=sendto(so_udp,buffer,strlen(buffer),0,&broadcast,sizeof(broadcast));
				//LOG("UDP","DEVDISC\tsendTo:%d\terrno:%d",recvdCount,get_error_code(so_udp));
			}
		}else {
			need_fresh=true;
		}
	}
	ESP_LOGI("INFO", "task_discoverservice stop");
}
void task_tcp_service(void *pvParameters){
	ref_wifi_context context=(ref_wifi_context)pvParameters;
	ref_nio_so_server server = new_so_server();
	server->init(server, "0.0.0.0", 9091);
	server->add_handler(server, server_recv_accept_send_process);
	server->add_handler(server, server_client_msg_process);
	while (true)
	{
		vTaskDelay(50 / portTICK_RATE_MS);
		if (context->wifi_connected)
		{
			xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
			server->process(server);
		}else {
			xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
		}
	}
}
void mqtt_learning_msg_callback(struct _mqtt_context * context,OnMqttLearningCallback callback){
	LOG("TASK", "mqtt_learning_msg start");
	EventBits_t uxBits = xEventGroupWaitBits(wifi_event_group, SEND_IRDA_BIT | LEARN_IRDA_BIT , false, true, 100);
	if (LEARN_IRDA_BIT == (uxBits & LEARN_IRDA_BIT) || SEND_IRDA_BIT == (uxBits & SEND_IRDA_BIT))
	{
		LOG("IRDA", "mqtt_learning_msg_callback\t\ton learning");
		//vTaskDelete(NULL);
		return;
	}
	xEventGroupSetBits(wifi_event_group, LEARN_IRDA_BIT);
	if(_learn_ir_data(ir_learned_data)){
		callback(context,ir_learned_data,232);
	}
	xEventGroupClearBits(wifi_event_group, LEARN_IRDA_BIT);
	LOG("TASK", "mqtt_learning_msg stop");
	//vTaskDelete(NULL);
}
void mqtt_sending_msg_callback(struct _mqtt_context * context,uint8_t *data,int datalength){
	LOG("TASK", "mqtt_sending_msg_callback start");
	EventBits_t uxBits = xEventGroupWaitBits(wifi_event_group, SEND_IRDA_BIT | LEARN_IRDA_BIT , false, true, 100);
	if (LEARN_IRDA_BIT == (uxBits & LEARN_IRDA_BIT) || SEND_IRDA_BIT == (uxBits & SEND_IRDA_BIT))
	{
		LOG("IRDA", "mqtt_sending_msg_callback\t\ton learning");
		//vTaskDelete(NULL);
		return;
	}
	xEventGroupSetBits(wifi_event_group,SEND_IRDA_BIT);
	LOG("IRDA", "send");
	for(int i=0;i<datalength;i++){
		printf("%02x ",*((uint8_t*)(data+i)));
	}
	printf(" end \r\n");
	writeI2C2(data, datalength);
	LOG("TASK", "mqtt_sending_msg_callback stop ");
	xEventGroupClearBits(wifi_event_group, SEND_IRDA_BIT);
	//vTaskDelete(NULL);
	return;
}
void vTaskKEY(void *pvParameters)
{
	LOG("KEY","task vTaskKEY start");
	while (1)
	{
		pressSecond = 0;
		while (1)
		{
			bool break_this = false;
			while (gpio_get_level(KEY_SET_IO) == 0)
			{
				vTaskDelay(20 / portTICK_RATE_MS);
			}
			int notPress = 0;
			while (gpio_get_level(KEY_SET_IO) == 1)
			{
				notPress++;
				vTaskDelay(10 / portTICK_RATE_MS);
				if (notPress > 50)
				{
					break_this = true;
					break;
				}
			}
			if (break_this)
			{
				break;
			}
			pressSecond++;
			LOG("KEY", "press:\t%d ms", pressSecond);
		}

		switch (pressSecond)
		{
		case 0:
			break;
		case 1:
			LOG("KEY", "key press ir learning");
			xTaskCreate(&task_keypress_ir_learning,
					"task_keypress_ir_learning",
					4096,
					NULL,
					XTASK6, NULL);
			LOG("KEY", "key press ir learning task create");
			break;
		case 2:
			LOG("KEY", "key press send irdata ");
			xTaskCreate(&task_keypress_ir_send,
					"task_keypress_ir_send",
					4096,
					NULL,
					XTASK6, NULL);
			LOG("KEY", "key press create send irdata task");
			break;
		case 3:
			LOG("KEY", "key press disconnect wifi");
			ESP_ERROR_CHECK(esp_wifi_disconnect());
			LOG("KEY", "key press wifi disconnect");
			break;
		case 5:
			LOG("SMART_WIFI","smartconfig_example_task begin");
			xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
			LOG("SMART_WIFI","smartconfig_example_task end");
			break;
		default:
			LOG("KEY", "press:\t%d ms", pressSecond);
			break;
		}
	}
	LOG("KEY","task vTaskKEY end");
	vTaskDelete(NULL);
}
void vTaskLED(void *pvParameters)
{
	static const char *TAG = "led";
	EventBits_t uxBits;
	ESP_LOGI(TAG, "led scan");

	gpio_set_level(LED_BLUE_IO, 0);
	gpio_set_level(LED_RED_IO, 0);

	while (1)
	{
		//LOG("LED","pressSecond:%d",pressSecond);
		if(pressSecond>0 ){
			LED_BLINK(LED_RED_IO,500);
			goto NO_DELAY;
		}
		uxBits = xEventGroupWaitBits(wifi_event_group,
				WIFI_CONNECTED_BIT |
				SMART_WIFI_WORKING |
				SEND_IRDA_BIT |
				LEARN_IRDA_BIT ,
				false, false, 50);
		//LOG("LED","uxBits:%d",uxBits);
		if (SEND_IRDA_BIT==(uxBits & SEND_IRDA_BIT))
		{
			gpio_set_level(LED_RED_IO,0);
			LED_BLINK(LED_BLUE_IO,50);
			LED_BLINK(LED_BLUE_IO,50);
			LED_BLINK(LED_BLUE_IO,50);
			goto NO_DELAY;
		}
		if (LEARN_IRDA_BIT==(uxBits & LEARN_IRDA_BIT))
		{
			gpio_set_level(LED_RED_IO,0);
			LED_BLINK(LED_BLUE_IO,250);
			LED_BLINK(LED_BLUE_IO,250);
			goto NO_DELAY;

		}
		if (WIFI_CONNECTED_BIT==(uxBits & WIFI_CONNECTED_BIT))
		{
			gpio_set_level(LED_RED_IO,0);
			LED_BLINK(LED_BLUE_IO,1000);
			goto NO_DELAY;
		}
		if (SMART_WIFI_WORKING==(uxBits & SMART_WIFI_WORKING))
		{
			gpio_set_level(LED_RED_IO,0);
			LED_BLINK(LED_RED_IO,200);
			goto NO_DELAY;
		}
		gpio_set_level(LED_RED_IO,1);
		vTaskDelay(20 / portTICK_RATE_MS);
		NO_DELAY:{
			continue;
		}
	}
	vTaskDelete(NULL);
}

void vTaskMQTT(void *pvParameters)
{
	ref_wifi_context wifi=(ref_wifi_context)pvParameters;

	static const char *TAG = "MQTT";
	static bool lose=true;
	tcp_mqtt_conn();
	while(1){
		if(wifi->wifi_connected){
			tcp_mqtt_yield();
		}
		vTaskDelay(500/ portTICK_RATE_MS);
	}
	vTaskDelete(NULL);
}

void task_keypress_ir_send(void * pvParameters){
	LOG("TASK", "task_keypress_ir_send start");
	EventBits_t uxBits = xEventGroupWaitBits(wifi_event_group, SEND_IRDA_BIT | LEARN_IRDA_BIT , false, true, 100);
	if (LEARN_IRDA_BIT == (uxBits & LEARN_IRDA_BIT) || SEND_IRDA_BIT == (uxBits & SEND_IRDA_BIT))
	{
		LOG("IRDA", "task_keypress_ir_send\t\ton learning");
		vTaskDelete(NULL);
		return;
	}
	xEventGroupSetBits(wifi_event_group,SEND_IRDA_BIT);
	LOG("IRDA", "send");
	for(int i=0;i<232;i++){
		printf("%02x ",*((uint8_t*)(ir_learned_data+i)));
	}
	printf(" end \r\n");
	writeI2C2(ir_learned_data, 232);
	LOG("TASK", "task_keypress_ir_send stop ");
	xEventGroupClearBits(wifi_event_group, SEND_IRDA_BIT);
	vTaskDelete(NULL);
	return;
}
void task_keypress_ir_learning(void * pvParameters){

	LOG("TASK", "task_keypress_ir_learning start");
	EventBits_t uxBits = xEventGroupWaitBits(wifi_event_group, SEND_IRDA_BIT | LEARN_IRDA_BIT , false, true, 100);
		if (LEARN_IRDA_BIT == (uxBits & LEARN_IRDA_BIT) || SEND_IRDA_BIT == (uxBits & SEND_IRDA_BIT))
	{
		LOG("IRDA", "task_keypress_ir_learning\t\ton learning");
		vTaskDelete(NULL);
		return;
	}
	xEventGroupSetBits(wifi_event_group, LEARN_IRDA_BIT);
	_learn_ir_data(ir_learned_data);
	xEventGroupClearBits(wifi_event_group, LEARN_IRDA_BIT);
	LOG("TASK", "task_keypress_ir_learning stop");
	vTaskDelete(NULL);
}

void task_tcpservice_ir_learning(void *pvParameters)
{
	ref_client_msg msg = (ref_client_msg)pvParameters;
	LOG("TASK", "task_ir_learn start");

	EventBits_t uxBits = xEventGroupWaitBits(wifi_event_group, SEND_IRDA_BIT | LEARN_IRDA_BIT , false, true, 100);
	if (LEARN_IRDA_BIT == (uxBits & LEARN_IRDA_BIT) || SEND_IRDA_BIT == (uxBits & SEND_IRDA_BIT))
	{
		LOG("IRDA", "task_tcpservice_ir_learning\t\ton learning");
		free(msg);
		vTaskDelete(NULL);
		return;
	}
	if(msg->type!=0xaf){
		LOG("IRDA", "wrong type af!=%02x",msg->type);
		free(msg);
		vTaskDelete(NULL);
		return;
	}
	xEventGroupSetBits(wifi_event_group, LEARN_IRDA_BIT);
	if(_learn_ir_data(ir_learned_data)){
		msg->success(msg,ir_learned_data);
	}
	xEventGroupClearBits(wifi_event_group, LEARN_IRDA_BIT);
	free(msg);
	LOG("TASK", "task_ir_learn_data stop");
	xEventGroupClearBits(wifi_event_group, LEARN_IRDA_BIT);
	vTaskDelete(NULL);
}

void task_tcpservice_ir_send(void *pvParameters){
	ref_client_msg msg = (ref_client_msg)pvParameters;
	LOG("TASK", "task_ir_send_data start");
	EventBits_t uxBits = xEventGroupWaitBits(wifi_event_group, SEND_IRDA_BIT | LEARN_IRDA_BIT , false, true, 100);
	if (LEARN_IRDA_BIT == (uxBits & LEARN_IRDA_BIT) || SEND_IRDA_BIT == (uxBits & SEND_IRDA_BIT))
	{
		LOG("IRDA", "task_tcpservice_ir_send\t\ton learning");
		vTaskDelete(NULL);
		return;
	}
	if(msg->type!=0xbf){
		LOG("IRDA", "wrong type bf!=%02x",msg->type);
		vTaskDelete(NULL);
		return;
	}
	xEventGroupSetBits(wifi_event_group,SEND_IRDA_BIT);
	int length = *((int *)msg->data);
	LOG("IRDA", "send:%d", length);
	for(int i=0;i<length;i++){
		printf("%02x ",*((uint8_t*)msg->data+i+4));
	}
	printf(" end \r\n");
	writeI2C2(msg->data + 4, length);
	LOG("TASK", "task_ir_send_data stop %d sent",length);
	xEventGroupClearBits(wifi_event_group, SEND_IRDA_BIT);
	vTaskDelete(NULL);
	return;
}

bool _learn_ir_data(uint8_t* buffer){
	int flag=0,ff_count=0,fail_times = 0;
	xEventGroupSetBits(wifi_event_group, LEARN_IRDA_BIT);
	while (fail_times < 5)
	{
		LOG("IRDA", "learn:%d\ttimes",fail_times);
		_read_ir_data(ir_learned_data,&flag,&ff_count);
		if (flag != 1  )
		{
			fail_times++;
		}
		else
		{
			if(ff_count>=4)
			{
				LOG("IRDA", "_learn_ir_data success");
				return true;
			}
			break;
		}
	}
	xEventGroupClearBits(wifi_event_group, LEARN_IRDA_BIT);
	LOG("TASK", "_learn_ir_data failed");
	return false;
}
void _read_ir_data(uint8_t* buffer, __OUT__ (int ) flag,__OUT__ (int) ff_count )
{
	int sum=0x33;

	IRDA_SET_BUSY_IN();
	memset(buffer, 0, 232);
	Learn_start2();
	vTaskDelay(50 / portTICK_RATE_MS);
	LOG("IRDA", "IRDA_BUSY_S begin");
	while (!IRDA_BUSY_S());
	LOG("IRDA", "IRDA_BUSY_S over");
	vTaskDelay(50 / portTICK_RATE_MS);
	LOG("IRDA", "readI2C2 start");
	*flag = readI2C2(buffer+1);
	buffer[0] = 0x30;
	buffer[1] = 0x03;
	for (int i=0; i < 231; i++)
	{
		sum+=buffer[i];
		if((*ff_count)<4){
			if (buffer[i] == 0xff)
			{
				(*ff_count)+=1;
			}else {
				(*ff_count)=0;
			}
		}
//		if(ff_count>4){
//			int dataLength=0;
//			while(buffer[i] == 0xff && i<231){
//				i++;
//			}
//			while(i<231 && buffer[i]!=0xF0){
//				dataLength++;
//				i++;
//			}
//		}
		printf("%02X ", buffer[i]);
	}
	printf("\r\n");
	buffer[231]=sum;
}
void msg_success_0xbf(ref_client_msg msg,void* data){

}
void msg_success_0xaf(ref_client_msg msg,void* data){
	LOG("DEBUG","msg_success_0xaf %d",__LINE__);
	msg->client->send_buffer.write(&msg->client->send_buffer,(uint8_t*)data,0,231);
	LOG("DEBUG","msg_success_0xaf %d",__LINE__);
}
bool server_recv_accept_send_process(void *server, ref_so_client client)
{
	LOG("PROC", "server_recv_accept_send_process begin");
	ref_nio_so_server self = (ref_nio_so_server)server;
	uint8_t read = client->recv_buffer.pos_read;
	uint8_t unread = client->recv_buffer.pos_write - read;
	if (unread < 0)
	{
		LOG("DEBUG", "no data to read");
		return false;
	}
	uint8_t type = 0;
	client->recv_buffer.read(&client->recv_buffer, self->buffer, 1);
	type = (uint8_t)*self->buffer;
	if (type != 0xfd)
	{
		LOG("ERROR", "0xfd");
		goto NEXT;
	}
	client->recv_buffer.read(&client->recv_buffer, self->buffer + 1, 1);
	uint8_t packageType = (uint8_t) * (self->buffer + 1);

	client->recv_buffer.read(&client->recv_buffer, self->buffer + 2, 2);
	uint16_t length = (uint16_t) * (self->buffer + 2);
	if (length != 0)
	{
		client->recv_buffer.read(&client->recv_buffer, self->buffer + 4, length);
	}
	client->recv_buffer.read(&client->recv_buffer, self->buffer + 4 + length, 1);
	type = (uint8_t) * (self->buffer + 4 + length);
	if (type != 0x00)
	{
		LOG("ERROR", "0x00");
		goto NEXT;
	}

	switch (packageType)
	{
	case 0xaf:
		LOG("INFO", "learn data");
		{
			ref_client_msg msg = new_client_msg();
			msg->client = client;
			msg->type = 0xaf;
			xQueueGenericSend(client->control_msg_queue, msg, 100, queueSEND_TO_BACK);
		}

		break;
	case 0xbf:
		LOG("INFO", "send data");
		{
			client_msg msg;
			msg.client = client;
			msg.type = 0xbf;
			msg.data = (uint8_t *)malloc(length + sizeof(int));
			int length32=length;
			// 4 byte length,'length' byte data
			memcpy(msg.data, &length32, sizeof(int));
			memcpy(msg.data + sizeof(int), client->recv_buffer.buffer + 4, length);
			xQueueGenericSend(client->control_msg_queue, &msg, 100, queueSEND_TO_BACK);
		}
	default:
		break;
	}
	client->recv_buffer.clear(&client->recv_buffer, 5 + length);
	goto OK;

NEXT:
{
	LOG("PROC", "server_recv_accept_send_process end");
	client->recv_buffer.pos_read = read;
	return false;
}
OK:
{
	LOG("PROC", "server_recv_accept_send_process end");
	return true;
}
}
bool server_client_msg_process(void *server, ref_so_client client)
{
	LOG("PROC", "server_client_msg_process begin");
	ref_nio_so_server self = (ref_nio_so_server)server;
	ref_client_msg msg = (ref_client_msg)malloc(sizeof(client_msg)); 
	xQueueReceive(client->control_msg_queue, msg, 100);
	TaskHandle_t handle;
	switch (msg->type)
	{
	case 0xaf:
		msg->success=msg_success_0xaf;
		LOG("QUEUE", "af\t\tprocess:%d", msg->type);
		xTaskCreate(&task_tcpservice_ir_learning, "task_ir_learn_data", 4096, msg, XTASK6, &handle);
		break;
	case 0xbf:
		LOG("QUEUE", "bf\t\tprocess:%d", msg->type);
		xTaskCreate(&task_tcpservice_ir_send, "task_ir_learn_data", 4096, msg, XTASK6, &handle);
		break;
	default:
		LOG("QUEUE", "none %d %d", msg->type, (int)msg);
		break;
	}
	LOG("PROC", "server_client_msg_process end");
	return true;
}




unsigned char Char2ToHex(unsigned char aChar, unsigned char bChar)
{
	if ((aChar >= 0x30) && (aChar <= 0x39))
	{
		aChar -= 0x30;
	}
	else if ((aChar >= 0x41) && (aChar <= 0x46)) // Capital
	{
		aChar -= 0x37;
	}
	else if ((aChar >= 0x61) && (aChar <= 0x66)) //littlecase
	{
		aChar -= 0x57;
	}
	else
	{
		aChar = 0xff;
	}
	if ((bChar >= 0x30) && (bChar <= 0x39))
	{
		bChar -= 0x30;
	}
	else if ((bChar >= 0x41) && (bChar <= 0x46)) // Capital
	{
		bChar -= 0x37;
	}
	else if ((bChar >= 0x61) && (bChar <= 0x66)) //littlecase
	{
		bChar -= 0x57;
	}
	else
	{
		bChar = 0xff;
	}
	aChar = (aChar << 4) + bChar;
	return aChar;
}
unsigned char CharToHex(unsigned char bHex)
{
	if (bHex <= 0x09)
	{
		bHex += 0x30;
	}
	else if ((bHex >= 10) && (bHex <= 15)) //Capital
	{
		bHex += 0x37;
	}
	else
	{
		bHex = 0xff;
	}
	return bHex;
}
unsigned char HexToChar(unsigned char bChar)
{
	if ((bChar >= 0x30) && (bChar <= 0x39))
	{
		bChar -= 0x30;
	}
	else if ((bChar >= 0x41) && (bChar <= 0x46)) // Capital
	{
		bChar -= 0x37;
	}
	else if ((bChar >= 0x61) && (bChar <= 0x66)) //littlecase
	{
		bChar -= 0x57;
	}
	else
	{
		bChar = 0xff;
	}
	return bChar;
}
