#ifndef _IOTX_COMMON_NET_H_
#define _IOTX_COMMON_NET_H_

#include <stdint.h>

#include "MQTT_export.h"

/**
 * @brief The structure of network connection(TCP or SSL).
 *   The user has to allocate memory for this structure.
 */

struct utils_network;
typedef struct utils_network utils_network_t, *utils_network_pt;

struct utils_network {
    const uint8_t *pHostAddress;
    uint16_t port;

    /**< connection handle: 0, NOT connection; NOT 0, handle of the connection */
    int32_t handle;


    /**< Read data from server function pointer. */
    int (*socket_read)(int32_t, char *, uint32_t, uint32_t);

    /**< Send data to server function pointer. */
    int (*socket_write)(int32_t, const char *, uint32_t, uint32_t);

    /**< Disconnect the network */
    void (*socket_disconnect)(int32_t);

    /**< Establish the network */
    int (*socket_connect)(const uint8_t*, uint16_t);


    /**< Read data from server function pointer. */
    int (*read)(utils_network_pt, char *, uint32_t, uint32_t);

    /**< Send data to server function pointer. */
    int (*write)(utils_network_pt, const char *, uint32_t, uint32_t);

    /**< Disconnect the network */
    int (*disconnect)(utils_network_pt);

    /**< Establish the network */
    int (*connect)(utils_network_pt);
};


int utils_net_read(utils_network_pt pNetwork, char *buffer, uint32_t len, uint32_t timeout_ms);
int utils_net_write(utils_network_pt pNetwork, const char *buffer, uint32_t len, uint32_t timeout_ms);
int iotx_net_disconnect(utils_network_pt pNetwork);
int iotx_net_connect(utils_network_pt pNetwork);
int iotx_net_init(utils_network_pt pNetwork, iotx_mqtt_param_pt param);

#endif /* IOTX_COMMON_NET_H */
