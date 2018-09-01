
#ifndef _IOT_MQTT_CLIENT_H_
#define _IOT_MQTT_CLIENT_H_
#if defined(__cplusplus)
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "MQTT_export.h"
#include "utils_timer.h"
#include "utils_net.h"
#include "MQTTConnect.h"
#include "MQTTPacket.h"
#include "MQTTFormat.h"
#include "MQTTPublish.h"
#include "MQTTSubscribe.h"
#include "MQTTUnsubscribe.h"

/* maximum number of successful subscribe */
#define IOTX_MC_SUB_NUM_MAX                     (10)

/* maximum republish elements in list */
#define IOTX_MC_REPUB_NUM_MAX                   (20)

/* MQTT client version number */
#define IOTX_MC_MQTT_VERSION                    (4)

/* maximum length of topic name in byte */
#define IOTX_MC_TOPIC_NAME_MAX_LEN              (64)

/* maximum MQTT packet-id */
#define IOTX_MC_PACKET_ID_MAX                   (65535)

/* maximum number of simultaneously invoke subscribe request */
#define IOTX_MC_SUB_REQUEST_NUM_MAX             (10)

/* Minimum interval of MQTT reconnect in millisecond */
#define IOTX_MC_RECONNECT_INTERVAL_MIN_MS       (1000)

/* Maximum interval of MQTT reconnect in millisecond */
#define IOTX_MC_RECONNECT_INTERVAL_MAX_MS       (60000)

/* Minimum timeout interval of MQTT request in millisecond */
#define IOTX_MC_REQUEST_TIMEOUT_MIN_MS          (500)

/* Maximum timeout interval of MQTT request in millisecond */
#define IOTX_MC_REQUEST_TIMEOUT_MAX_MS          (5000)

/* Default timeout interval of MQTT request in millisecond */
#define IOTX_MC_REQUEST_TIMEOUT_DEFAULT_MS      (2000)

/* Max times of keepalive which has been send and did not received response package */
#define IOTX_MC_KEEPALIVE_PROBE_MAX             (3)

typedef enum IOT_RETURN_CODES {
    ERROR_DEVICE_NOT_EXSIT = -311,
    ERROR_NET_TIMEOUT = -310,
    ERROR_CERT_VERIFY_FAIL  = -309,
    ERROR_NET_SETOPT_TIMEOUT = -308,
    ERROR_NET_SOCKET = -307,
    ERROR_NET_CONNECT = -306,
    ERROR_NET_BIND = -305,
    ERROR_NET_LISTEN = -304,
    ERROR_NET_RECV = -303,
    ERROR_NET_SEND = -302,
    ERROR_NET_CONN = -301,
    ERROR_NET_UNKNOWN_HOST = -300,

    MQTT_SUB_INFO_NOT_FOUND_ERROR = -43,
    MQTT_PUSH_TO_LIST_ERROR = -42,
    MQTT_TOPIC_FORMAT_ERROR = -41,
    NETWORK_RECONNECT_TIMED_OUT_ERROR = -40,/** Returned when the Network is disconnected and the reconnect attempt has timed out */
    MQTT_CONNACK_UNKNOWN_ERROR = -39,/** Connect request failed with the server returning an unknown error */
    MQTT_CONANCK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR = -38,/** Connect request failed with the server returning an unacceptable protocol version error */
    MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR = -37,/** Connect request failed with the server returning an identifier rejected error */
    MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR = -36,/** Connect request failed with the server returning an unavailable error */
    MQTT_CONNACK_BAD_USERDATA_ERROR = -35,/** Connect request failed with the server returning a bad userdata error */
    MQTT_CONNACK_NOT_AUTHORIZED_ERROR = -34,/** Connect request failed with the server failing to authenticate the request */
    MQTT_CONNECT_ERROR = -33,
    MQTT_CREATE_THREAD_ERROR = -32,
    MQTT_PING_PACKET_ERROR = -31,
    MQTT_CONNECT_PACKET_ERROR = -30,
    MQTT_CONNECT_ACK_PACKET_ERROR = -29,
    MQTT_NETWORK_CONNECT_ERROR = -28,
    MQTT_STATE_ERROR = -27,
    MQTT_SUBSCRIBE_PACKET_ERROR = -26,
    MQTT_SUBSCRIBE_ACK_PACKET_ERROR = -25,
    MQTT_SUBSCRIBE_ACK_FAILURE = -24,
    MQTT_SUBSCRIBE_QOS_ERROR = -23,
    MQTT_UNSUBSCRIBE_PACKET_ERROR = -22,
    MQTT_PUBLISH_PACKET_ERROR = -21,
    MQTT_PUBLISH_QOS_ERROR = -20,
    MQTT_PUBLISH_ACK_PACKET_ERROR = -19,
    MQTT_PUBLISH_COMP_PACKET_ERROR = -18,
    MQTT_PUBLISH_REC_PACKET_ERROR = -17,
    MQTT_PUBLISH_REL_PACKET_ERROR = -16,
    MQTT_UNSUBSCRIBE_ACK_PACKET_ERROR = -15,
    MQTT_NETWORK_ERROR = -14,
    MQTT_PUBLISH_ACK_TYPE_ERROR = -13,

    ERROR_SHADOW_NO_METHOD = -2008,
    ERROR_SHADOW_UNDEF_TYPE = -2007,
    ERROR_SHADOW_UPDATE_TIMEOUT = -2006,
    ERROR_SHADOW_UPDATE_NACK = -2005,         /**< Negative ACK  */
    ERROR_SHADOW_NO_ATTRIBUTE = -2004,
    ERROR_SHADOW_ATTR_NO_EXIST = -2003,       /**< NO such attribute */
    ERROR_SHADOW_ATTR_EXIST = -2002,          /**< attribute already exists   */
    ERROR_SHADOW_WAIT_LIST_OVERFLOW = -2001,
    ERROR_SHADOW_INVALID_STATE = -2000,

    ERROR_NO_MEM = -1016,
    ERROR_CERTIFICATE_EXPIRED = -1015,
    ERROR_MALLOC = -1014,
    ERROR_NO_ENOUGH_MEM = -1013,               /**< Writes more than size value. */

    ERROR_NO_SUPPORT = -12,
    ERROR_NO_PERSISTENCE = -11,
    ERROR_HTTP_BREAK = -10,
    ERROR_NULL_VALUE = -9,
    ERROR_HTTP_CONN = -8,                    /**< Connection failed. */
    ERROR_HTTP_PARSE = -7,                   /**< A URL parse error occurred. */
    ERROR_HTTP_UNRESOLVED_DNS = -6,           /**< Could not resolve the hostname. */
    ERROR_HTTP_PRTCL = -5,                   /**< A protocol error occurred. */
    ERROR_HTTP = -4,                         /**< An unknown error occurred. */
    ERROR_HTTP_CLOSED = -3,                  /**< Connection was closed by a remote host. */
    NULL_VALUE_ERROR = -2,

    FAIL_RETURN = -1,                        /**< generic error. */
    SUCCESS_RETURN = 0,


    /* @value > 0, reserved for other usage */

} iotx_err_t;


typedef enum {
    IOTX_MC_CONNECTION_ACCEPTED = 0,
    IOTX_MC_CONNECTION_REFUSED_UNACCEPTABLE_PROTOCOL_VERSION = 1,
    IOTX_MC_CONNECTION_REFUSED_IDENTIFIER_REJECTED = 2,
    IOTX_MC_CONNECTION_REFUSED_SERVER_UNAVAILABLE = 3,
    IOTX_MC_CONNECTION_REFUSED_BAD_USERDATA = 4,
    IOTX_MC_CONNECTION_REFUSED_NOT_AUTHORIZED = 5
} iotx_mc_connect_ack_code_t;


/* State of MQTT client */
typedef enum {
    IOTX_MC_STATE_INVALID = 0,                    /* MQTT in invalid state */
    IOTX_MC_STATE_INITIALIZED = 1,                /* MQTT in initializing state */
    IOTX_MC_STATE_CONNECTED = 2,                  /* MQTT in connected state */
    IOTX_MC_STATE_DISCONNECTED = 3,               /* MQTT in disconnected state */
    IOTX_MC_STATE_DISCONNECTED_RECONNECTING = 4,  /* MQTT in reconnecting state */
} iotx_mc_state_t;


typedef enum MQTT_NODE_STATE {
    IOTX_MC_NODE_STATE_NORMANL = 0,
    IOTX_MC_NODE_STATE_INVALID,
} iotx_mc_node_t;


/* Handle structure of subscribed topic */
typedef struct {
    char topic_filter[64];
    iotx_mqtt_event_handle_t handle;
} iotx_mc_topic_handle_t;


/* Information structure of subscribed topic */
typedef struct SUBSCRIBE_INFO {
    enum msgTypes           type;           /* type, (sub or unsub) */
    uint16_t                msg_id;         /* packet id of subscribe(unsubcribe) */
    iotx_time_t             sub_start_time; /* start time of subscribe request */
    iotx_mc_node_t          node_state;     /* state of this node */
    iotx_mc_topic_handle_t  handler;        /* handle of topic subscribed(unsubcribed) */
    uint16_t                len;            /* length of subscribe message */
    unsigned char           buf[64];            /* subscribe message */
} iotx_mc_subsribe_info_t, *iotx_mc_subsribe_info_pt;


/* Information structure of published topic */
typedef struct REPUBLISH_INFO {
    iotx_time_t             pub_start_time;     /* start time of publish request */
    iotx_mc_node_t          node_state;         /* state of this node */
    uint16_t                msg_id;             /* packet id of publish */
    uint32_t                len;                /* length of publish message */
    unsigned char           buf[256];                /* publish message */
} iotx_mc_pub_info_t, *iotx_mc_pub_info_pt;


/* Reconnected parameter of MQTT client */
typedef struct {
    iotx_time_t         reconnect_next_time;         /* the next time point of reconnect */
    uint32_t            reconnect_time_interval_ms;  /* time interval of this reconnect */
} iotx_mc_reconnect_param_t;

/* structure of MQTT client */
typedef struct Client {
    void                           *lock_generic;                            /* generic lock */
    uint32_t                        packet_id;                               /* packet id */
    uint32_t                        request_timeout_ms;                      /* request timeout in millisecond */
    uint32_t                        buf_size_send;                           /* send buffer size in byte */
    uint32_t                        buf_size_read;                           /* read buffer size in byte */
    uint8_t                         keepalive_probes;                        /* keepalive probes */
    char                           *buf_send;                                /* pointer of send buffer */
    char                           *buf_read;                                /* pointer of read buffer */
    iotx_mc_topic_handle_t          sub_handle[IOTX_MC_SUB_REQUEST_NUM_MAX];         /* array of subscribe handle */
    iotx_mc_subsribe_info_t         sub_to_ack[IOTX_MC_SUB_REQUEST_NUM_MAX];
    iotx_mc_pub_info_t              pub_to_ack[IOTX_MC_REPUB_NUM_MAX];
    uint32_t sub_to_ack_size;
    uint32_t pub_to_ack_size;
    utils_network_t                 ipstack;                                 /* network parameter */
    iotx_time_t                     next_ping_time;                          /* next ping time */
    int                             ping_mark;                               /* flag of ping */
    iotx_mc_state_t                 client_state;                            /* state of MQTT client */
    iotx_mc_reconnect_param_t       reconnect_param;                         /* reconnect parameter */
    MQTTPacket_connectData          connect_data;                            /* connection parameter */    
    //void                           *lock_write_buf;                          /* lock of write */
    iotx_mqtt_event_handle_t        handle_event;                            /* event handle */
    
} iotx_mc_client_t, *iotx_mc_client_pt;

typedef enum {
    TOPIC_NAME_TYPE = 0,
    TOPIC_FILTER_TYPE
} iotx_mc_topic_type_t;


#if defined(__cplusplus)
}
#endif
#endif  /* #ifndef _IOT_MQTT_CLIENT_H_ */
