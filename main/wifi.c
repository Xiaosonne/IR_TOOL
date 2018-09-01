/* Esptouch example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <stdlib.h>
#include "include/wifi.h"
#include "include/key.h"
#include "include/led.h"
void list_init(struct _list *self);
ref_so_client new_so_client();
bool process1(void *server, ref_so_client client);
char *WIFI_IP_ADDR;
char WIFI_MAC_ADDR[13];
uint8_t wifi_mac[6];
const int WIFI_CONNECTED_BIT = BIT0;
const int WIFI_DISCONNECTED_BIT = BIT1;
const int WIFI_LINKED_BIT = BIT2;
const int WIFI_REQUEST_STA_CONNECT_BIT = BIT3;
const int WIFI_STA_GOT_IP_BIT = BIT4;
const int ESPTOUCH_DONE_BIT = BIT5;
const int UDP_CONNCETED_SUCCESS = BIT8;
const int SMART_WIFI_WORKING = BIT9;
/* FreeRTOS event group to signal when we are connected & ready to make a request */
EventGroupHandle_t wifi_event_group;
wifi_scan_config_t wifi_scan_config = {
	.ssid = NULL,
	.bssid = NULL,
	.channel = 0,
	.show_hidden = true,
	.scan_type = WIFI_SCAN_TYPE_ACTIVE};


/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
/* @brief indicate that the ESP32 is currently connected. */

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event);
void onWiFiDidScan(void *ctx, void *param)
{
	ref_wifi_context context = (ref_wifi_context)ctx;
	esp_wifi_scan_get_ap_num(&(context->apRecordCount));
	if (context->apRecords != NULL)
	{
		free(context->apRecords);
		context->apRecords = NULL;
	}
	LOG("WIFI", "wifiCount:%d", context->apRecordCount);
	if (context->apRecordCount > 0)
	{
		context->apRecords = (wifi_ap_record_t *)malloc(context->apRecordCount * sizeof(wifi_ap_record_t));
		esp_wifi_scan_get_ap_records(&(context->apRecordCount), context->apRecords);
		LOG("WIFI", "getApCount:%d", context->apRecordCount);
		if (context->apRecordCount > 0)
		{
			for (int i = 0; i < context->apRecordCount; i++)
			{
				LOG("DEBUG", "wifi:%s\t\trssi:%d", (context->apRecords + i)->ssid, (context->apRecords + i)->rssi);
				if (0 == strcmp((const char *)((context->apRecords + i)->ssid), context->ssid))
				{
//					wifi_config_t config;
//					memcpy(config.sta.ssid,global_wifi_context->ssid,strlen(global_wifi_context->ssid));
//					memcpy(config.sta.password,global_wifi_context->password,strlen(global_wifi_context->password));
					ESP_ERROR_CHECK( esp_wifi_disconnect() );

					ESP_ERROR_CHECK( esp_wifi_connect() );
					return;
				}
			}
		}
	}
	LOG("WIFI", "nextTermScan");
	esp_wifi_scan_start(&wifi_scan_config, false);
}
void onWiFiConnected(void *ctx, void *param)
{
	ref_wifi_context context = (ref_wifi_context)ctx;
}
void onWiFiDisconnected(void *ctx, void *param)
{
	ref_wifi_context context = (ref_wifi_context)ctx;

	context->wifi_connected = false;
	ESP_ERROR_CHECK(esp_wifi_connect());
}
void onWiFiStart(void *ctx, void *param)
{
	ref_wifi_context context = (ref_wifi_context)ctx;
	ESP_ERROR_CHECK(esp_wifi_connect());
}
void onWiFiStop(void *ctx, void *param)
{
	ref_wifi_context context = (ref_wifi_context)ctx;
}
void onWiFiGotIP(void *ctx, void *param)
{
	system_event_t *event = (system_event_t *)param;
	// ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP!");
	// ESP_LOGI(TAG, "got ip:%s\n",
	// 		 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
	ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, wifi_mac));
	sprintf(WIFI_MAC_ADDR, "%02X%02X%02X%02X%02X%02X",
			wifi_mac[0],
			wifi_mac[1],
			wifi_mac[2],
			wifi_mac[3],
			wifi_mac[4],
			wifi_mac[5]);
	WIFI_IP_ADDR = ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip);
	ref_wifi_context context = (ref_wifi_context)ctx;
	context->wifi_connected = true;
}
void onWiFiLostIP(void *ctx, void *param)
{
	ref_wifi_context context = (ref_wifi_context)ctx;
	context->wifi_connected = false;
}

  int get_error_code(int socket)
{
	static const char *TAG = "udp";
	int result = -1;
	uint32_t optlen = sizeof(int);
	if (getsockopt(socket, SOL_SOCKET, SO_ERROR, &result, &optlen) == -1)
	{
		ESP_LOGE(TAG, "getsockopt failed");
		return -1;
	}
	return result;
}
 int new_socket(struct sockaddr_in *localPoint, int IPPROTO)
{
	int fd = 0;
	if (IPPROTO == IPPROTO_UDP)
		fd = socket(AF_INET, SOCK_DGRAM, IPPROTO);
	if (IPPROTO == IPPROTO_TCP)
		fd = socket(AF_INET, SOCK_STREAM, IPPROTO);
	if (bind(fd, localPoint, sizeof(struct sockaddr)) < 0)
	{
		int err = get_error_code(fd);
		LOG("SOCKET", "protocol:%d\terr:%d\terrMsg:%s", IPPROTO, err, strerror(err));
		close(fd);
		return 0;
	}
	LOG("SOCKET", "newSocket:%d", fd);
	return fd;
}
static bool so_send(struct _nonBlockSocket *socket, uint8_t *data, int dataLength, __OUT__(int) sendCount)
{
	LOG("SOCKTCP", "so_send");
	*sendCount = send(socket->socket, data, dataLength, 0);
	socket->_sendErrorCode = get_error_code(socket->socket);
	LOG("SOCKTCP", "tcpSend:%d\t\terror:%d\t\terrStr:%s", *sendCount, socket->_sendErrorCode, lwip_strerr(socket->_sendErrorCode));
	socket->createNew = (socket->_sendErrorCode != 0 && socket->_sendErrorCode != EWOULDBLOCK);
	return !socket->createNew;
}
static void so_connect(struct _nonBlockSocket *socket)
{
	LOG("SOCKTCP", "so_connect");
	int con = connect(socket->socket, &(socket->remoteIPEndPoint), sizeof(socket->remoteIPEndPoint));
	socket->_connErrorCode = get_error_code(socket->socket);
	socket->_sendErrorCode = 0;
	LOG("SOCKTCP", "con:%d\t\tconnErr:%d", con, socket->_connErrorCode);
	socket->canWrite = (con == 0 || socket->_connErrorCode == 0 || socket->_connErrorCode == EISCONN || socket->_connErrorCode == EALREADY);
}
static void so_init(struct _nonBlockSocket *socket)
{
	LOG("SOCKTCP", "so_init");
	if (socket->socket != 0)
		close(socket->socket);
	socket->socket = new_socket(&(socket->localIPEndPoint), socket->ipProtocol);
	socket->_connErrorCode = -1;
	socket->canWrite = false;
	FD_SET_NONBLOCK((socket->socket));
}

void newIP(struct sockaddr_in *addr, char *host, uint16_t port)
{
	bzero(addr, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = inet_addr(host);
}
bool need_to_close(int socket, int returnCode, int errorCode)
{
	if (returnCode < 0)
	{
		if (errorCode == EWOULDBLOCK)
		{
			LOG("SOCKET", "socket:%d\twould block", socket);
			return false;
		}
		LOG("SOCKET", "socket:%d\twould close", socket);
		return true;
	}
	if (returnCode == 0)
	{
		LOG("SOCKET", "socket:%d\twould close\tzero recv", socket);
		return true;
	}
	return false;
}

uint8_t sendData[512] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

static void sc_callback(smartconfig_status_t status, void *pdata)
{
    switch (status) {
        case SC_STATUS_WAIT:
            ESP_LOGI("SMART_CONFIG", "SC_STATUS_WAIT");
            break;
        case SC_STATUS_FIND_CHANNEL:
            ESP_LOGI("SMART_CONFIG", "SC_STATUS_FINDING_CHANNEL");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            ESP_LOGI("SMART_CONFIG", "SC_STATUS_GETTING_SSID_PSWD");
            break;
        case SC_STATUS_LINK:
            ESP_LOGI("SMART_CONFIG", "SC_STATUS_LINK");
            {
            	wifi_config_t *wifi_config = pdata;
            	LOG("SMART_CONFIG","Assid:%s\tlength:%d\tpaswd:%s\tlength2:%d",
            			wifi_config->sta.ssid,
						strlen((char*)&wifi_config->sta.ssid),
						wifi_config->sta.password,
						strlen((char*)&wifi_config->sta.password));
            	ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA,wifi_config) );
//            	memcpy(global_wifi_context->ssid,wifi_config->sta.ssid,strlen((char*)&wifi_config->sta.ssid));
//            	memcpy(global_wifi_context->password,wifi_config->sta.password,strlen((char*)&wifi_config->sta.password));
//            	global_wifi_context->ssid[strlen((char*)&(wifi_config->sta.ssid))]='\0';
//            	global_wifi_context->password[strlen((char*)&(wifi_config->sta.password))]='\0';
            	COPY_SSID_PASSWD(wifi_config);
            	LOG("SMART_CONFIG","Bssid:%s\tlength:%d\tpaswd:%s\tlength2:%d",
            			global_wifi_context->ssid,
						strlen((char*)&global_wifi_context->ssid),
						global_wifi_context->password,
						strlen((char*)&global_wifi_context->password));

            	esp_wifi_scan_start(&wifi_scan_config, false);
            }

            break;
        case SC_STATUS_LINK_OVER:
            ESP_LOGI("SMART_CONFIG", "SC_STATUS_LINK_OVER");
            xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
            break;
        default:
            break;
    }
}
void smartconfig_example_task(void * parm)
{
	ESP_LOGI("SMART_CONFIG", "smartconfig_example_task begin");
	EventBits_t uxBits = xEventGroupWaitBits(wifi_event_group, SMART_WIFI_WORKING, false, false, 10);
	if(SMART_WIFI_WORKING==(uxBits & SMART_WIFI_WORKING)) {
		ESP_LOGI("SMART_CONFIG", "smartconfig_example_task working");
		vTaskDelete(NULL);
		return;
	}
	xEventGroupSetBits(wifi_event_group,SMART_WIFI_WORKING);
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS) );
    ESP_ERROR_CHECK( esp_smartconfig_start(sc_callback) );
    while (1) {
        uxBits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if(WIFI_CONNECTED_BIT ==(uxBits & WIFI_CONNECTED_BIT)) {
            ESP_LOGI("SMART_CONFIG", "WiFi Connected to ap");
        }
        if(ESPTOUCH_DONE_BIT==(uxBits & ESPTOUCH_DONE_BIT)) {
            ESP_LOGI("SMART_CONFIG", "smartconfig over");
            esp_smartconfig_stop();
            break;
        }
    }
    ESP_LOGI("SMART_CONFIG", "smartconfig_example_task end");
    xEventGroupClearBits(wifi_event_group,SMART_WIFI_WORKING);
    vTaskDelete(NULL);
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
	ref_wifi_context ref = (ref_wifi_context)ctx;

	switch (event->event_id)
	{
	case SYSTEM_EVENT_STA_START:
		LOG("EVENT", "SYSTEM_EVENT_STA_START");
		ref->onWiFiStart(ctx, event);
		break;
	case SYSTEM_EVENT_STA_STOP:
		LOG("EVENT", "SYSTEM_EVENT_STA_STOP");
		ref->onWiFiStop(ctx, event);
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		LOG("EVENT", "SYSTEM_EVENT_STA_GOT_IP");
		ref->onWiFiGotIP(ctx, event);
		break;
	case SYSTEM_EVENT_STA_LOST_IP:
		LOG("EVENT", "SYSTEM_EVENT_STA_LOST_IP");
		ref->onWiFiLostIP(ctx, event);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		LOG("EVENT", "SYSTEM_EVENT_STA_DISCONNECTED");
		ref->onWiFiDisconnected(ctx, event);
		break;
	case SYSTEM_EVENT_SCAN_DONE:
		LOG("EVENT", "SYSTEM_EVENT_SCAN_DONE");
		ref->onWiFiDidScan(ctx, event);
		break;
	case SYSTEM_EVENT_STA_CONNECTED:
		LOG("EVENT", "SYSTEM_EVENT_STA_CONNECTED");
		ref->onWiFiConnected(ctx, event);
		break;
	default:
		LOG("EVENT", "EVENTID:%d", event->event_id);
		break;
	}
	return ESP_OK;
}

void list_add(struct _list *self, void *item)
{
	if (self->head == NULL)
	{
		self->head = (ref_list_item)malloc(sizeof(list_item));
		//LOG("DEBUG", "run %d ", __LINE__);
		self->head->pre_item = NULL;
		//LOG("DEBUG", "run %d ", __LINE__);
		self->head->next_item = NULL;
		//LOG("DEBUG", "run %d ", __LINE__);
		self->head->value = item;
		//LOG("DEBUG", "run %d ", __LINE__);
		self->tail = self->head;
		//LOG("DEBUG", "run %d ", __LINE__);
		self->count++;
		//LOG("DEBUG", "run %d ", __LINE__);
		return;
	}
	ref_list_item newNode = (ref_list_item)malloc(sizeof(list_item));
	newNode->pre_item = self->tail;
	newNode->next_item = NULL;
	newNode->value = item;

	self->tail->next_item = newNode;
	self->tail = newNode;
	self->count++;
}
void _remove_item(struct _list *self, ref_list_item temp)
{
	if (temp->pre_item != NULL)
	{
		temp->pre_item->next_item = temp->next_item;
	}
	if (temp->next_item != NULL)
	{
		temp->next_item->pre_item = temp->pre_item;
	}
	if (temp == self->head)
	{
		self->head = temp->next_item;
	}
	if (temp == self->tail)
	{
		self->tail = temp->pre_item;
	}
	free(temp);
}
void so_list_remove(struct _list *self, void *item)
{
	ref_list_item temp = self->head;
	while (temp != NULL)
	{
		if (temp->value == item)
		{
			_remove_item(self, temp);
			free(temp);
			break;
		}
		else
		{
			temp = temp->next_item;
		}
	}
}

ref_list_item list_index_of(struct _list *self, int index)
{
	int i = 0;
	//LOG("DEBUG", "run %d", __LINE__);
	ref_list_item temp = self->head;
	//LOG("DEBUG", "run %d", __LINE__);
	while (temp != NULL && i < index)
	{
		i++;
		//LOG("DEBUG", "run %d", __LINE__);
	}
	if (temp != NULL && i == index)
	{
		return temp;
		//LOG("DEBUG", "run %d", __LINE__);
	}
	return NULL;
	//LOG("DEBUG", "run %d", __LINE__);
}
void list_remove_at(struct _list *self, int index)
{
	int i = 0;
	ref_list_item temp = self->head;
	while (temp != NULL && i < index)
	{
		i++;
	}
	if (temp != NULL && i == index)
	{
		_remove_item(self, temp);
		free(temp);
	}
}
void list_enumerate(struct _list *self, enum_block block)
{
	int i = 0;
	ref_list_item temp = self->head;
	while (temp != NULL)
	{
		block(temp->value, i++);
	}
}
void list_init(struct _list *self)
{
	self->head = NULL;
	self->tail = NULL;
	self->count = 0;
	self->add = list_add;
	self->remove = so_list_remove;
	self->remove_at = list_remove_at;
	self->enumerate = list_enumerate;
	self->index_of = list_index_of;
}
void so_client_write(struct _ring_buffer *self, uint8_t *buff, int offset, int length)
{
	uint8_t left = ((uint8_t)255 - self->pos_write);
	int write = length > left ? left : length;
	int other = length > left ? (length - left) : 0;
	LOG("DEBUG", "so_client_write\t\tleft:%d\twrite:%d\tother:%d\t", left, write, other);
	memcpy(self->buffer + self->pos_write, buff + offset, write);
	self->pos_write += write;
	if (other > 0)
	{
		memcpy(self->buffer + self->pos_write, buff + offset + write, other);
	}
	LOG("DEBUG", "so_client_write over");
}
void so_client_read(struct _ring_buffer *self, uint8_t *buff, int length)
{
	uint8_t left = ((uint8_t)255 - self->pos_read);
	int write = length > left ? left : length;
	int other = length > left ? (length - left) : 0;
	LOG("DEBUG", "so_client_read\t\tleft:%d\tread:%d\tother:%d\t", left, write, other);
	memcpy(buff, self->buffer + self->pos_read, write);
	self->pos_read += write;
	if (other > 0)
	{
		memcpy(buff, self->buffer + self->pos_read, other);
	}
	LOG("DEBUG", "so_client_read over");
}
void so_client_clear(struct _ring_buffer *self, int length)
{
	self->pos_read = 0;
	self->pos_write = self->pos_write - length;
	memcpy(self->buffer, self->buffer + length, ((uint8_t)255 - length));
}
ref_so_client so_client_element_at(ref_so_client self, int index)
{
	ref_so_client ret = self;
	int i = 1;
	while (i < index)
	{
		ret = ret->next;
		if (ret == NULL)
			return NULL;
		i++;
	}
	return ret;
}
void so_client_element_swap(ref_so_client *head, ref_so_client self, int a, int b)
{
	if (a >= b)
		return;

	ref_so_client tempA = self->elementAt(self, a);
	ref_so_client tempANext = tempA->next;
	ref_so_client tempB = self->elementAt(self, b);
	ref_so_client tempBNext = tempB->next;
	ref_so_client tempC = self->elementAt(self, b - 1);
	tempC->next = tempA;
	tempA->next = tempB->next;
	tempB->next = tempA->next;
	if (a == 0)
		*head = tempB;
}
void so_server_init(struct _nio_so_server *self, char *addr, uint16_t port)
{
	struct sockaddr_in tcpEndPoint;
	newIP(&(tcpEndPoint), addr, port);
	self->so_tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//socket.init(&socket);
	int on = 1;
	if (setsockopt(self->so_tcp, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(on)) < 0)
	{
		LOG("ERROR", "setOpt:%d", errno);
	}
	if (bind(self->so_tcp, &(tcpEndPoint), sizeof(tcpEndPoint)) < 0)
	{
		LOG("ERROR", "bindErr:%d", errno);
	}
	if (listen(self->so_tcp, 10) < 0)
	{
		LOG("ERROR", "lisErr:%d", errno);
	}
	self->max_fd = self->so_tcp;
	self->min_fd = self->so_tcp;
	self->timeout.tv_sec = 0;
	self->timeout.tv_usec = 100;
	FD_SET(self->so_tcp, &self->master_set);
	//LOG("DEBUG", "run %d", __LINE__);
	FD_SET_NONBLOCK(self->so_tcp);
	self->fd_sets[self->fd_set_index] = self->so_tcp;
	self->fd_set_index++;
	//LOG("DEBUG", "run %d", __LINE__);
}
void so_server_process(struct _nio_so_server *self)
{
	FD_ZERO(&self->reading_set);
	FD_ZERO(&self->writing_set);
	memcpy(&self->reading_set, &self->master_set, sizeof(self->master_set));
	memcpy(&self->writing_set, &self->master_set, sizeof(self->master_set));
	int rc = select(self->max_fd + 1, &self->reading_set, NULL, NULL, &self->timeout);
	int rc_write = select(self->max_fd + 1, NULL, &self->writing_set, NULL, &self->timeout);
	int recvCount = 0;
	for (size_t i = 0; i < self->fd_set_index; i++)
	{
		int tempSocket = self->fd_sets[i];
		// ref_list_item item = list->index_of(list, i);
		// int tempSocket = ((ref_so_client)item->value)->so_tcp;
		bool closed = false;
		if (rc > 0 && FD_ISSET(tempSocket, &self->reading_set))
		{ 
			if (tempSocket == self->so_tcp)
			{
				LOG("SOCKET", "server A");
				do
				{ 
					int client = accept(self->so_tcp, NULL, NULL); 
					int error = errno; 
					if (client < 0)
					{
						if (error != EWOULDBLOCK)
						{
							LOG("SOCKET", "Accept:\terr%d", error);
						}
						break;
					}
					if (client > 0)
					{ 
						FD_SET(client, &self->master_set);
						(self->clients + self->fd_set_index)->so_tcp = client;
						self->fd_sets[self->fd_set_index++] = client; 
					} 
					self->max_fd = self->max_fd > client ? self->max_fd : client;
					self->min_fd = client < self->min_fd ? client : self->min_fd;
					LOG("SOCKET", "Accept\terr:%d\tso:%d", error, client);
				} while (true);
			}
			else
			{

				recvCount = recv(tempSocket, self->buffer, 1024, 0);
				int error = errno;
				closed = need_to_close(tempSocket, recvCount, error);
				LOG("SOCKET", "socket:%d\trecv:%d\terr:%d\tclose:%d\tdata:%s", tempSocket, recvCount, error, closed, self->buffer);
			}
		}
		ref_so_client client = self->clients->elementAt(self->clients, i);
		if (tempSocket != self->so_tcp && rc_write > 0 && FD_ISSET(tempSocket, &self->writing_set))
		{
			int canWrite=client->send_buffer.pos_write-client->send_buffer.pos_read;
			if(canWrite>0){
				LOG("SOCKET", "socket:%d\t send:%d \tbegin", tempSocket, canWrite);
				client->send_buffer.read(&client->send_buffer,sendData,canWrite);
				client->send_buffer.clear(&client->send_buffer,canWrite);
				int sd = send(tempSocket, sendData, canWrite, 0);
				int error = errno;
				closed = need_to_close(tempSocket, sd, error);
				LOG("SOCKET", "socket:%d\tsend:%d\terr:%d\tclose:%d", tempSocket, sd, error, closed);
			}
		}
		if (closed)
		{
			client->recv_buffer.pos_read = 0;
			client->recv_buffer.pos_write = 0;
			client->send_buffer.pos_read = 0;
			client->send_buffer.pos_write = 0;

			self->fd_sets[i] = self->fd_sets[self->fd_set_index - 1];
			self->fd_sets[self->fd_set_index - 1] = 0;
			self->fd_set_index--;
			self->clients->swap(&self->clients, self->clients, i, self->fd_set_index - 1);
			self->max_fd = self->max_fd > (tempSocket - 1) ? self->max_fd : (tempSocket - 1);
			self->min_fd = (tempSocket - 1) < self->min_fd ? (tempSocket - 1) : self->min_fd;
			if (FD_ISSET(tempSocket, &self->master_set))
			{
				FD_CLR(tempSocket, &self->master_set);
				int rc1 = close(tempSocket);
				LOG("SOCKET", "REMOVE:%d,%d", tempSocket, rc1);
			}
			else
			{
				LOG("SOCKET", "NotInMFD:%d", tempSocket);
			}
			continue;
		} 
		if (recvCount > 0)
		{
			client->recv_buffer.write(&client->recv_buffer, self->buffer, 0, recvCount);
		}
		int bytes = client->recv_buffer.pos_write - client->recv_buffer.pos_read;
		if ((bytes) <= 0)
		{
			continue;
		}
		LOG("SOCKET", "bytes to process %d", bytes);
		for (size_t i = 0; i < self->handler_index; i++)
		{
			LOG("SOCKET", "handler:%d\tprocess begin",i);
			self->handlers[i](self, client);
			LOG("SOCKET", "handler:%d\tprocess end",i); 
		}
	}
}

void so_server_add_handler(struct _nio_so_server *self, onDataReady callback)
{
	self->handlers[self->handler_index++] = callback;
}
ref_wifi_context new_wifi_context()
{
	ref_wifi_context ctx = (ref_wifi_context)malloc(sizeof(wifi_context));
	ctx->wifi_connected = false;
	ctx->apRecordCount = 0;
	ctx->apRecords = NULL;
	ctx->onWiFiDidScan = onWiFiDidScan;
	ctx->onWiFiConnected = onWiFiConnected;
	ctx->onWiFiDisconnected = onWiFiDisconnected;
	ctx->onWiFiStart = onWiFiStart;
	ctx->onWiFiStop = onWiFiStop;
	ctx->onWiFiGotIP = onWiFiGotIP;
	ctx->onWiFiLostIP = onWiFiLostIP;
	return ctx;
}
ref_non_block_socket new_so_socket()
{
	ref_non_block_socket socket = (ref_non_block_socket)malloc(sizeof(non_block_socket));
	socket->canWrite = false;
	socket->createNew = false;
	socket->ipProtocol = IPPROTO_TCP;
	socket->connect = so_connect;
	socket->send = so_send;
	socket->init = so_init;
	return socket;
}
ref_nio_so_server new_so_server()
{
	ref_nio_so_server ser = (ref_nio_so_server)malloc(sizeof(nio_so_server));
	ser->fd_set_index = 0;
	ser->handler_index = 0;
	memset(ser->fd_sets, 0, sizeof(ser->fd_sets));
	memset(ser->handlers, 0, sizeof(ser->handlers));
	ser->init = so_server_init;
	ser->process = so_server_process;
	ser->add_handler = so_server_add_handler;

	ser->clients = new_so_client();
	;
	ref_so_client clients = ser->clients;
	ref_so_client temp = clients;
	int cts = 1;
	while (cts < 5)
	{
		temp->next = new_so_client();
		temp = temp->next;
		cts++;
	}
	return ser;
}
ref_so_client new_so_client()
{
	ref_so_client client = (ref_so_client)malloc(sizeof(so_client));
	client->control_msg_queue = xQueueCreate(5, sizeof(client_msg));
	client->recv_buffer.read = so_client_read;
	client->recv_buffer.write = so_client_write;
	client->send_buffer.read = so_client_read;
	client->send_buffer.write = so_client_write;
	client->send_buffer.clear = so_client_clear;
	client->recv_buffer.clear = so_client_clear;
	client->recv_buffer.pos_read = 0;
	client->recv_buffer.pos_write = 0;
	client->send_buffer.pos_read = 0;
	client->send_buffer.pos_write = 0;
	client->next = NULL;
	client->elementAt = so_client_element_at;
	client->swap = so_client_element_swap;
	return client;
}

ref_client_msg new_client_msg(){
	ref_client_msg msg=(ref_client_msg)malloc(sizeof(client_msg));
	return msg;
}

void initialise_wifi(ref_wifi_context context)
{
	//int so_udp = 0, so_tcp = 0;
	// struct sockaddr_in udpEndPoint, broadcast, mypc, clientIP;
	// newIP(&udpEndPoint, "0.0.0.0", 8000);
	// newIP(&broadcast, "255.255.255.255", 8000);
	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, context));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));  
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());
}
