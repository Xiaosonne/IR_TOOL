

#include <string.h>

#include "include/utils_net.h"

/*** TCP connection ***/
int read_tcp(utils_network_pt pNetwork, char *buffer, uint32_t len, uint32_t timeout_ms)
{
    return pNetwork->socket_read(pNetwork->handle, buffer, len, timeout_ms);
}


static int write_tcp(utils_network_pt pNetwork, const char *buffer, uint32_t len, uint32_t timeout_ms)
{
    return pNetwork->socket_write(pNetwork->handle, buffer, len, timeout_ms);
}

static int disconnect_tcp(utils_network_pt pNetwork)
{
    if (0 == pNetwork->handle) {
        return -1;
    }

    pNetwork->socket_disconnect(pNetwork->handle);
    pNetwork->handle = 0;
    return 0;
}

static int connect_tcp(utils_network_pt pNetwork)
{
    if (NULL == pNetwork) {
        return 1;
    }

    pNetwork->handle = pNetwork->socket_connect(pNetwork->pHostAddress, pNetwork->port);
    if (0 == pNetwork->handle) {
        return -1;
    }

    return 0;
}

/****** network interface ******/
int utils_net_read(utils_network_pt pNetwork, char *buffer, uint32_t len, uint32_t timeout_ms)
{
    int     ret = 0;

    ret = read_tcp(pNetwork, buffer, len, timeout_ms);

    return ret;
}

int utils_net_write(utils_network_pt pNetwork, const char *buffer, uint32_t len, uint32_t timeout_ms)
{
    int     ret = 0;

    ret = write_tcp(pNetwork, buffer, len, timeout_ms);

    return ret;
}

int iotx_net_disconnect(utils_network_pt pNetwork)
{
    int     ret = 0;

    ret = disconnect_tcp(pNetwork);

    return  ret;
}

int iotx_net_connect(utils_network_pt pNetwork)
{
    int     ret = 0;
    
    ret = connect_tcp(pNetwork);

    return ret;
}

int iotx_net_init(utils_network_pt pNetwork, iotx_mqtt_param_pt param)
{
    if (!pNetwork || !param->host) {
        return -1;
    }
    pNetwork->pHostAddress = param->host;
    pNetwork->port = param->port;


    pNetwork->handle = 0;
    pNetwork->read = utils_net_read;
    pNetwork->write = utils_net_write;
    pNetwork->disconnect = iotx_net_disconnect;
    pNetwork->connect = iotx_net_connect;

    pNetwork->socket_connect = param->connect;
    pNetwork->socket_disconnect = param->disconnect;
    pNetwork->socket_read = param->read;
    pNetwork->socket_write = param->write;

    return 0;
}
