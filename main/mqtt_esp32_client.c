/**
******************************************************************************
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>


// #include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "esp_event_loop.h"
#include "include/MQTT_esp32_client.h"
#include "esp_log.h"
#include "errno.h"

#include "include/wifi.h"
#include "include/MQTT_export.h"


static iotx_mqtt_param_t param;
static char wbuffer[2048];
static char rbuffer[2048];
static void* pmqtt = 0;
static uint8_t host[] = "192.168.1.216";//"47.92.73.208";
static uint16_t port = 1883;
static char productKey[] = "IRT";
static char client_id[64];
static uint8_t reconnected = 0;
static const char *TAG = "mqtt_client_tcp:";


// ++ per ms
uint32_t time_ms = 0;

uint32_t time_get_time(void)
{
    return esp_log_timestamp();
}

int tcp_esp32_connect(char* ippp, uint16_t poport)
{
	ESP_LOGI(TAG, "mqtt_client_tcp ");

	int sock=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	serverAddress.sin_addr.s_addr = inet_addr((char*)host);
	int rc = connect(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
	if(rc < 0)
	{
		ESP_LOGE(TAG,"connect: %d %s",rc,strerror(errno));
		return -1;
	}
    struct timeval timeout={2,0};//3s
    //int ret=setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
    int ret=setsockopt(sock,SOL_SOCKET, SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));


	reconnected = 1;
    return sock;
}

int tcp_esp32_read(int32_t fd, char* buffer, uint32_t size, uint32_t timeout)
{
    int ret = 0;
    int32_t readBytes = 0;
	readBytes = read(fd, buffer, size);
	if(readBytes <= 0)
	{
		readBytes = 0;
	}
	ret += readBytes;

    return ret;
}

int tcp_esp32_write(int32_t fd, char* buffer, uint32_t size, uint32_t timeout)
{
	ESP_LOGI(TAG, "mqtt_client_write %d", size);
    return send(fd, (uint8_t *)buffer, size, 0);
}

void tcp_esp32_disconnect(int32_t fd)
{
    close(fd);
}

void handle_msg(void* pcontext, void* pclient, iotx_mqtt_event_msg_pt msg)
{

}
void _on_mqtt_learning_callback(void* context,uint8_t*data,int length)
{
	LOG("MQTT","leaning callback begin");
	tcp_mqtt_report((char*)"learn",(char*)data,length);
	LOG("MQTT","leaning callback e n d");
}
void handle_switch(void* pcontext, void* pclient, iotx_mqtt_event_msg_pt msg)
{
	LOG("MQTT","handle_switch\t%d",__LINE__);
    char tmp[256];
    LOG("MQTT","handle_switch\t%d",__LINE__);int i,j;
    if(msg->event_type == IOTX_MQTT_EVENT_PUBLISH_RECVEIVED)
    {
    	LOG("MQTT","handle_switch\t%d",__LINE__);
    	iotx_mqtt_topic_info_pt topicMsg = (iotx_mqtt_topic_info_t*)msg->msg;
    	LOG("MQTT","handle_switch\t%d",__LINE__);
    	LOG("MQTT","handle_switch\t%d\ttopicMsg==NULL:%d\ttopicMsg->ptopic:%s",__LINE__,topicMsg==NULL,topicMsg->ptopic);
    	LOG("MQTT","handle_switch\ttopicMsg->ptopic:%s",topicMsg->ptopic);
    	LOG("MQTT","handle_switch\ttopicMsg->payload:%s",topicMsg->payload);
    	LOG("MQTT","handle_switch\ttopicMsg->payload_len:%d",topicMsg->payload_len);
        if(NULL != strstr(topicMsg->ptopic, "learn"))
        {
        	LOG("MQTT","handle_switch\t%d",__LINE__);
//			if(TCP_LearnIR == NULL && ( xSemaphore != NULL))
//            {
//                if( xSemaphoreTake (xSemaphore,( TickType_t ) 100) == pdTRUE)
//                {
//                    learn_from_MQTT_flag = 1;
//                    xTaskCreate(&vTasklearnIRDA1, "vTasklearnIRDA1", 2048 , NULL, XTASK7, &TCP_LearnIR);
//                }
//            }
        	LOG("MQTT","recvd\tleaning:\t%s",topicMsg->ptopic);
			mqtt_context_str.onMqttLearningMsg(&mqtt_context_str,_on_mqtt_learning_callback);
			LOG("MQTT","e n d\tleaning:\t%s",topicMsg->ptopic);
        	// tcp_mqtt_report("learn", (char*)learndata, sizeof(learndata));
        }
        else
        {
//            irtxlen = topicMsg->payload_len;
//            memset(irda_data, 0 ,sizeof(irda_data));
//            memcpy(irda_data, topicMsg->payload, topicMsg->payload_len);

        	LOG("MQTT","handle_switch\t%d",__LINE__);

        	LOG("MQTT","handle_switch\t%d",__LINE__);
        	uint8_t data[topicMsg->payload_len+1];
        	memcpy(data,topicMsg->payload,topicMsg->payload_len);
        	int length=topicMsg->payload_len;
        	LOG("MQTT","handle_switch\t%d\tlength:%d",__LINE__,length);

        	for(int i=0;i<length;i++){
        		printf("%02x ",*(data+i));
        	}
        	printf("\r\n");

            mqtt_context_str.onMqttSendingMsg(&mqtt_context_str,data,length);
            LOG("MQTT","handle_switch\t%d",__LINE__);
            //xTaskCreate(&vTaskIRDA1, "vTaskIRDA1", 2048 ,NULL , XTASK8, NULL);
        }



//			topicMsg->ptopic  ����
//			topicMsg->topic_len  ���ⳤ��
//			topicMsg->payload_len  ��Ϣ�ֽڳ���
//			topicMsg->payload  ��Ϣ�ֽ�����

//			for(i = 0; i < topicMsg->topic_len; ++i)
//			{
//				tmp[i] = (char)topicMsg->ptopic[i];
//			}
//			tmp[i] = '\0';
//
//			printf("topic %s\n", tmp);

//       for(i = 0; i < topicMsg->payload_len; ++i)
//       {
//           tmp[i] = (char)topicMsg->payload[i];
//       }
//       i=0;
//       RS485_TX_EN = 1;
//		
//       for (j = 0; j<topicMsg->payload_len; ++j)
//       {
//						USART_ClearFlag(USART2, USART_FLAG_TC);
//            USART_SendData(USART2, tmp[j]); //�򴮿�2��������
//            while (USART_GetFlagStatus(USART2, USART_FLAG_TC) != SET); //�ȴ����ͽ���
//       }
//			delay_ms(20);
//      RS485_TX_EN = 0;
		
//			tmp[i] = '\0';
//			printf("msg %s\n", tmp);
    }
    LOG("MQTT","handle_switch\t%d",__LINE__);
}
void tcp_mqtt_conn()
{
    param.port = port;
    param.host = host;
    sprintf(client_id,"%s/%s", productKey, WIFI_MAC_ADDR);
    param.client_id = client_id;
    param.clean_session = 1;
    param.request_timeout_ms = 5000;
    param.keepalive_interval_ms = 2000;
    param.pwrite_buf = wbuffer;
    param.write_buf_size = 2048;
    param.pread_buf = rbuffer;
    param.read_buf_size = 2048;

    param.handle_event.h_fp = handle_msg;
    param.handle_event.pcontext = 0;

    param.read = tcp_esp32_read;
    param.write = tcp_esp32_write;
    param.connect = tcp_esp32_connect;
    param.disconnect = tcp_esp32_disconnect;

    pmqtt = IOT_MQTT_Construct(&param);
}

void tcp_mqtt_yield()
{
	//LOG("MQTT","yield %d",__LINE__);
	char topic[64];
	if(NULL != pmqtt)
	{
		//LOG("MQTT","yield %d",__LINE__);
		IOT_MQTT_Yield(pmqtt, 5);
		//LOG("MQTT","yield %d",__LINE__);
		if(reconnected == 1)
		{
			//LOG("MQTT","yield %d",__LINE__);
			reconnected = 0;
			//LOG("MQTT","yield %d",__LINE__);
			memset(topic, 0, sizeof(topic));

			//LOG("MQTT","yield %d",__LINE__);
			snprintf(topic, sizeof(topic), "execute/service/%s/#", client_id);

			//LOG("MQTT","yield %d",__LINE__);
			ESP_LOGI(TAG, "mqtt_client_subscribe");
			//LOG("MQTT","yield %d",__LINE__);
			IOT_MQTT_Subscribe(pmqtt, topic, 1, handle_switch, &mqtt_context_str);
			//LOG("MQTT","yield %d",__LINE__);
		}
	}
	else
	{
		//LOG("MQTT","yield %d",__LINE__);
		vTaskDelay(20000 / portTICK_RATE_MS);
		tcp_mqtt_conn();
		//LOG("MQTT","yield %d",__LINE__);

	}
}

void tcp_mqtt_report( char* propertyName, char* msg, uint32_t size)
{
    char topic[64];
    iotx_mqtt_topic_info_t mqtt_msg;

    snprintf(topic, sizeof(topic), "report/meta/%s/%s", client_id, propertyName);
    mqtt_msg.qos = 1;
    mqtt_msg.topic_len = strlen(topic);
    mqtt_msg.payload_len = size;
    mqtt_msg.ptopic = topic;
    mqtt_msg.payload = msg;

    IOT_MQTT_Publish(pmqtt, topic, &mqtt_msg);
}
void tcp_mqtt_disconn()
{
    IOT_MQTT_Destroy(pmqtt);
}

