#ifndef CONFIG_DATA_H
#define CONFIG_DATA_H

#include <stdint.h>
#include <stdbool.h>

#define BAUD_RATE 115200
#define BUF_SIZE 1024

/* WiFi AP */
#define AP_SSID "JAQUAR_REMOTE"
#define AP_PASS "Jaquar@remote"
// #define WIFI_SSID "REMOTE_CONFIG"
// #define WIFI_PASS "config@123"
#define TCP_PORT 5000
/* UART */
#define UART_PORT       UART_NUM_1
#define UART_TX   17
#define UART_RX   18

#define BUTTON_GPIO     GPIO_NUM_1
#define ZWAVE_LED       GPIO_NUM_15
#define WIFI_LED        GPIO_NUM_16

#define TURN_ON_ZWAVE_LED   gpio_set_level(ZWAVE_LED, 0);
#define TURN_OFF_ZWAVE_LED  gpio_set_level(ZWAVE_LED, 1);

#define TURN_ON_WIFI_LED    gpio_set_level(WIFI_LED, 0);
#define TURN_OFF_WIFI_LED   gpio_set_level(WIFI_LED, 1);

#define MAX_NODES       232
#define MAX_NIF_LEN     64

#define INC_EXC_TIMEOUT 60
/* Z-Wave */
#define SOF 0x01
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

#define REQUEST  0x00
#define RESPONSE 0x01
// Function IDs
#define FUNC_ID_ZW_SEND_DATA                0x13
#define ZW_SET_DEFAULT                      0x42
#define FUNC_ID_SERIALAPI_STARTED           0x0A
#define FUNC_ID_SERIALAPI_SETUP             0x0B
#define FUNC_ID_MEMORY_GET_ID               0x20
#define FUNC_ID_ZW_ADD_NODE_TO_NETWORK      0x4A
#define FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK 0x4B
#define FUNC_ID_ZW_REQUEST_NODE_INFO        0x60
#define FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO   0x41
#define FUNC_ID_APPLICATION_UPDATE          0x49
#define FUNC_ID_ZW_IS_PRIMARY_CTRL          0x66
#define FUNC_ID_ZW_SET_DEFAULT              0x42
#define FUNC_ID_APPLICATION_COMMAND_HANDLER 0xA8

// RF Region Commands
#define RF_REGION_SET       0x40
#define RF_REGION_GET       0x20

// RF Region Values
#define RF_REGION_EUROPE        0x00
#define RF_REGION_USA           0x01
#define RF_REGION_AU_NZ         0x02
#define RF_REGION_HK            0x03
#define RF_REGION_INDIA         0x05
#define RF_REGION_ISRAEL        0x06
#define RF_REGION_RUSSIA        0x07
#define RF_REGION_CHINA         0x08
#define RF_REGION_JAPAN         0x09
#define RF_REGION_KOREA         0x0A
#define FUNC_ID_ZW_GET_CAPABILITIES     0x07
#define FUNC_ID_ZW_GET_SUC_NODE_ID      0x02
#define FUNC_ID_ZW_GET_VERSION          0x15
#define FUNC_ID_SOFT_RESET              0x08
#define FUNC_ID_ZW_SET_TX_POWER         0x04  // Sub-command of FUNC_ID_SERIALAPI_SETUP

// TX Power sub-commands
#define TX_POWER_GET       0x03
#define TX_POWER_SET       0x04

#define TX_OPTIONS       0x25
// CORRECTED ADD_NODE status values based on actual Z-Wave documentation
#define ADD_NODE_STATUS_LEARN_READY          0x01
#define ADD_NODE_STATUS_NODE_FOUND           0x02
#define ADD_NODE_STATUS_ADDING_SLAVE         0x03
#define ADD_NODE_STATUS_ADDING_CONTROLLER    0x04
#define ADD_NODE_STATUS_PROTOCOL_DONE        0x05
#define ADD_NODE_STATUS_DONE                 0x06
#define ADD_NODE_STATUS_FAILED               0x07
#define ADD_NODE_STATUS_NOT_PRIMARY          0x23

#define ADD_NODE_ANY                        0x01
#define ADD_NODE_CONTROLLER                 0x02
#define ADD_NODE_SLAVE                      0x03
#define ADD_NODE_EXISTING                   0x04
#define ADD_NODE_STOP                       0x05

// REMOVE_NODE status values
#define REMOVE_NODE_STATUS_LEARN_READY          0x01
#define REMOVE_NODE_STATUS_NODE_FOUND           0x02
#define REMOVE_NODE_STATUS_REMOVING_SLAVE       0x03
#define REMOVE_NODE_STATUS_REMOVING_CONTROLLER  0x04
#define REMOVE_NODE_STATUS_PROTOCOL_DONE        0x05
#define REMOVE_NODE_STATUS_DONE                 0x06
#define REMOVE_NODE_STATUS_FAILED               0x07

//Command Classes
#define COMMAND_CLASS_MANUFACTURER_SPECIFIC     0x72
#define CC_MANUFACTURER_SPECIFIC_REPORT         0x05
#define COMMAND_CLASS_CENTRAL_SCENE             0x5B
#define COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION 0x8E
#define MULTI_CHANNEL_ASSOCIATION_SET           0x01
#define MULTI_CHANNEL_ASSOCIATION_REPORT        0x03
#define COMMAND_CLASS_CONFIGURATION             0x70
#define CONFIGURATION_SET                       0x04
#define CONFIGURATION_GET                       0x05
#define CONFIGURATION_REPORT                    0x06
#define MULTI_CHANNEL_ASSOCIATION_GET           0x02
#define MULTI_CHANNEL_ASSOCIATION_REMOVE        0x04
#define MULTI_CHANNEL_ASSOCIATION_SET_MARKER    0x00
#define COMMAND_CLASS_MULTI_CHANNEL             0x60
#define LIFELINE_GROUP_1                        0x01
#define INCLUSION_TIMEOUT_MS   60000
#define EXCLUSION_TIMEOUT_MS   60000

typedef enum {
    INT_EVT_START,
    INT_EVT_MANUFACTURER_DONE,
    INT_EVT_MC_ENDPOINT_DONE,
    INT_EVT_MC_CAPABILITY_DONE,
    INT_EVT_CENTRAL_SCENE_DONE,
    INT_EVT_TIMEOUT
} interview_event_t;

typedef struct {
    uint8_t node_id;
    interview_event_t evt;
    uint8_t endpoint;   // used for MC capability
} interview_msg_t;

static QueueHandle_t interview_queue;
// ==================== INTERVIEW STATE MACHINE ====================

typedef enum {
    INT_STATE_IDLE,
    INT_STATE_MANUFACTURER,
    INT_STATE_MC_ENDPOINT,
    INT_STATE_MC_CAPABILITY,
    INT_STATE_CENTRAL_SCENE,
    INT_STATE_DONE
} interview_state_t;

typedef enum {
    DEV_UNKNOWN = 0,
    DEV_SWITCH_BINARY,
    DEV_REMOTE
} device_type_t;
/* Node information structure */
typedef struct {
    uint8_t node_id;
    //uint8_t nif[29];
    uint8_t nif_len;
    uint8_t capabilities;
    uint8_t device_type[3];
    bool interviewed;
    bool interview_in_progress;
   // uint8_t  node_id;
    uint8_t  nif[64];
    //uint8_t  nif_len;

    uint8_t  basic;
    uint8_t  generic;
    uint8_t  specific;

    device_type_t dev_type;

    uint8_t  endpoint_count;
    interview_state_t interview_state;
    uint8_t current_ep;    
    bool     interview_done; 
    /* Optional */
    uint16_t manufacturer_id;
    uint16_t product_type;
    uint16_t product_id;
    uint8_t scene_count; 
    uint8_t scene_flags;      
} node_info_t;

/* ================= STATE ================= */
typedef enum {
    ZW_IDLE = 0,
    ZW_WAIT_RF_SET,
    ZW_WAIT_HOMEID,
    ZW_WAIT_INCLUSION,
    ZW_WAIT_EXCLUSION
} zw_state_t;

typedef struct {
    uint8_t node_id;
    interview_state_t state;
    uint8_t endpoint_count;
    uint8_t current_endpoint;
    uint32_t timeout;
    bool has_multi_channel;
    bool manufacturer_info_received;
    bool protocol_info_received;
    bool version_info_received;
    uint8_t retry_count;  // ADD THIS LINE
} interview_context_t;

interview_context_t g_interview_ctx = {0};

// Store device capabilities
typedef struct {
    uint8_t node_id;
    uint8_t listening;
    uint8_t routing;
    uint8_t security;
    uint8_t version_major;
    uint8_t version_minor;
    
    // Device class fields:
    uint8_t basic_device_class;
    uint8_t generic_device_class;
    uint8_t specific_device_class;
    
    // Z-Wave Plus info
    uint8_t zwave_plus_version;
    uint8_t role_type;
    uint8_t node_type;
    uint16_t installer_icon;
    uint16_t user_icon;
    
    uint16_t manufacturer_id;
    uint16_t product_type_id;
    uint16_t product_id;
    uint8_t library_type;
    uint8_t protocol_version;
    uint8_t app_version;
    uint8_t endpoint_count;
    uint8_t supported_groupings;
    
    // Wake up info
    uint32_t min_wakeup;
    uint32_t max_wakeup;
    uint32_t default_wakeup;
    uint8_t wakeup_interval_set;
    
    // Battery info
    uint8_t battery_support;
    uint8_t battery_level;
    
    // Sensors and alarms
    uint8_t sensor_types[32];
    uint8_t sensor_count;
    uint8_t alarm_types[32];
    uint8_t alarm_count;
    
    // Command classes
    uint8_t cmd_classes[64];
    uint8_t cmd_class_count;
    
    // Interview status
    uint8_t interview_complete;
    uint32_t last_query_time;
    uint8_t retry_count;
} device_capabilities_t;

device_capabilities_t g_device_caps = {0};

/* Helper macros */
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

/* Function prototypes */
void kick_watchdog(void);
static void init_uart(void);
static void send_rf_region_india(void);
static void send_memory_get_id(void);
void zw_start_inclusion(void);
static void zw_stop_inclusion(void);
//static void zw_start_exclusion(void);
static void zw_stop_exclusion(void);
static void zw_stop_exclusion(void);
//static void zw_get_node_protocol_info(uint8_t nodeId);
//static void zw_request_node_info(uint8_t nodeId);
static void print_node_info(uint8_t node_id, uint8_t *nif_data, uint8_t nif_len);
void zw_send_manufacturer_get(uint8_t node_id);
void zw_send_mc_endpoint_get(uint8_t node_id);
void zw_send_mc_capability_get(uint8_t node_id, uint8_t ep);
void zw_send_central_scene_get(uint8_t node_id);
void stop_configuration_server(void);
void start_mdns_service(void);
httpd_handle_t start_webserver(void);
void wifi_init_softap(void);
void start_configuration_server(void);
static void print_node_table(void);
void send_msg_to_web(const char *msg);
static void zw_setup_central_scene_association(uint8_t node_id);
void zw_send_mc_association_set(uint8_t node_id);
#endif // CONFIG_DATA_H
