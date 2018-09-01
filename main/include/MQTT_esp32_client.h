#ifndef __HTTP_CLIENT_H
#define __HTTP_CLIENT_H


extern uint32_t time_ms;
void do_http_client(void);//TCP Clinet回环演示函数

void tcp_mqtt_conn();
void tcp_mqtt_yield();
void tcp_mqtt_report(char* propertyName, char* msg, uint32_t size);
typedef void (*OnMqttLearningCallback)(void* context,uint8_t*data,int length);
typedef struct _mqtt_context{
	 void (* onMqttLearningMsg)(struct _mqtt_context * context,OnMqttLearningCallback callback);
	 void (* onMqttSendingMsg)(struct _mqtt_context * context,uint8_t *data,int datalength);
}mqtt_context,* ptr_mqtt_context;
mqtt_context mqtt_context_str;
#endif 

