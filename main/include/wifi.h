/*
 * wifi.h
 *
 *  Created on: Dec 19, 2017
 *      Author: markin
 */

#include "lwip/ip_addr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
#include "driver/gpio.h"
#include "esp_spi_flash.h"
//所有的错误
#include "errno.h"
#include "port/arpa/inet.h"
#include "freertos/event_groups.h"
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/fcntl.h>
#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_

typedef enum {
	XTASK1 = 6,
	XTASK2,
	XTASK3,
	XTASK4,
	XTASK5,
	XTASK6,
	XTASK7,
	XTASK8,
	XTASK9,
	XTASK10,

} TASK_YX;
#define __OUT__(TYPE) TYPE *
#define LOG ESP_LOGI
#define NAME_FLASH_ADDR 0x00FFA25F
#define NAME_LEN (4 * 8)
#define FD_SET_NONBLOCK(socket)            \
    int flags = fcntl(socket, F_GETFL, 0); \
    LOG("SOCKTCP", "cntl:%d", fcntl(socket, F_SETFL, flags | O_NONBLOCK));
extern EventGroupHandle_t wifi_event_group;

extern TaskHandle_t xTaskSmartconfig;
extern TaskHandle_t xMQTTHandle, xLedHandle, xKeyHandle, xUDPipHandle, xUDPDeviceMHandle;

extern const int WIFI_CONNECTED_BIT;
extern const int WIFI_DISCONNECTED_BIT;
extern const int WIFI_REQUEST_STA_CONNECT_BIT;
extern const int WIFI_STA_GOT_IP_BIT;
extern const int ESPTOUCH_DONE_BIT;
extern const int UDP_CONNCETED_SUCCESS;
extern const int SEND_IRDA_BIT;
extern const int LEARN_IRDA_BIT;
extern const int SMART_WIFI_WORKING;

extern char *WIFI_IP_ADDR;
extern char WIFI_MAC_ADDR[13];
void newIP(struct sockaddr_in *addr, char *host, uint16_t port);
int new_socket(struct sockaddr_in *localPoint, int IPPROTO);
int get_error_code(int socket);
typedef void (*wifi_callback)(void *ctx, void *param);
typedef struct _wifi_context
{
    char ssid[50];
    char password[50];
    bool wifi_connected;
    uint16_t apRecordCount;
    wifi_ap_record_t *apRecords;
    wifi_config_t* wifi_config;
    wifi_callback onWiFiDidScan;
    wifi_callback onWiFiConnected;
    wifi_callback onWiFiDisconnected;
    wifi_callback onWiFiStart;
    wifi_callback onWiFiStop;
    wifi_callback onWiFiGotIP;
    wifi_callback onWiFiLostIP;
} wifi_context, *ref_wifi_context;
typedef struct _nonBlockSocket
{
    int socket;
    int ipProtocol;
    bool createNew;
    bool canWrite;
    int _sendErrorCode;
    int _connErrorCode;
    struct sockaddr_in localIPEndPoint;
    struct sockaddr_in remoteIPEndPoint;
    bool (*send)(struct _nonBlockSocket *socket, uint8_t *data, int dataLength, __OUT__(int) sendCount);
    void (*connect)(struct _nonBlockSocket *socket);
    void (*init)(struct _nonBlockSocket *socket);
} non_block_socket, *ref_non_block_socket;
typedef struct _list_item
{
    void *value;
    struct _list_item *next_item;
    struct _list_item *pre_item;
} list_item, *ref_list_item;
typedef void (*enum_block)(void *item, int index);
typedef struct _list
{
    int count;
    ref_list_item head;
    ref_list_item tail;
    void (*add)(struct _list *self, void *item);
    ref_list_item (*index_of)(struct _list *self, int index);
    void (*remove)(struct _list *self, void *item);
    void (*remove_at)(struct _list *self, int index);
    void (*enumerate)(struct _list *self, enum_block block);
} list, *ref_list;
typedef struct _ring_buffer
{
    uint8_t buffer[256];
    uint8_t pos_read;
    uint8_t pos_write;
    void (*write)(struct _ring_buffer *self, uint8_t *buff, int offset, int length);
    void (*read)(struct _ring_buffer *self, uint8_t *buff, int length);
    void (*clear)(struct _ring_buffer *self, int length);
} ring_buffer, *ref_ring_buffer;
typedef struct _client
{
    int so_tcp;
    QueueHandle_t *control_msg_queue;
    ring_buffer send_buffer;
    ring_buffer recv_buffer;
    struct _client *next;
    struct _client *(*elementAt)(struct _client *head, int index);
    void (*swap)(struct _client **headAddr, struct _client *head, int a, int b);
} so_client, *ref_so_client;
typedef struct _client_msg
{
    int type;
    void *data;
    ref_so_client client; 
    void (*success)(struct _client_msg * msg,void* param);
    void (*error)(struct _client_msg * msg,void* param);
} client_msg, *ref_client_msg;
typedef bool (*onDataReady)(void *server, ref_so_client client);
typedef struct _nio_so_server
{
    struct fd_set master_set;
    struct fd_set reading_set;
    struct fd_set writing_set;
    int max_fd;
    int min_fd; 
    int so_tcp; 
    struct timeval timeout;
    ref_so_client clients;
    onDataReady handlers[5];
    size_t handler_index;
    int fd_sets[5];
    size_t fd_set_index;
    uint8_t buffer[1024];
    void (*init)(struct _nio_so_server *self, char *addr, uint16_t port);
    void (*process)(struct _nio_so_server *self);
    void (*add_handler)(struct _nio_so_server *self, onDataReady callback);
} nio_so_server, *ref_nio_so_server;

ref_wifi_context new_wifi_context( );
ref_non_block_socket new_so_socket();
ref_client_msg new_client_msg();
ref_nio_so_server new_so_server();
void initialise_wifi(ref_wifi_context context);
void smartconfig_example_task(void *pvParameters);
ref_wifi_context global_wifi_context;
void smartconfig_example_task(void * parm);
#define COPY_SSID_PASSWD(wifi_config)  	\
		memcpy(global_wifi_context->ssid,wifi_config->sta.ssid,strlen((char*)&wifi_config->sta.ssid));\
		memcpy(global_wifi_context->password,wifi_config->sta.password,strlen((char*)&wifi_config->sta.password));\
		global_wifi_context->ssid[strlen((char*)&(wifi_config->sta.ssid))]='\0';\
		global_wifi_context->password[strlen((char*)&(wifi_config->sta.password))]='\0';
//void smartconfigTask(void *pvParameters);
#endif /* MAIN_WIFI_H_ */
