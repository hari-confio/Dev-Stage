#include <stdint.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_http_server.h"
#include "index_html.h"
#include "nvs.h"
#include "config_data.h"
#include <ctype.h>
#include <inttypes.h>
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "esp_netif.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "mdns.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "cJSON.h"

static const char *TAG = "MAIN";
static const char *TAG_ASSOCIATION = "ASSOCIATION";
static const char *TAG_PARAMETER = "PARAMETER";
static const char *TAG_WIFI = "WIFI";
static const char *TAG_WEB = "WEB_MSG";
static httpd_handle_t server = NULL;
static bool system_healthy = false;
static TimerHandle_t task_watchdog_timer = NULL;
static const char *TAGBUTTON = "BUTTON";
static QueueHandle_t gpio_evt_queue = NULL;
static uint32_t button_press_count = 0;
static uint32_t last_press_time = 0;
static const uint32_t MULTI_PRESS_TIMEOUT_MS = 1000; // 1 second timeout for multi-press detection
static const uint8_t MULTI_PRESS_THRESHOLD = 5; // Threshold for multi-press detection
static esp_timer_handle_t web_msg_timer;
//static bool wifi_initialized = false;
static uint8_t pending_node_id = 0;
static uint8_t pending_nif[64];
static uint8_t pending_nif_len = 0;
static bool exclusion_in_progress = false;
// WiFi configuration variables
static bool wifi_connected = false;
uint8_t web_dev_type = 0;
// WiFi connection retry variables
static int wifi_connect_retries = 0;
static const int MAX_WIFI_RETRIES = 5;           // 5 retries
static const int WIFI_RETRY_INTERVAL_MS = 4000;  // 4 seconds between retries
static TimerHandle_t wifi_retry_timer = NULL;
// static char wifi_ssid[32] = {0};
// static char wifi_password[64] = {0};
//static esp_netif_t *sta_netif = NULL;
static httpd_handle_t config_server = NULL;
static char web_status[256] = "Connecting...";
static SemaphoreHandle_t web_status_mutex = NULL;
TaskHandle_t blink_zwave_led_handle;
// Z-Wave state
zw_state_t zw_state = ZW_IDLE;
static uint8_t calculate_checksum(uint8_t *data, int len);
static void debug_print_frame(uint8_t *data, int len, const char *label);
// Node management
node_info_t g_included_nodes[232] = {0};
uint8_t g_node_count = 0;
uint8_t g_home_id[4] = {0};
bool g_home_id_valid = false;
static esp_netif_t *ap_netif  = NULL;
static esp_netif_t *sta_netif = NULL;
static bool wifi_driver_initialized = false;
uint8_t powerKeypad_nodeId = 0;
uint8_t remote_nodeId = 0;
// Inclusion control
static bool inclusion_completed = false;
static bool inclusion_protocol_done = false;
static uint32_t protocol_done_timer = 0;
static bool stop_sent_after_protocol = false;
bool add_node_found = false;
bool remove_node_found = false;
bool manufacturer_done;
bool central_scene_done;
bool show_Online_msg = true;
// ==================== WATCHDOG FUNCTIONS ====================
void kick_watchdog() {
    system_healthy = true;
}

void system_watchdog_callback(TimerHandle_t xTimer) {
    if (!system_healthy) {
        ESP_LOGE(TAG, "Watchdog timeout - System appears stuck, restarting...");
        esp_restart();
    }
    system_healthy = false;
}

// ==================== UART INITIALIZATION ====================
static void init_uart() {
    uart_config_t cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    // Install with larger buffers
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, 4096, 4096, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX, UART_RX,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    // Set timeout for read
    // uart_set_rx_timeout(UART_PORT, 10); // 10 character times

}
static void wifi_driver_init_once(void)
{
    if (wifi_driver_initialized) return;

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_driver_initialized = true;
}

// ==================== WiFi CONFIGURATION HANDLERS ====================
void url_decode(char *dst, const char *src, size_t dst_size) {
    char a, b;
    size_t i = 0;
    size_t j = 0;
    
    while (src[i] != '\0' && j < dst_size - 1) {
        if (src[i] == '%') {
            if (src[i+1] != '\0' && src[i+2] != '\0') {
                a = src[i+1];
                b = src[i+2];
                
                if (isxdigit(a) && isxdigit(b)) {
                    if (a >= '0' && a <= '9') a -= '0';
                    else if (a >= 'a' && a <= 'f') a = a - 'a' + 10;
                    else if (a >= 'A' && a <= 'F') a = a - 'A' + 10;
                    
                    if (b >= '0' && b <= '9') b -= '0';
                    else if (b >= 'a' && b <= 'f') b = b - 'a' + 10;
                    else if (b >= 'A' && b <= 'F') b = b - 'A' + 10;
                    
                    dst[j++] = (a << 4) | b;
                    i += 3;
                } else {
                    dst[j++] = src[i++];
                }
            } else {
                dst[j++] = src[i++];
            }
        } else if (src[i] == '+') {
            dst[j++] = ' ';
            i++;
        } else {
            dst[j++] = src[i++];
        }
    }
    dst[j] = '\0';
}
// HTML page for WiFi configuration

static const char* wifi_config_html = 
"<html>"
"<head><title>Jaquar Remote WiFi Configuration</title>"
"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
"<style>"
"body { font-family: Arial; margin: 40px; }"
".container { max-width: 400px; margin: 0 auto; }"
".form-group { margin-bottom: 20px; }"
"label { display: block; margin-bottom: 5px; font-weight: bold; }"
"input[type='text'], input[type='password'] { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 4px; }"
"button { background: #007bff; color: white; border: none; padding: 12px 20px; border-radius: 4px; cursor: pointer; width: 100%; }"
"button:hover { background: #0056b3; }"
".status { padding: 10px; border-radius: 4px; margin-top: 20px; }"
".success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }"
".error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }"
".checkbox-group { margin: 10px 0; }"
".checkbox-group label { display: inline-block; margin-left: 8px; font-weight: normal; }"
"</style>"
"</head>"
"<body>"
"<div class='container'>"
"<h2>Jaquar Remote WiFi Configuration</h2>"
"<form method='post' action='/wifi-config' enctype='application/x-www-form-urlencoded'>"
"<div class='form-group'>"
"<label for='ssid'>SSID:</label>"
"<input type='text' id='ssid' name='ssid' required>"
"</div>"
"<div class='form-group'>"
"<label for='password'>Password:</label>"
"<input type='password' id='password' name='password'>"
"</div>"
"<div class='checkbox-group'>"
"<input type='checkbox' id='showPassword' onchange='togglePassword()'>"
"<label for='showPassword'>Show Password</label>"
"</div>"
"<button type='submit'>Connect</button>"
"</form>"
"<div id='status'></div>"
"</div>"
"<script>"
"function togglePassword() {"
"    var passwordField = document.getElementById('password');"
"    var showPassword = document.getElementById('showPassword');"
"    if (showPassword.checked) {"
"        passwordField.type = 'text';"
"    } else {"
"        passwordField.type = 'password';"
"    }"
"}"
"document.querySelector('form').addEventListener('submit', function(e) {"
"    e.preventDefault();"
"    var formData = new URLSearchParams(new FormData(this));"
"    fetch('/wifi-config', {"
"        method: 'POST',"
"        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },"
"        body: formData"
"    }).then(response => response.text())"
"    .then(data => {"
"        var status = document.getElementById('status');"
"        if(data.includes('Success')) {"
"            status.innerHTML = '<div class=\"status success\">' + data + '</div>';"
"            setTimeout(function() { window.location.reload(); }, 3000);"
"        } else {"
"            status.innerHTML = '<div class=\"status error\">' + data + '</div>';"
"        }"
"    });"
"});"
"</script>"
"</body>"
"</html>";

// Function to check if character is whitespace (safe version)
static bool is_whitespace(char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

// Function to trim trailing whitespace from a string
static void trim_trailing_whitespace(char* str) {
    if (str == NULL) return;
    
    size_t len = strlen(str);
    while (len > 0 && is_whitespace(str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }
}

void clear_wifi_credentials_and_restart(void) {
    ESP_LOGI(TAG_WIFI, "Max WiFi retries reached (%d attempts), clearing credentials and restarting in AP mode", MAX_WIFI_RETRIES);
    
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("wifi_config", NVS_READWRITE, &nvs);
    if (err == ESP_OK) {
        nvs_erase_key(nvs, "ssid");
        nvs_erase_key(nvs, "password");
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGI(TAG_WIFI, "WiFi credentials cleared from NVS");
    }
    
    // Stop retry timer if running
    if (wifi_retry_timer != NULL) {
        xTimerStop(wifi_retry_timer, 0);
        xTimerDelete(wifi_retry_timer, 0);
        wifi_retry_timer = NULL;
    }
    
    // Restart to go into AP mode
    esp_restart();
}

// WiFi retry timer callback
void wifi_retry_callback(TimerHandle_t xTimer) {
    wifi_connect_retries++;
    ESP_LOGI(TAG_WIFI, "WiFi connection retry %d/%d", wifi_connect_retries, MAX_WIFI_RETRIES);
    
    if (wifi_connect_retries >= MAX_WIFI_RETRIES) {
        clear_wifi_credentials_and_restart();
    } else {
        ESP_LOGI(TAG_WIFI, "Attempting to reconnect to WiFi...");
        esp_wifi_connect();
    }
}


// WiFi configuration page handler
esp_err_t wifi_config_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, wifi_config_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}
// Function to parse multipart form data
bool parse_multipart_form_data(char* data, size_t data_len, char* ssid, size_t ssid_size, char* password, size_t password_size) {
    char* ptr = data;
    
    // Find SSID field
    char* ssid_start = strstr(ptr, "name=\"ssid\"");
    if (ssid_start) {
        ssid_start = strstr(ssid_start, "\r\n\r\n");
        if (ssid_start) {
            ssid_start += 4; // Skip \r\n\r\n
            char* ssid_end = strstr(ssid_start, "\r\n");
            if (ssid_end) {
                size_t ssid_len = ssid_end - ssid_start;
                if (ssid_len > 0 && ssid_len < ssid_size) {
                    strncpy(ssid, ssid_start, ssid_len);
                    ssid[ssid_len] = '\0';
                    // Trim trailing whitespace
                    trim_trailing_whitespace(ssid);
                }
            }
        }
    }
    
    // Find password field
    char* password_start = strstr(ptr, "name=\"password\"");
    if (password_start) {
        password_start = strstr(password_start, "\r\n\r\n");
        if (password_start) {
            password_start += 4; // Skip \r\n\r\n
            char* password_end = strstr(password_start, "\r\n");
            if (password_end) {
                size_t password_len = password_end - password_start;
                if (password_len > 0 && password_len < password_size) {
                    strncpy(password, password_start, password_len);
                    password[password_len] = '\0';
                    // Trim trailing whitespace
                    trim_trailing_whitespace(password);
                }
            }
        }
    }
    
    return (strlen(ssid) > 0);
}

// WiFi configuration POST handler (updated to handle both content types)
esp_err_t wifi_config_post_handler(httpd_req_t *req) {
    char content[512]; // Increased buffer size
    int ret, remaining = req->content_len;
    int total_read = 0;
    
    // Check content length
    if (remaining > sizeof(content) - 1) {
        httpd_resp_send(req, "Error: Content too large", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // Read POST data
    while (remaining > 0) {
        ret = httpd_req_recv(req, content + total_read, remaining);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
            httpd_resp_send(req, "Error: Failed to read data", HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }
        total_read += ret;
        remaining -= ret;
    }
    content[total_read] = '\0';
    
    ESP_LOGI(TAG_WIFI, "Received form data (%d bytes)", total_read);
    
    char ssid[32] = {0};
    char password[64] = {0};
    bool parse_success = false;
    
    // Check content type
    char content_type[64] = {0};
    if (httpd_req_get_hdr_value_str(req, "Content-Type", content_type, sizeof(content_type)) == ESP_OK) {
        ESP_LOGI(TAG_WIFI, "Content-Type: %s", content_type);
        
        if (strstr(content_type, "multipart/form-data") != NULL) {
            // Parse multipart form data
            parse_success = parse_multipart_form_data(content, total_read, ssid, sizeof(ssid), password, sizeof(password));
            ESP_LOGI(TAG_WIFI, "Parsed multipart - SSID: '%s', Password: '%s'", ssid, password);
        } else if (strstr(content_type, "application/x-www-form-urlencoded") != NULL) {
            // Parse URL encoded form data
            char *token = strtok(content, "&");
            while (token != NULL) {
                char *key = token;
                char *value = strchr(token, '=');
                if (value) {
                    *value = '\0';
                    value++;
                    
                    // URL decode the value
                    char decoded[64];
                    url_decode(decoded, value, sizeof(decoded));
                    
                    if (strcmp(key, "ssid") == 0) {
                        strncpy(ssid, decoded, sizeof(ssid) - 1);
                    } else if (strcmp(key, "password") == 0) {
                        strncpy(password, decoded, sizeof(password) - 1);
                    }
                }
                token = strtok(NULL, "&");
            }
            parse_success = (strlen(ssid) > 0);
            ESP_LOGI(TAG_WIFI, "Parsed urlencoded - SSID: '%s', Password: '%s'", ssid, 
                     strlen(password) > 0 ? "***" : "<empty>");
        }
    }
    
    if (!parse_success) {
        // Try to parse anyway (fallback)
        ESP_LOGW(TAG_WIFI, "Unknown content type, trying fallback parsing");
        parse_multipart_form_data(content, total_read, ssid, sizeof(ssid), password, sizeof(password));
        parse_success = (strlen(ssid) > 0);
    }
    
    // Validate SSID
    if (!parse_success || strlen(ssid) == 0) {
        httpd_resp_send(req, "Error: SSID cannot be empty or parsing failed", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    ESP_LOGI(TAG_WIFI, "Final WiFi config - SSID: '%s', Password: '%s'", ssid, 
             strlen(password) > 0 ? "***" : "<empty>");
    
    // Save to NVS
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("wifi_config", NVS_READWRITE, &nvs);
    if (err == ESP_OK) {
        nvs_set_str(nvs, "ssid", ssid);
        nvs_set_str(nvs, "password", password);
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGI(TAG_WIFI, "WiFi credentials saved to NVS");
        
        httpd_resp_send(req, "Success! Connecting to WiFi... The device will restart.", HTTPD_RESP_USE_STRLEN);
        TURN_OFF_WIFI_LED
        // Delay to allow response to be sent
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        // Restart to apply new WiFi configuration
        esp_restart();
    } else {
        httpd_resp_send(req, "Error: Failed to save configuration", HTTPD_RESP_USE_STRLEN);
    }
    
    return ESP_OK;
}
static void wifi_event_handler(void* arg, esp_event_base_t event_base, 
                             int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG_WIFI, "WiFi STA started, connecting...");
        
        // Start retry timer (6 seconds per retry)
        wifi_connect_retries = 0;
        if (wifi_retry_timer == NULL) {
            wifi_retry_timer = xTimerCreate("wifi_retry", pdMS_TO_TICKS(WIFI_RETRY_INTERVAL_MS), pdFALSE, NULL, wifi_retry_callback);
        }
        xTimerStart(wifi_retry_timer, 0);
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        TURN_OFF_WIFI_LED
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGI(TAG_WIFI, "WiFi disconnected, reason: %d. Retry %d/%d in %d seconds", 
                 event->reason, wifi_connect_retries + 1, MAX_WIFI_RETRIES, WIFI_RETRY_INTERVAL_MS / 1000);
        
        // Restart retry timer for next attempt
        if (wifi_retry_timer != NULL) {
            xTimerReset(wifi_retry_timer, 0);
        }
        wifi_connected = false;
        
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_WIFI, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
        
        // Stop retry timer on successful connection
        if (wifi_retry_timer != NULL) {
            xTimerStop(wifi_retry_timer, 0);
            xTimerDelete(wifi_retry_timer, 0);
            wifi_retry_timer = NULL;
        }
        wifi_connect_retries = 0;
        
        // Stop configuration server if running
        stop_configuration_server();
        
        // Start mDNS and MODBUS server once connected
        start_mdns_service();
        // Start web server
        ESP_LOGI(TAG, "Starting web server...");
        server = start_webserver();
        if (server == NULL) {
            ESP_LOGE(TAG, "Failed to start web server");
            return;
        }
        
        ESP_LOGI(TAG_WIFI, "WiFi connected successfully after %d seconds total", 
                 (wifi_connect_retries * WIFI_RETRY_INTERVAL_MS / 1000));
        TURN_ON_WIFI_LED
    }
}
void wifi_init_sta(void)
{
    ESP_LOGI(TAG_WIFI, "Initializing WiFi STA mode");

    wifi_driver_init_once();

    if (!sta_netif) {
        sta_netif = esp_netif_create_default_wifi_sta();
        assert(sta_netif);
    }

    wifi_config_t wifi_config = {0};

    // ---- LOAD credentials from NVS ----
    nvs_handle_t nvs;
    size_t ssid_len = sizeof(wifi_config.sta.ssid);
    size_t pass_len = sizeof(wifi_config.sta.password);

    if (nvs_open("wifi_config", NVS_READONLY, &nvs) == ESP_OK) {
        nvs_get_str(nvs, "ssid",
                    (char *)wifi_config.sta.ssid, &ssid_len);
        nvs_get_str(nvs, "password",
                    (char *)wifi_config.sta.password, &pass_len);
        nvs_close(nvs);
    }

    ESP_LOGI(TAG_WIFI, "Connecting to SSID: %s",
             (char *)wifi_config.sta.ssid);

    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}


// Check if we should use STA or AP mode
bool should_use_sta_mode(void) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("wifi_config", NVS_READONLY, &nvs);
    if (err != ESP_OK) {
        ESP_LOGI(TAG_WIFI, "No wifi_config in NVS, use AP mode");
        return false;
    }
    
    char ssid[32] = {0};
    size_t ssid_len = sizeof(ssid);
    err = nvs_get_str(nvs, "ssid", ssid, &ssid_len);
    nvs_close(nvs);
    
    if (err != ESP_OK || strlen(ssid) == 0) {
        ESP_LOGI(TAG_WIFI, "No valid SSID in NVS, use AP mode");
        return false;
    }
    
    ESP_LOGI(TAG_WIFI, "Valid SSID found in NVS, use STA mode");
    return true;
}
// Initialize WiFi in AP mode for configuration
void wifi_init_softap(void)
{
    ESP_LOGI(TAG_WIFI, "Starting configuration AP");

    wifi_driver_init_once();

    if (!ap_netif) {
        ap_netif = esp_netif_create_default_wifi_ap();
        assert(ap_netif);
    }

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = 1,
            .password = AP_PASS,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    if (strlen(AP_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    start_configuration_server();
}

void blink_wifi_led_task(void *pvParameter)
{
    while (1) {
        TURN_ON_WIFI_LED
        vTaskDelay(100 / portTICK_PERIOD_MS);
        TURN_OFF_WIFI_LED
        vTaskDelay(100 / portTICK_PERIOD_MS);
        TURN_ON_WIFI_LED
        vTaskDelay(100 / portTICK_PERIOD_MS);
        TURN_OFF_WIFI_LED
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

// Start configuration web server (AP mode)
void start_configuration_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    
    if (httpd_start(&config_server, &config) == ESP_OK) {
        httpd_uri_t wifi_config_get = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = wifi_config_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(config_server, &wifi_config_get);
        
        httpd_uri_t wifi_config_post = {
            .uri = "/wifi-config",
            .method = HTTP_POST,
            .handler = wifi_config_post_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(config_server, &wifi_config_post);
        
        ESP_LOGI(TAG_WIFI, "Configuration server started on AP");
        xTaskCreate(&blink_wifi_led_task, "Gateway is in AP mode", 2048, NULL, 5, NULL);
    }
}

// Stop configuration server
void stop_configuration_server(void) {
    if (config_server) {
        httpd_stop(config_server);
        config_server = NULL;
        ESP_LOGI(TAG_WIFI, "Configuration server stopped");
    }
}

// Initialize mDNS service
void start_mdns_service(void) {
    static bool mdns_started = false;
    if (!mdns_started) {
        ESP_ERROR_CHECK(mdns_init());
        ESP_ERROR_CHECK(mdns_hostname_set("jaquarremote"));
        ESP_ERROR_CHECK(mdns_instance_name_set("Confio Jaquar Remote"));
        ESP_ERROR_CHECK(mdns_service_add(NULL, "_jaquarremote", "_tcp", TCP_PORT, NULL, 0));
        ESP_LOGI(TAG_WIFI,"mDNS service started with hostname 'jaquarremote' on port %d\n", TCP_PORT);
        mdns_started = true;
    } else {
        ESP_LOGI(TAG_WIFI,"mDNS already started, skipping service add.\n");
    }
}
static const char *add_node_status_str(uint8_t status) {
    switch (status) {
        case ADD_NODE_STATUS_LEARN_READY:     return "LEARN_READY";
        case ADD_NODE_STATUS_NODE_FOUND:      return "NODE_FOUND";
        case ADD_NODE_STATUS_ADDING_SLAVE:    return "ADDING_SLAVE";
        case ADD_NODE_STATUS_ADDING_CONTROLLER:return "ADDING_CONTROLLER";
        case ADD_NODE_STATUS_PROTOCOL_DONE:   return "PROTOCOL_DONE";
        case ADD_NODE_STATUS_DONE:            return "DONE";
        case ADD_NODE_STATUS_FAILED:          return "FAILED";
        case ADD_NODE_STATUS_NOT_PRIMARY:     return "NOT_PRIMARY";
        default:                              return "UNKNOWN";
    }
}
static const char *basic_dev_str(uint8_t b) {
    switch (b) {
        case 0x01: return "Controller";
        case 0x02: return "Static Controller";
        case 0x03: return "Slave";
        case 0x04: return "Routing Slave";
        default:   return "Unknown";
    }
}
static const char *cc_str(uint8_t cc) {
    switch (cc) {
        case 0x25: return "Switch Binary";
        case 0x26: return "Switch Multilevel";
        case 0x70: return "Configuration";
        case 0x72: return "Manufacturer Specific";
        case 0x85: return "Association";
        case 0x8E: return "Multi Channel Association";
        case 0x5E: return "Z-Wave Plus Info";
        case 0x60: return "Multi Channel";
        case 0x98: return "Security";
        default:   return "Unknown CC";
    }
}

/* ================= TIMER CALLBACK ================= */
static void web_msg_timer_cb(void *arg)
{
    if(show_Online_msg){
        send_msg_to_web("Gateway Status : ONLINE");
    }
    //ESP_LOGW(TAG, "Web Msg elapsing since last msg update");
}

static void nvs_save_node_table(void)
{
    nvs_handle_t nvs;
    ESP_ERROR_CHECK(nvs_open("nodes", NVS_READWRITE, &nvs));

    ESP_ERROR_CHECK(nvs_set_u8(nvs, "count", g_node_count));

    for (int i = 0; i < g_node_count; i++) {
        char key[16];
        snprintf(key, sizeof(key), "node_%d", i);

        ESP_ERROR_CHECK(
            nvs_set_blob(nvs, key,
                         &g_included_nodes[i],
                         sizeof(node_info_t))
        );
    }

    ESP_ERROR_CHECK(nvs_commit(nvs));
    nvs_close(nvs);

    ESP_LOGI(TAG, "✓ Node table saved to NVS (%d nodes)", g_node_count);
    ESP_LOGI(TAG, "==============NVS Table Data=================");
    print_node_table();
}

static void nvs_load_node_table(void)
{
    nvs_handle_t nvs;
    uint8_t count = 0;

    if (nvs_open("nodes", NVS_READONLY, &nvs) != ESP_OK)
        return;

    if (nvs_get_u8(nvs, "count", &count) != ESP_OK) {
        nvs_close(nvs);
        return;
    }

    g_node_count = count;

    for (int i = 0; i < count; i++) {
        char key[16];
        size_t size = sizeof(node_info_t);

        snprintf(key, sizeof(key), "node_%d", i);
        nvs_get_blob(nvs, key, &g_included_nodes[i], &size);
    }

    nvs_close(nvs);

    ESP_LOGI(TAG, "✓ Loaded %d nodes from NVS", g_node_count);
}


static const char *dev_type_str(uint8_t t)
{
    switch (t) {
        case DEV_SWITCH_BINARY: return "SWITCH_BINARY";
        case DEV_REMOTE:        return "REMOTE";
        case DEV_UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

// ==================== RF REGION FUNCTION ====================

// Function to set RF region
static void zw_set_rf_region(uint8_t region) {
    uint8_t data[] = {0x05, 0x00, 0x0B, RF_REGION_SET, region};
    uint8_t chk = calculate_checksum(data, sizeof(data));
    uint8_t frame[] = {SOF, data[0], data[1], data[2], data[3], data[4], chk};
    
    debug_print_frame(frame, sizeof(frame), "TX RF Region Set");
    uart_write_bytes(UART_PORT, (char*)frame, sizeof(frame));
    
    ESP_LOGI(TAG, "Setting RF Region to: 0x%02X", region);
    
    // Print region name
    switch(region) {
        case RF_REGION_EUROPE: ESP_LOGI(TAG, "Region: Europe"); break;
        case RF_REGION_USA: ESP_LOGI(TAG, "Region: USA"); break;
        case RF_REGION_AU_NZ: ESP_LOGI(TAG, "Region: Australia/New Zealand"); break;
        case RF_REGION_HK: ESP_LOGI(TAG, "Region: Hong Kong"); break;
        case RF_REGION_INDIA: ESP_LOGI(TAG, "Region: India"); break;
        case RF_REGION_ISRAEL: ESP_LOGI(TAG, "Region: Israel"); break;
        case RF_REGION_RUSSIA: ESP_LOGI(TAG, "Region: Russia"); break;
        case RF_REGION_CHINA: ESP_LOGI(TAG, "Region: China"); break;
        case RF_REGION_JAPAN: ESP_LOGI(TAG, "Region: Japan"); break;
        case RF_REGION_KOREA: ESP_LOGI(TAG, "Region: Korea"); break;
        default: ESP_LOGI(TAG, "Region: Unknown (0x%02X)", region); break;
    }
}

// Function to get current RF region
static void zw_get_rf_region(void) {
    uint8_t data[] = {0x04, 0x00, 0x0B, RF_REGION_GET};
    uint8_t chk = calculate_checksum(data, sizeof(data));
    uint8_t frame[] = {SOF, data[0], data[1], data[2], data[3], chk};
    
    debug_print_frame(frame, sizeof(frame), "TX RF Region Get");
    uart_write_bytes(UART_PORT, (char*)frame, sizeof(frame));
    
    ESP_LOGI(TAG, "Requesting current RF Region");
}

// Function to print region info
static void print_region_info(uint8_t region) {
    ESP_LOGI(TAG, "=== RF REGION INFORMATION ===");
    ESP_LOGI(TAG, "Region Code: 0x%02X", region);
    
    const char* region_name = "Unknown";
    switch(region) {
        case RF_REGION_EUROPE: region_name = "Europe"; break;
        case RF_REGION_USA: region_name = "USA"; break;
        case RF_REGION_AU_NZ: region_name = "Australia/New Zealand"; break;
        case RF_REGION_HK: region_name = "Hong Kong"; break;
        case RF_REGION_INDIA: region_name = "India"; break;
        case RF_REGION_ISRAEL: region_name = "Israel"; break;
        case RF_REGION_RUSSIA: region_name = "Russia"; break;
        case RF_REGION_CHINA: region_name = "China"; break;
        case RF_REGION_JAPAN: region_name = "Japan"; break;
        case RF_REGION_KOREA: region_name = "Korea"; break;
        default: break;
    }
    
    ESP_LOGI(TAG, "Region Name: %s", region_name);
    ESP_LOGI(TAG, "============================");
}
// ==================== Z-WAVE HELPER FUNCTIONS ====================
static void send_ack(void) {
    uint8_t b = ACK;
    uart_write_bytes(UART_PORT, (char *)&b, 1);
}

static int find_sof(uint8_t *buf, int len) {
    for (int i = 0; i < len; i++) {
        if (buf[i] == SOF) return i;
    }
    return -1;
}

static uint8_t calculate_checksum(uint8_t *data, int len) {
    uint8_t chk = 0xFF;
    for (int i = 0; i < len; i++) {
        chk ^= data[i];
    }
    return chk;
}

// ==================== Z-WAVE COMMAND FUNCTIONS ====================
static void send_rf_region_india(void) {
    uint8_t frame[] = {0x01, 0x05, 0x00, 0x0B, 0x40, 0x05, 0xB4};
    uart_write_bytes(UART_PORT, (char*)frame, sizeof(frame));
    ESP_LOGI(TAG, "RF region set: INDIA");
}

static void send_memory_get_id(void) {
    uint8_t frame[] = {0x01, 0x03, 0x00, 0x20, 0xDC};
    uart_write_bytes(UART_PORT, (char*)frame, sizeof(frame));
}

static void debug_print_frame(uint8_t *data, int len, const char *label) {
    printf("%s: ", label);
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

static node_info_t *find_node(uint8_t node_id)
{
    for (int i = 0; i < g_node_count; i++) {
        if (g_included_nodes[i].node_id == node_id) {
            return &g_included_nodes[i];
        }
    }
    return NULL;
}

void blink_zwave_led(){
    TURN_OFF_ZWAVE_LED
    vTaskDelay(300 / portTICK_PERIOD_MS);
    TURN_ON_ZWAVE_LED
    vTaskDelay(300 / portTICK_PERIOD_MS);
}


// ==================== INCLUSION/EXCLUSION FUNCTIONS ====================
void zw_inclusion_task(void *arg)
{
    blink_zwave_led();
    if (zw_state != ZW_IDLE) {
        ESP_LOGW(TAG, "Cannot start inclusion, Z-Wave busy");
        //return;
    }

    /* Reset inclusion state */
    pending_node_id          = 0;
    pending_nif_len          = 0;
    inclusion_completed      = false;
    inclusion_protocol_done  = false;
    stop_sent_after_protocol = false;
    protocol_done_timer      = 0;

    ESP_LOGI(TAG, "========== STARTING INCLUSION PROCESS ==========");

    /* Ensure clean controller state */
    uint8_t stop_frame[] = {
        0x01, 0x04, 0x00, 0x4A, ADD_NODE_STOP, 0xB4
    };
    uart_write_bytes(UART_PORT, (char*)stop_frame, sizeof(stop_frame));
    vTaskDelay(pdMS_TO_TICKS(200));

    /* Start inclusion */
    uint8_t data[] = {0x04, 0x00, 0x4A, ADD_NODE_ANY};
    uint8_t chk = calculate_checksum(data, sizeof(data));
    uint8_t frame[] = {SOF, data[0], data[1], data[2], data[3], chk};

    uart_write_bytes(UART_PORT, (char*)frame, sizeof(frame));
    // add_node_found = false;
    zw_state = ZW_WAIT_INCLUSION;

    ESP_LOGI(TAG, "Z-Wave inclusion started");
    //bool add_node_found = false;
    int64_t start = esp_timer_get_time();
    int64_t timeout = INC_EXC_TIMEOUT * 1000000;
    while ((esp_timer_get_time() - start) < timeout) {
        if (add_node_found) {
            add_node_found = false;
            ESP_LOGI(TAG, "Done received from within 60 seconds");
            vTaskDelete(NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    ESP_LOGW(TAG, "Inclusion timeout");
    show_Online_msg = true;
    send_msg_to_web("Timeout! No device found.");
    zw_stop_inclusion();
    zw_state = ZW_IDLE;
    vTaskDelete(NULL);
}

void zw_start_inclusion_async(void)
{
    xTaskCreate(
        zw_inclusion_task,
        "zw_inclusion_task",
        4096,
        NULL,
        5,
        NULL
    );
}

void zw_exclusion_task(void *arg) {
    blink_zwave_led();
    if (zw_state != ZW_IDLE) {
        ESP_LOGW(TAG, "Cannot start exclusion, state is %d", zw_state);
        //return;
    }
    
    ESP_LOGI(TAG, "========== STARTING EXCLUSION PROCESS ==========");
    
    // if (g_node_count == 0) {
    //     ESP_LOGW(TAG, "No nodes to exclude!");
    //     return;
    // }

    // ESP_LOGI(TAG, "Current nodes to exclude: %d", g_node_count);
    // for (int i = 0; i < g_node_count; i++) {
    //     ESP_LOGI(TAG, "  Node %d", g_included_nodes[i].node_id);
    // }
    
    // Send stop command first to ensure clean state
    uint8_t stop_data[] = {0x04, 0x00, 0x4B, 0x05};
    uint8_t stop_chk = calculate_checksum(stop_data, sizeof(stop_data));
    uint8_t stop_frame[] = {SOF, stop_data[0], stop_data[1], stop_data[2], stop_data[3], stop_chk};
    
    debug_print_frame(stop_frame, sizeof(stop_frame), "TX Ensure Clean (Exclusion)");
    uart_write_bytes(UART_PORT, (char*)stop_frame, sizeof(stop_frame));
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // Start exclusion with proper checksum
    uint8_t data[] = {0x04, 0x00, 0x4B, 0x01};  // Length, Type, CMD, Mode
    uint8_t chk = calculate_checksum(data, sizeof(data));
    uint8_t frame[] = {SOF, data[0], data[1], data[2], data[3], chk};
    
    debug_print_frame(frame, sizeof(frame), "TX Exclusion Start");
    uart_write_bytes(UART_PORT, (char*)frame, sizeof(frame));
    exclusion_in_progress = true;
    zw_state = ZW_WAIT_EXCLUSION;
    ESP_LOGI(TAG, "Z-Wave exclusion started");
    int64_t start = esp_timer_get_time();
    int64_t timeout = INC_EXC_TIMEOUT * 1000000;
    while ((esp_timer_get_time() - start) < timeout) {
        if (remove_node_found) {
            remove_node_found = false;
            ESP_LOGI(TAG, "Done received from within 60 seconds");
            vTaskDelete(NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    ESP_LOGW(TAG, "Exclusion timeout");
    // Return immediate response
    show_Online_msg = true;
    send_msg_to_web("Timeout! No device found.");
    zw_stop_exclusion();
    zw_state = ZW_IDLE;
    vTaskDelete(NULL);    
}

void zw_start_exclusion_async(void)
{
    xTaskCreate(
        zw_exclusion_task,
        "zw_exclusion_task",
        4096,
        NULL,
        5,
        NULL
    );
}

// ==================== SERIAL API INITIALIZATION FUNCTIONS ====================

// Get Controller Capabilities
static void zw_get_capabilities(void) {
    uint8_t data[] = {0x03, REQUEST, FUNC_ID_ZW_GET_CAPABILITIES};
    uint8_t chk = calculate_checksum(data, sizeof(data));
    uint8_t frame[] = {SOF, data[0], data[1], data[2], chk};
    
    debug_print_frame(frame, sizeof(frame), "TX Get Capabilities");
    uart_write_bytes(UART_PORT, (char*)frame, sizeof(frame));
    
    ESP_LOGI(TAG, "Requesting controller capabilities...");
}

// Get SUC Node ID
static void zw_get_suc_node_id(void) {
    uint8_t data[] = {0x03, REQUEST, FUNC_ID_ZW_GET_SUC_NODE_ID};
    uint8_t chk = calculate_checksum(data, sizeof(data));
    uint8_t frame[] = {SOF, data[0], data[1], data[2], chk};
    
    debug_print_frame(frame, sizeof(frame), "TX Get SUC Node ID");
    uart_write_bytes(UART_PORT, (char*)frame, sizeof(frame));
    
    ESP_LOGI(TAG, "Requesting SUC (Static Update Controller) Node ID...");
}

// Get SerialAPI Supported Command Classes
static void zw_get_serialapi_supported(void) {
    uint8_t data[] = {0x04, REQUEST, FUNC_ID_SERIALAPI_SETUP, 0x01};  // Sub-command 0x01
    uint8_t chk = calculate_checksum(data, sizeof(data));
    uint8_t frame[] = {SOF, data[0], data[1], data[2], data[3], chk};
    
    debug_print_frame(frame, sizeof(frame), "TX SerialAPI Setup: Supported");
    uart_write_bytes(UART_PORT, (char*)frame, sizeof(frame));
    
    ESP_LOGI(TAG, "Requesting SerialAPI supported command classes...");
}


// Set TX Power Level (0x7F = +7dBm, 0x00 = 0dBm, etc.)
static void zw_set_tx_power(uint8_t power_level, uint8_t measured_power) {
    // power_level: 0x00 to 0x7F (0 to +7dBm)
    // measured_power: 0x00 (not measured) or actual measured value
    uint8_t data[] = {0x06, REQUEST, FUNC_ID_SERIALAPI_SETUP, TX_POWER_SET, power_level, measured_power};
    uint8_t chk = calculate_checksum(data, sizeof(data));
    uint8_t frame[] = {SOF, data[0], data[1], data[2], data[3], data[4], data[5], chk};
    
    debug_print_frame(frame, sizeof(frame), "TX Set TX Power");
    uart_write_bytes(UART_PORT, (char*)frame, sizeof(frame));
    
    ESP_LOGI(TAG, "Setting TX power to: 0x%02X (measured: 0x%02X)", power_level, measured_power);
}

// Get Z-Wave Library Version
static void zw_get_version(void) {
    uint8_t data[] = {0x03, REQUEST, FUNC_ID_ZW_GET_VERSION};
    uint8_t chk = calculate_checksum(data, sizeof(data));
    uint8_t frame[] = {SOF, data[0], data[1], data[2], chk};
    
    debug_print_frame(frame, sizeof(frame), "TX Get Version");
    uart_write_bytes(UART_PORT, (char*)frame, sizeof(frame));
    
    ESP_LOGI(TAG, "Requesting Z-Wave library version...");
}


// ==================== RESPONSE PARSING FUNCTIONS ====================

// Parse Capabilities response
static void parse_capabilities_response(uint8_t *data, int len) {
    ESP_LOGI(TAG, "=== CAPABILITIES RESPONSE ===");
    ESP_LOGI(TAG, "Length: %d bytes", len);
    
    if (len >= 43) {
        ESP_LOGI(TAG, "SerialAPI Version: %d.%02d", data[0], data[1]);
        ESP_LOGI(TAG, "Manufacturer ID: 0x%02X%02X", data[2], data[3]);
        ESP_LOGI(TAG, "Product Type: 0x%02X%02X", data[4], data[5]);
        ESP_LOGI(TAG, "Product ID: 0x%02X%02X", data[6], data[7]);
        
        // Function Classes
        ESP_LOGI(TAG, "Function Classes (%d):", data[8]);
        for (int i = 0; i < data[8] && i < 32; i++) {
            printf("  [%02d] 0x%02X\n", i, data[9 + i]);
        }
    }
    
    ESP_LOGI(TAG, "Full data:");
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n=============================\n");
}

// Parse SerialAPI Setup Supported response
static void parse_serialapi_supported_response(uint8_t *data, int len) {
    ESP_LOGI(TAG, "=== SERIALAPI SUPPORTED RESPONSE ===");
    ESP_LOGI(TAG, "Length: %d bytes", len);
    
    if (len >= 23) {
        ESP_LOGI(TAG, "SerialAPI Version: %d.%d", data[0] >> 4, data[0] & 0x0F);
        ESP_LOGI(TAG, "Manuf. ID: 0x%02X%02X", data[1], data[2]);
        ESP_LOGI(TAG, "Product Type: 0x%02X%02X", data[3], data[4]);
        ESP_LOGI(TAG, "Product ID: 0x%02X%02X", data[5], data[6]);
        
        // Supported Command Classes
        ESP_LOGI(TAG, "Supported Command Classes:");
        for (int i = 7; i < len && i < 23; i++) {
            if (data[i] != 0) {
                printf("  0x%02X\n", data[i]);
            }
        }
    }
    ESP_LOGI(TAG, "=============================\n");
}

// Parse SUC Node ID response
static void parse_suc_node_response(uint8_t *data, int len) {
    ESP_LOGI(TAG, "=== SUC NODE RESPONSE ===");
    
    if (len >= 2) {
        uint8_t suc_node_id = data[0];
        uint8_t capabilities = data[1];
        
        ESP_LOGI(TAG, "SUC Node ID: %d", suc_node_id);
        ESP_LOGI(TAG, "Capabilities: 0x%02X", capabilities);
        
        if (capabilities & 0x01) ESP_LOGI(TAG, "  - SUC/SIS is present");
        if (capabilities & 0x02) ESP_LOGI(TAG, "  - SUC is active");
        if (capabilities & 0x04) ESP_LOGI(TAG, "  - SIS present");
        if (capabilities & 0x08) ESP_LOGI(TAG, "  - SIS is active");
    }
    
    ESP_LOGI(TAG, "Full response:");
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n=============================\n");
}

// Parse Version response
static void parse_version_response(uint8_t *data, int len) {
    ESP_LOGI(TAG, "=== VERSION RESPONSE ===");
    
    if (len >= 12) {
        ESP_LOGI(TAG, "Library Type: 0x%02X", data[0]);
        
        const char *lib_type = "Unknown";
        switch(data[0]) {
            case 0x01: lib_type = "Static Controller"; break;
            case 0x02: lib_type = "Controller"; break;
            case 0x03: lib_type = "Enhanced Slave"; break;
            case 0x04: lib_type = "Slave"; break;
            case 0x05: lib_type = "Installer"; break;
            case 0x06: lib_type = "Routing Slave"; break;
            case 0x07: lib_type = "Bridge Controller"; break;
            case 0x08: lib_type = "Device under Test"; break;
        }
        ESP_LOGI(TAG, "Library Type: %s", lib_type);
        
        ESP_LOGI(TAG, "Protocol Version: %d.%02d", data[1], data[2]);
        ESP_LOGI(TAG, "Application Version: %d.%02d", data[3], data[4]);
    }
    
    ESP_LOGI(TAG, "Full data:");
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n=============================\n");
}

static void zw_stop_inclusion(void) {
    ESP_LOGI(TAG, "Stopping inclusion process...");
    
    // Send stop command with ADD_NODE_STOP (0x05)
    uint8_t stop_frame[] = {0x01, 0x04, 0x00, 0x4A, ADD_NODE_STOP, 0xB4};
    debug_print_frame(stop_frame, sizeof(stop_frame), "TX Stop Inclusion");
    uart_write_bytes(UART_PORT, (char*)stop_frame, sizeof(stop_frame));
    
    // Wait for ACK
    vTaskDelay(pdMS_TO_TICKS(100));
    
    zw_state = ZW_IDLE;
    ESP_LOGI(TAG, "Inclusion stopped, state set to ZW_IDLE");
}

static void zw_stop_exclusion(void) {
    ESP_LOGI(TAG, "Stopping exclusion process...");
    
    uint8_t data[] = {0x04, 0x00, 0x4B, 0x05};
    uint8_t chk = calculate_checksum(data, sizeof(data));
    uint8_t frame[] = {SOF, data[0], data[1], data[2], data[3], chk};
    
    debug_print_frame(frame, sizeof(frame), "TX Stop Exclusion");
    uart_write_bytes(UART_PORT, (char*)frame, sizeof(frame));
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    zw_state = ZW_IDLE;
    ESP_LOGI(TAG, "Exclusion stopped, state set to ZW_IDLE");
}

// ==================== NODE MANAGEMENT ====================
static void print_node_info(uint8_t node_id, uint8_t *nif_data, uint8_t nif_len) {
    ESP_LOGI(TAG, "=== NODE %d INFORMATION ===", node_id);
    ESP_LOGI(TAG, "NIF length: %d bytes", nif_len);

    if (nif_len >= 3) {
        ESP_LOGI(TAG, "Basic Device Class   : 0x%02X (%s)",
                 nif_data[0], basic_dev_str(nif_data[0]));
        ESP_LOGI(TAG, "Generic Device Class : 0x%02X", nif_data[1]);
        ESP_LOGI(TAG, "Specific Device Class: 0x%02X", nif_data[2]);
    }

    ESP_LOGI(TAG, "Supported Command Classes:");
    for (int i = 3; i < nif_len; i++) {
        ESP_LOGI(TAG, "  CC 0x%02X (%s)", nif_data[i], cc_str(nif_data[i]));
    }

    ESP_LOGI(TAG, "==========================");
}


static void add_node_to_list(uint8_t node_id, uint8_t *nif_data, uint8_t nif_len) {
    // Skip controller node
    if (node_id == 1) {
        ESP_LOGI(TAG, "Skipping controller node (Node 1)");
        return;
    }
    
    // Skip nodes with insufficient NIF data
    if (nif_len < 4) {
        ESP_LOGW(TAG, "Skipping node %d: insufficient NIF data (%d bytes)", node_id, nif_len);
        return;
    }
    
    // Skip nodes with all zeros in NIF
    bool all_zeros = true;
    for (int i = 0; i < nif_len; i++) {
        if (nif_data[i] != 0x00) {
            all_zeros = false;
            break;
        }
    }
    if (all_zeros) {
        ESP_LOGW(TAG, "Skipping node %d: NIF data is all zeros", node_id);
        return;
    }
    
    if (node_id == 0 || node_id > 232) {
        ESP_LOGW(TAG, "Invalid node ID %d", node_id);
        return;
    }
    
    if (g_node_count >= 232) {
        ESP_LOGW(TAG, "Cannot add node %d: maximum node count reached", node_id);
        return;
    }

    // Check if node already exists
    for (int i = 0; i < g_node_count; i++) {
        if (g_included_nodes[i].node_id == node_id) {
            ESP_LOGI(TAG, "Node %d already in list, updating info", node_id);
            g_included_nodes[i].nif_len = nif_len;
            memcpy(g_included_nodes[i].nif, nif_data, nif_len);
            
            // Update capabilities if available from NIF
            if (nif_len >= 1) {
                g_included_nodes[i].capabilities = nif_data[0];
            }
            if (nif_len >= 2) {
                g_included_nodes[i].device_type[0] = nif_data[1];
            }
            if (nif_len >= 3) {
                g_included_nodes[i].device_type[1] = nif_data[2];
            }
            if (nif_len >= 4) {
                g_included_nodes[i].device_type[2] = nif_data[3];
            }
            
            print_node_info(node_id, nif_data, nif_len);
            return;
        }
    }

    // Add new node
    node_info_t *node = &g_included_nodes[g_node_count];
    node->node_id = node_id;
    node->nif_len = nif_len;
    node->basic    = nif_data[0];
    node->generic  = nif_data[1];
    node->specific = nif_data[2];
    memcpy(node->nif, nif_data, nif_len);
    
    // Parse basic information from NIF
    if (nif_len >= 1) {
        node->capabilities = nif_data[0];
    }
    if (nif_len >= 2) {
        node->device_type[0] = nif_data[1];
    }
    if (nif_len >= 3) {
        node->device_type[1] = nif_data[2];
    }
    if (nif_len >= 4) {
        node->device_type[2] = nif_data[3];
    }
    
    g_node_count++;
    ESP_LOGI(TAG, "Node %d added to list. Total nodes: %d", node_id, g_node_count);
    ESP_LOGI(TAG, "Adding node %d with HomeID: %02X%02X%02X%02X", 
            node_id, 
            g_home_id[0], g_home_id[1], g_home_id[2], g_home_id[3]);    
    print_node_info(node_id, nif_data, nif_len);
    // /* ✅ SAVE TO NVS */
    // nvs_save_node_table();    
}

static void remove_node_from_list(uint8_t node_id)
{
    for (int i = 0; i < g_node_count; i++) {
        if (g_included_nodes[i].node_id == node_id) {

            ESP_LOGI(TAG, "Removing node %d from list", node_id);

            for (int j = i; j < g_node_count - 1; j++) {
                g_included_nodes[j] = g_included_nodes[j + 1];
            }

            g_node_count--;

            /* ✅ SAVE UPDATED TABLE */
            nvs_save_node_table();

            ESP_LOGI(TAG,
                "Node %d removed permanently. Total nodes: %d",
                node_id, g_node_count
            );
            return;
        }
    }

    ESP_LOGW(TAG, "Node %d not found in runtime list", node_id);
}

static void nvs_clear_all_nodes(void)
{
    nvs_handle_t nvs;

    esp_err_t err = nvs_open("nodes", NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No node table found in NVS");
        return;
    }

    ESP_LOGI(TAG, "Clearing all Z-Wave nodes from NVS");

    ESP_ERROR_CHECK(nvs_erase_all(nvs));
    ESP_ERROR_CHECK(nvs_commit(nvs));
    nvs_close(nvs);

    /* Clear runtime copy too */
    memset(g_included_nodes, 0, sizeof(g_included_nodes));
    g_node_count = 0;

    ESP_LOGI(TAG, "✓ All nodes removed from NVS and runtime");
}

static void classify_device(node_info_t  *node)
{
    if (node->basic == 0x04 &&
        node->generic == 0x10 &&
        node->specific == 0x04) {

        node->dev_type = DEV_SWITCH_BINARY;
        ESP_LOGI(TAG, "===================================================");
        ESP_LOGI(TAG, "Node %d classified as MULTI-ENDPOINT SWITCH BINARY", node->node_id);
        ESP_LOGI(TAG, "===================================================");
        show_Online_msg = false;
        char web_msg[128];
        snprintf(web_msg, sizeof(web_msg),
                "RS485 Device %d was adding...",
                node->node_id);
        send_msg_to_web(web_msg); 
        return;
    }

    if (node->basic == 0x04 &&
        node->generic == 0x18 &&
        node->specific == 0x01) {

        node->dev_type = DEV_REMOTE;
        ESP_LOGI(TAG, "=============================================");
        ESP_LOGI(TAG, "Node %d classified as REMOTE / CENTRAL SCENE", node->node_id);
        ESP_LOGI(TAG, "=============================================");
        // show_Online_msg = false;
        // send_msg_to_web("REMOTE Device was adding...");        
        show_Online_msg = false;
        char web_msg[128];
        snprintf(web_msg, sizeof(web_msg),
                "REMOTE Device %d was adding...",
                node->node_id);
        send_msg_to_web(web_msg);
        return;
    }

    node->dev_type = DEV_UNKNOWN;
    ESP_LOGE(TAG,
        "Node %d classified as UNKNOWN device "
        "(basic=0x%02X gen=0x%02X spec=0x%02X)",
        node->node_id,
        node->basic,
        node->generic,
        node->specific);
}

static void reset_zwave(){
    uint8_t frame[] = {
        SOF, 0x04, REQUEST,
        ZW_SET_DEFAULT,
        0x01,0x00
    };

    frame[5] = calculate_checksum(&frame[1], 4);
    uart_write_bytes(UART_PORT, (char *)frame, sizeof(frame));

    ESP_LOGE(TAG, "Sent Z-Wave Reset to Default");
}
static void zw_interview_task(void *arg)
{
    interview_msg_t msg;
    node_info_t *node = NULL;

    while (1) {
        kick_watchdog();// Kick watchdog before waiting for queue
        if (xQueueReceive(interview_queue, &msg, pdMS_TO_TICKS(1000))) {
            kick_watchdog();// Kick watchdog after receiving message
            node = find_node(msg.node_id);
            if (!node) continue;

            switch (msg.evt) {

            case INT_EVT_START:
                ESP_LOGI(TAG, "[INT] Start interview node %d", node->node_id);
                node->interview_state = INT_STATE_MANUFACTURER;
                zw_send_manufacturer_get(node->node_id);
                kick_watchdog();
                break;

            case INT_EVT_MANUFACTURER_DONE:
                
                if (node->dev_type == DEV_SWITCH_BINARY) {
                    node->interview_state = INT_STATE_MC_ENDPOINT;
                    zw_send_mc_endpoint_get(node->node_id);
                    web_dev_type = DEV_SWITCH_BINARY;
                } else if (node->dev_type == DEV_REMOTE) {
                    node->interview_state = INT_STATE_CENTRAL_SCENE;
                    zw_send_central_scene_get(node->node_id);
                    web_dev_type = DEV_REMOTE;
                } else {
                    node->interview_state = INT_STATE_DONE;
                }
                kick_watchdog();
                break;

            case INT_EVT_MC_ENDPOINT_DONE:
                node->current_ep = 1;
                node->interview_state = INT_STATE_MC_CAPABILITY;
                zw_send_mc_capability_get(node->node_id, node->current_ep);
                kick_watchdog();
                break;

            case INT_EVT_MC_CAPABILITY_DONE:
                node->current_ep++;
                if (node->current_ep <= node->endpoint_count) {
                    zw_send_mc_capability_get(node->node_id, node->current_ep);
                } else {
                    zw_send_mc_association_set(node->node_id);
                    node->interview_state = INT_STATE_DONE;
                }
                kick_watchdog();
                break;

            case INT_EVT_CENTRAL_SCENE_DONE:
                zw_setup_central_scene_association(node->node_id);
                node->interview_state = INT_STATE_DONE;
                kick_watchdog();
                break;

            default:
                kick_watchdog();
                break;
            }

            if (node->interview_state == INT_STATE_DONE) {
                node->interview_done = true;
                ESP_LOGI(TAG, "[INT] Interview completed for node %d", node->node_id);
                nvs_save_node_table();
                const char *device = web_dev_type == 1 ? "RS485 Device " : "REMOTE";
                const char *resp = " - was Added Successfully!!!";
                char buffer[64];
                snprintf(buffer, sizeof(buffer), "%s %d %s", device, node->node_id, resp);
                show_Online_msg = true;
                send_msg_to_web(buffer);
                kick_watchdog();
            }
        } else{
            //No message received within timeout, kick watchdog and continue
            kick_watchdog();
        }
    }
}

static void print_node_table(void)
{
    ESP_LOGI(TAG, "===== STORED NODE TABLE =====");

    for (int i = 0; i < g_node_count; i++) {
        node_info_t *n = &g_included_nodes[i];

        int ep_count = (n->dev_type == DEV_SWITCH_BINARY)
                        ? n->endpoint_count
                        : (n->dev_type == DEV_REMOTE ? 6 : 1);

        ESP_LOGI(TAG,
            "DEVICE %d -> Node %d | %-14s | MFG:%04X TYPE:%04X PROD:%04X",
            i + 1,
            n->node_id,
            dev_type_str(n->dev_type),
            n->manufacturer_id,
            n->product_type,
            n->product_id
        );
        if(n->dev_type == DEV_SWITCH_BINARY){// 0 - DEV_SWITCH_BINARY
            powerKeypad_nodeId = n->node_id;
        }
        else if(n->dev_type == DEV_REMOTE){// 0 - DEV_REMOTE
            remote_nodeId = n->node_id;
        }        
        for (int ep = 1; ep <= ep_count; ep++) {
            ESP_LOGI(TAG,
                "           Node %d | %-14s | EP:%d SC:%d",
                n->node_id,
                dev_type_str(n->dev_type),
                ep,
                n->scene_count
            );
        }
    }

    ESP_LOGI(TAG, "=============================");
}



void zw_send_manufacturer_get(uint8_t node_id)
{
    uint8_t frame[] = {
        0x01, 0x09, 0x00, 0x13,
        node_id,
        0x02,
        COMMAND_CLASS_MANUFACTURER_SPECIFIC, 0x04,
        0x05,
        0x01,
        0x00
    };

    frame[10] = calculate_checksum(&frame[1], 9);
    uart_write_bytes(UART_PORT, (char *)frame, sizeof(frame));

    ESP_LOGI(TAG, "Sent Manufacturer Specific Get to node %d", node_id);
}

void zw_send_mc_endpoint_get(uint8_t node_id)
{
    uint8_t frame[] = {
        0x01, 0x09, 0x00, 0x13,
        node_id,
        0x02,
        COMMAND_CLASS_MULTI_CHANNEL, 0x07,
        0x05,
        0x01,
        0x00
    };

    frame[10] = calculate_checksum(&frame[1], 9);
    uart_write_bytes(UART_PORT, (char *)frame, sizeof(frame));

    ESP_LOGI(TAG, "Sent Multi Channel Endpoint Get to node %d", node_id);
}

void zw_send_mc_capability_get(uint8_t node_id, uint8_t ep)
{
    uint8_t frame[] = {
        0x01, 0x0A, 0x00, 0x13,
        node_id,
        0x03,
        COMMAND_CLASS_MULTI_CHANNEL, 0x09,
        ep,
        0x05,
        0x01,
        0x00
    };

    frame[11] = calculate_checksum(&frame[1], 10);
    uart_write_bytes(UART_PORT, (char *)frame, sizeof(frame));

    ESP_LOGI(TAG, "Sent MC Capability Get (EP %d) to node %d", ep, node_id);
}

void zw_send_mc_association_set(uint8_t node_id)
{
    //0x01 0x0C 0x00 0x13 0x0C 0x06 0x8E 0x01 0x01 0x01 0x01 0x00 0x05 0x61
    uint8_t frame[] = {
        SOF, 0x0C, REQUEST, FUNC_ID_ZW_SEND_DATA,
        node_id,
        0x06,
        COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION, MULTI_CHANNEL_ASSOCIATION_SET,
        0x01,0x01,0x01,0x00,0x05,0x00
    };
    frame[13] = calculate_checksum(&frame[1], 12);
    uart_write_bytes(UART_PORT, (char *)frame, sizeof(frame));

    ESP_LOGI(TAG, "Sent MC Association to node %d", node_id);
}

void zw_send_central_scene_get(uint8_t node_id)
{
    uint8_t frame[] = {
        0x01, 0x09, 0x00, 0x13,
        node_id,
        0x02,
        COMMAND_CLASS_CENTRAL_SCENE, 0x01,
        0x05,
        0x01,
        0x00
    };

    frame[10] = calculate_checksum(&frame[1], 9);
    uart_write_bytes(UART_PORT, (char *)frame, sizeof(frame));

    ESP_LOGI(TAG, "Sent Central Scene Supported Get to node %d", node_id);
}

static void zw_setup_central_scene_association(uint8_t node_id)
{
    uint8_t frame[] = {
        SOF, 0x0C, REQUEST, FUNC_ID_ZW_SEND_DATA,
        node_id,
        0x06,
        COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION, MULTI_CHANNEL_ASSOCIATION_SET,
        LIFELINE_GROUP_1,0x01,0x01,0x00,0x05,0x00
    };
    frame[13] = calculate_checksum(&frame[1], 12);
    uart_write_bytes(UART_PORT, (char *)frame, sizeof(frame));

    ESP_LOGI(TAG, "Sent MC Lifeline association to node %d", node_id);
}

// ==================== RESPONSE HANDLING FOR ASSOCIATION AND PARAMETER CONFIG ====================

static void parse_multi_channel_association_report(
        uint8_t *cmd,
        uint8_t cmd_len,
        uint8_t src_node)
{
    /* Minimum valid length:
     * 8E 03 grp max rpt marker node ep  -> 8 bytes
     */
    if (cmd_len < 5) {
        ESP_LOGW(TAG_ASSOCIATION,
            "Invalid MC_ASSOCIATION_REPORT length: %d", cmd_len);
        return;
    }

    uint8_t group_id          = cmd[2];
    //uint8_t max_nodes         = cmd[3];
    //uint8_t reports_to_follow = cmd[4];
    if (cmd_len == 5) {
        ESP_LOGW(TAG_ASSOCIATION,"Group %d has NO associations", group_id);
        char web_msg[128];
        snprintf(web_msg, sizeof(web_msg),
                "Association Report: Remote %d Button %d has No mapping",
                src_node, group_id-1);
        show_Online_msg = true;
        send_msg_to_web(web_msg);        
        return;
    }
    ESP_LOGI(TAG_ASSOCIATION, "=== MULTI_CHANNEL_ASSOCIATION_REPORT ===");
    ESP_LOGI(TAG_ASSOCIATION, "Remote node      : %d", src_node);
    ESP_LOGI(TAG_ASSOCIATION, "Group ID         : %d", group_id);
    //ESP_LOGI(TAG_ASSOCIATION, "Max nodes        : %d", max_nodes);
    //ESP_LOGI(TAG_ASSOCIATION, "Reports to follow: %d", reports_to_follow);

    /* Start parsing after reports_to_follow */
    uint8_t idx = 5;

    /* -------- Plain Node Associations (before marker) -------- */
    while (idx < cmd_len && cmd[idx] != 0x00) {
        ESP_LOGI(TAG_ASSOCIATION,
            "Associated node (root EP): %d", cmd[idx]);
        idx++;
    }

    /* -------- Endpoint Associations (after marker) -------- */
    if (idx < cmd_len && cmd[idx] == 0x00) {
        idx++; // skip marker

        while ((idx + 1) < cmd_len) {
            uint8_t power_node = cmd[idx++];
            uint8_t endpoint   = cmd[idx++];

            ESP_LOGI(TAG_ASSOCIATION,
                "Associated node %d → endpoint %d",
                power_node, endpoint);
            char web_msg[128];
            snprintf(web_msg, sizeof(web_msg),
                    "Association Report: Remote %d Button %d mapped to RS485 Device %d endpoint %d",
                    src_node, group_id-1, power_node, endpoint);
            show_Online_msg = true;
            send_msg_to_web(web_msg);
        }
    }
    ESP_LOGI(TAG_ASSOCIATION, "==================================");
}


// Add this function to parse Configuration Report
static void parse_configuration_report(uint8_t *cmd, uint8_t cmd_len, uint8_t src_node) {
    if (cmd_len < 5) {
        ESP_LOGW(TAG, "Invalid CONFIGURATION_REPORT length: %d", cmd_len);
        return;
    }
    
    uint8_t param_num = cmd[2];
    uint8_t param_size = cmd[3];
    uint8_t param_value = 0;
    
    // Extract parameter value based on size
    for (int i = 0; i < param_size && (4 + i) < cmd_len; i++) {
        param_value |= (cmd[4 + i] << (8 * i));
    }
    
    ESP_LOGI(TAG, "=== CONFIGURATION_REPORT ===");
    ESP_LOGI(TAG, "Node: %d", src_node);
    ESP_LOGI(TAG, "Parameter Number: %d", param_num);
    // ESP_LOGI(TAG, "Parameter Size: %d byte(s)", param_size);
    ESP_LOGI(TAG, "Parameter Value: %d (0x%08X)", param_value, param_value);
    ESP_LOGI(TAG, "==========================");
}

// ==================== UART RX TASK ====================
static void zw_uart_rx_task(void *arg) {
    uint8_t rx[256];
    //static uint32_t inclusion_timeout = 0;
    static uint32_t exclusion_timeout = 0;
    static bool debug_mode = true;

    while (1) {
        int len = uart_read_bytes(UART_PORT, rx, sizeof(rx), pdMS_TO_TICKS(100));
        
        if (len > 0) {
            if (debug_mode) {
                ESP_LOGI(TAG, "RAW RX (%d bytes):", len);
                for (int i = 0; i < len; i++) {
                    printf("%02X ", rx[i]);
                    if ((i + 1) % 16 == 0) printf("\n");
                }
                printf("\n");
                //ESP_LOG_BUFFER_HEXDUMP("rx ->", rx, len, ESP_LOG_INFO);
            }
            
            // Handle ACK (0x06) if present at beginning
            int start_idx = 0;
            if (rx[0] == 0x06) {
                ESP_LOGI(TAG, "Received ACK (0x06)");
                start_idx = 1;
                
                // If we only got ACK, send ACK back and continue
                if (len == 1) {
                    send_ack();
                    continue;
                }
            }
            
            // Now look for SOF starting from start_idx
            int sof = find_sof(&rx[start_idx], len - start_idx);
            if (sof < 0) {
                // No SOF found, might be just ACK or other control byte
                if (len == 1 && rx[0] == CAN) {
                    ESP_LOGW(TAG, "Received CAN (0x18)");
                }
                continue;
            }
            
            // Adjust sof to be absolute position in rx buffer
            sof += start_idx;
            
            send_ack();  // Send ACK for valid SOF

            uint8_t frame_len = rx[sof + 1];  // Length byte
            uint8_t type = rx[sof + 2];
            uint8_t func = rx[sof + 3];
            
            // Validate frame length
            if (frame_len < 2 || frame_len > 64) {
                ESP_LOGW(TAG, "Invalid frame length: %d", frame_len);
                continue;
            }

            ESP_LOGD(TAG, "Frame: len=%d, type=0x%02X, func=0x%02X", frame_len, type, func);

            if (type == REQUEST && func == FUNC_ID_SERIALAPI_STARTED) {
                ESP_LOGI(TAG, "SERIALAPI_STARTED received");
                zw_state = ZW_WAIT_RF_SET;
                send_rf_region_india();
            } 
            else if (type == RESPONSE && func == FUNC_ID_SERIALAPI_SETUP && zw_state == ZW_WAIT_RF_SET) {
                ESP_LOGI(TAG, "RF region set successfully");
                zw_state = ZW_WAIT_HOMEID;
                send_memory_get_id();
            } 
            else if (type == RESPONSE && func == FUNC_ID_ZW_SET_DEFAULT) {
                ESP_LOGW(TAG, "Controller reset done...");
            }
            // Handle FUNC_ID_ZW_GET_CAPABILITIES response
            else if (type == RESPONSE && func == FUNC_ID_ZW_GET_CAPABILITIES) {
                ESP_LOGI(TAG, "=== GET CAPABILITIES RESPONSE ===");
                uint8_t response_data[256];
                int data_len = frame_len - 2;  // Subtract type and func bytes
                
                if (data_len > 0 && (sof + 4 + data_len) <= len) {
                    memcpy(response_data, &rx[sof + 4], data_len);
                    parse_capabilities_response(response_data, data_len);
                }
            }

            // Handle FUNC_ID_ZW_GET_SUC_NODE_ID response
            else if (type == RESPONSE && func == FUNC_ID_ZW_GET_SUC_NODE_ID) {
                ESP_LOGI(TAG, "=== GET SUC NODE ID RESPONSE ===");
                uint8_t response_data[256];
                int data_len = frame_len - 2;
                
                if (data_len > 0 && (sof + 4 + data_len) <= len) {
                    memcpy(response_data, &rx[sof + 4], data_len);
                    parse_suc_node_response(response_data, data_len);
                }
            }

            // Handle FUNC_ID_SERIALAPI_SETUP responses (multiple sub-commands)
            else if (type == RESPONSE && func == FUNC_ID_SERIALAPI_SETUP) {
                if (frame_len >= 2) {  // Need at least sub-command byte
                    // SUB-COMMAND is at position sof+4 (after SOF, LEN, TYPE, FUNC_ID)
                    uint8_t sub_cmd = rx[sof + 4];
                    
                    ESP_LOGI(TAG, "SERIALAPI_SETUP Response - Sub-command: 0x%02X", sub_cmd);
                    
                    switch(sub_cmd) {
                        case 0x01:  // Supported Command Classes
                            ESP_LOGI(TAG, "=== SERIALAPI SUPPORTED RESPONSE ===");
                            // Data starts at sof+5
                            if (frame_len >= 3) {
                                uint8_t response_data[256];
                                int data_len = frame_len - 2;  // Subtract type and func
                                
                                if ((sof + 5 + data_len) <= len) {
                                    memcpy(response_data, &rx[sof + 5], data_len);
                                    parse_serialapi_supported_response(response_data, data_len);
                                }
                            }
                            break;
                            
                        case TX_POWER_SET:  // 0x04
                            ESP_LOGI(TAG, "=== TX POWER SET RESPONSE ===");
                            if (frame_len >= 3) {
                                uint8_t status = rx[sof + 5];
                                ESP_LOGI(TAG, "Status: 0x%02X", status);
                                
                                // According to Z-Wave docs: 0x01 = SUCCESS for this command
                                if (status == 0x01) {
                                    ESP_LOGI(TAG, "✓ TX power set SUCCESSFULLY to 0x7F");
                                    ESP_LOGI(TAG, "TX Power is now at maximum (+7 dBm)");
                                } else if (status == 0x00) {
                                    ESP_LOGW(TAG, "TX power set failed");
                                } else {
                                    ESP_LOGI(TAG, "TX power set status: 0x%02X", status);
                                }
                            }
                            break;
                            
                        case TX_POWER_GET:  // 0x03
                            ESP_LOGI(TAG, "=== TX POWER GET RESPONSE ===");
                            // Power value is at sof+5
                            if (frame_len >= 3) {
                                uint8_t power_value = rx[sof + 5];
                                ESP_LOGI(TAG, "Current TX Power: 0x%02X", power_value);
                            }
                            break;
                            
                        case RF_REGION_GET:  // 0x20
                            ESP_LOGI(TAG, "=== RF REGION GET RESPONSE ===");
                            // Region value is at sof+5
                            if (frame_len >= 3) {
                                uint8_t region = rx[sof + 5];
                                ESP_LOGI(TAG, "Current RF Region: 0x%02X", region);
                                print_region_info(region);
                            }
                            break;
                            
                        case RF_REGION_SET:  // 0x40
                            ESP_LOGI(TAG, "=== RF REGION SET RESPONSE ===");
                            if (frame_len >= 3) {
                                uint8_t status = rx[sof + 5];
                                ESP_LOGI(TAG, "Status: 0x%02X", status);
                                
                                // 0x01 = SUCCESS for RF Region Set too
                                if (status == 0x01) {
                                    ESP_LOGI(TAG, "✓ RF Region set SUCCESSFULLY");
                                } else {
                                    ESP_LOGW(TAG, "RF Region set failed");
                                }
                            }
                            break;
                            
                        default:
                            ESP_LOGI(TAG, "Unknown SERIALAPI_SETUP sub-command: 0x%02X", sub_cmd);
                            break;
                    }
                }
            }
            // Handle FUNC_ID_ZW_GET_VERSION response
            else if (type == RESPONSE && func == FUNC_ID_ZW_GET_VERSION) {
                ESP_LOGI(TAG, "=== GET VERSION RESPONSE ===");
                uint8_t response_data[256];
                int data_len = frame_len - 2;
                
                if (data_len > 0 && (sof + 4 + data_len) <= len) {
                    memcpy(response_data, &rx[sof + 4], data_len);
                    parse_version_response(response_data, data_len);
                }
            }
                        // Handle RF Region Set response
                        else if (type == RESPONSE && func == FUNC_ID_MEMORY_GET_ID && zw_state == ZW_WAIT_HOMEID) {
                            if (frame_len >= 5) {
                                memcpy(g_home_id, &rx[sof + 4], 4);
                                g_home_id_valid = true;
                                ESP_LOGI(TAG, "HomeID: %02X%02X%02X%02X",
                                        g_home_id[0], g_home_id[1], g_home_id[2], g_home_id[3]);
                            }
                            zw_state = ZW_IDLE;
                            ESP_LOGI(TAG, "Z-Wave controller ready");

                        } 
            else if (type == REQUEST && func == FUNC_ID_ZW_ADD_NODE_TO_NETWORK)
            {
                uint8_t status = rx[sof + 5];
                uint8_t nodeId = (frame_len >= 6) ? rx[sof + 6] : 0;

                ESP_LOGI(TAG,
                    "ADD_NODE → status=%s node=%d",
                    add_node_status_str(status), nodeId
                );

                /* Ignore stale callbacks */
                if (zw_state != ZW_WAIT_INCLUSION &&
                    status != ADD_NODE_STATUS_LEARN_READY) {
                    ESP_LOGW(TAG, "Ignoring ADD_NODE callback outside inclusion");
                    continue;
                }

                switch (status)
                {
                    case ADD_NODE_STATUS_LEARN_READY:
                        ESP_LOGI(TAG, "Controller ready");
                        break;

                    case ADD_NODE_STATUS_NODE_FOUND:
                        ESP_LOGI(TAG, "Device detected");
                        show_Online_msg = false;
                        send_msg_to_web("Device detected and adding please wait!!");
                        break;

                    case ADD_NODE_STATUS_ADDING_SLAVE:
                    {
                        if (frame_len < 8) break;

                        uint8_t nif_len = rx[sof + 7];
                        if (nif_len < 4) break;
                        if (nif_len > sizeof(pending_nif))
                            nif_len = sizeof(pending_nif);

                        pending_node_id = nodeId;
                        pending_nif_len = nif_len;
                        memcpy(pending_nif, &rx[sof + 8], nif_len);

                        ESP_LOGI(TAG,
                            "Node %d assigned, NIF stored (%d bytes)",
                            nodeId, nif_len
                        );
                        break;
                    }

                    /* 🔥 KEY CHANGE IS HERE 🔥 */
                    case ADD_NODE_STATUS_PROTOCOL_DONE:
                    {
                        inclusion_protocol_done = true;

                        ESP_LOGI(TAG,
                            "Protocol done for node %d",
                            pending_node_id
                        );

                        /* SEND STOP EXACTLY ONCE */
                        if (!stop_sent_after_protocol) {

                            uint8_t stop_frame[] = {
                                0x01, 0x04, 0x00,
                                0x4A,
                                ADD_NODE_STOP,
                                0xB4
                            };

                            ESP_LOGI(TAG,
                                "Sending ADD_NODE_STOP after PROTOCOL_DONE"
                            );
                            add_node_found=true;
                            uart_write_bytes(
                                UART_PORT,
                                (char *)stop_frame,
                                sizeof(stop_frame)
                            );

                            stop_sent_after_protocol = true;
                        }
                        break;
                    }

                    case ADD_NODE_STATUS_DONE:
                    {
                        if (pending_node_id == 0 || nodeId != pending_node_id) {
                            ESP_LOGW(TAG,
                                "DONE ignored (stale node %d)",
                                nodeId
                            );
                            break;
                        }

                        if (inclusion_completed) {
                            ESP_LOGW(TAG, "Duplicate DONE ignored");
                            break;
                        }

                        inclusion_completed = true;

                        ESP_LOGI(TAG,
                            "✔ Inclusion COMPLETED for node %d",
                            nodeId
                        );
                        
                        /* Add node */
                        add_node_to_list(
                            pending_node_id,
                            pending_nif,
                            pending_nif_len
                        );

                        node_info_t *node = find_node(nodeId);
                        if (node) {
                            classify_device(node);

                            interview_msg_t msg = {
                                .node_id = nodeId,
                                .evt = INT_EVT_START
                            };
                            xQueueSend(interview_queue, &msg, 0);
                        }

                        /* Cleanup */
                        pending_node_id          = 0;
                        pending_nif_len          = 0;
                        inclusion_protocol_done  = false;
                        stop_sent_after_protocol = false;
                        zw_state = ZW_IDLE;

                        break;
                    }

                    case ADD_NODE_STATUS_FAILED:
                        ESP_LOGE(TAG, "Inclusion FAILED");
                        zw_state = ZW_IDLE;
                        show_Online_msg = true;
                        send_msg_to_web("Inclusion FAILED!!");
                        break;

                    default:
                        break;
                }
            }


            else if (type == REQUEST && func == FUNC_ID_ZW_REMOVE_NODE_FROM_NETWORK) {
                if (frame_len < 3) continue;
                if (zw_state != ZW_WAIT_EXCLUSION) {
                    ESP_LOGD(TAG,
                        "Ignoring REMOVE_NODE callback outside exclusion "
                        "(status=0x%02X)",
                        rx[sof + 5]
                    );
                    continue;   // NOT return (we are inside a task loop)
                }
            //uint8_t callback_id = rx[sof + 4];
            uint8_t status      = rx[sof + 5];
            uint8_t nodeId      = (frame_len >= 6) ? rx[sof + 6] : 0;


                ESP_LOGI(TAG, "REMOVE_NODE cb: status=0x%02X nodeId=%d", status, nodeId);

                switch (status) {
                    case REMOVE_NODE_STATUS_LEARN_READY:
                        ESP_LOGI(TAG, "Exclusion: Learn ready - waiting for device");
                        exclusion_timeout = 0;
                        break;

                    case REMOVE_NODE_STATUS_NODE_FOUND:
                        ESP_LOGI(TAG, "Exclusion: Node found");
                        exclusion_timeout = 0;
                        break;

                    case REMOVE_NODE_STATUS_REMOVING_SLAVE:
                        ESP_LOGI(TAG, "Exclusion: Removing slave node %d", nodeId);
                        exclusion_timeout = 0;
                        // Store the node being excluded for reference
                        ESP_LOGI(TAG, "Node %d is being excluded from network", nodeId);
                        break;

                    case REMOVE_NODE_STATUS_REMOVING_CONTROLLER:
                        ESP_LOGW(TAG, "Exclusion: Removing controller node %d", nodeId);
                        exclusion_timeout = 0;
                        break;

                    case REMOVE_NODE_STATUS_PROTOCOL_DONE:
                        ESP_LOGI(TAG, "Exclusion protocol done (internal), waiting for DONE");
                        exclusion_timeout = 0;
                        break;
                    case REMOVE_NODE_STATUS_DONE:
                        if (!exclusion_in_progress) {
                            ESP_LOGW(TAG, "Duplicate DONE ignored");
                            break;
                        }

                        exclusion_in_progress = false;

                        ESP_LOGI(TAG, "✔ Exclusion COMPLETED for node %d", nodeId);

                        if (nodeId != 0) {
                            remove_node_from_list(nodeId);
                            //persist_remove_node(nodeId);
                        }

                        zw_stop_exclusion();
                        zw_state = ZW_IDLE;

                        ESP_LOGI(TAG,
                            "========== EXCLUSION COMPLETED SUCCESSFULLY =========="
                        );
                        remove_node_found = true;
                        show_Online_msg = true;
                        char buffer[64];
                        snprintf(buffer, sizeof(buffer), "DeviceID %d Exclusion Completed!!", nodeId);

                        send_msg_to_web(buffer);
                        break;


                    case REMOVE_NODE_STATUS_FAILED:
                        ESP_LOGE(TAG, "Exclusion FAILED");
                        show_Online_msg = true;
                        send_msg_to_web("Exclusion Failed!!");
                        zw_stop_exclusion();
                        break;

                    default:
                        ESP_LOGW(TAG, "Unknown REMOVE_NODE status 0x%02X", status);
                        exclusion_timeout = 0;
                        break;
                }
            }
            else if (type == RESPONSE && func == FUNC_ID_ZW_IS_PRIMARY_CTRL) {
                if (frame_len >= 2) {
                    uint8_t is_primary = rx[sof + 4];
                    if (is_primary) {
                        ESP_LOGI(TAG, "Controller is PRIMARY");
                    } else {
                        ESP_LOGW(TAG, "Controller is NOT PRIMARY - inclusion may not work!");
                    }
                }
            }
            else if (type == REQUEST && func == FUNC_ID_APPLICATION_UPDATE) {
                // APPLICATION_UPDATE callback
                if (frame_len >= 6) {
                    uint8_t status = rx[sof + 4];
                    uint8_t nodeId = rx[sof + 5];
                    uint8_t lenNif = (frame_len >= 7) ? rx[sof + 6] : 0;
                    
                    ESP_LOGI(TAG, "Application Update: nodeId=%d, status=0x%02X, nif_len=%d", 
                            nodeId, status, lenNif);
                    
                    if (status == 0x84 && lenNif > 0 && (sof + 7 + lenNif) <= len) {
                        // This is a Node Information Frame (NIF) response
                        ESP_LOGI(TAG, "Received NIF for node %d (len=%d)", nodeId, lenNif);
                        
                        // Extract NIF data
                        uint8_t nif_data[29];
                        uint8_t actual_len = MIN(lenNif, sizeof(nif_data));
                        memcpy(nif_data, &rx[sof + 7], actual_len);
                        
                        if (zw_state == ZW_WAIT_INCLUSION) {
                            ESP_LOGI(TAG,
                                "Device %d found during exclusion. Put device in exclusion mode!",
                                nodeId
                            );
                        } 
                        else if (zw_state == ZW_WAIT_EXCLUSION) {
                            // During exclusion, status 0x84 might mean device found
                            ESP_LOGI(TAG, "Device %d found during exclusion. Put device in exclusion mode!", nodeId);
                            // Don't remove yet - wait for REMOVE_NODE_FROM_NETWORK callback
                        }
                        
                    } else if (status == 0x00) {
                        // This is an exclusion request - device removed
                        ESP_LOGI(TAG, "Application update: node %d removed (info only)",nodeId
                        );
                    } else if (status == 0x01) {
                        // Node info received during inclusion/exclusion
                        ESP_LOGI(TAG, "Node info received for node %d", nodeId);
                    } else if (status == 0x02) {
                        // Routing pending
                        ESP_LOGI(TAG, "Routing pending for node %d", nodeId);
                    } else if (status == 0x03) {
                        // New ID assigned
                        ESP_LOGI(TAG, "New ID assigned for node %d", nodeId);
                    }
                }
            }
            /* Ignore SendData transmit callbacks */
            if (type == REQUEST && func == FUNC_ID_ZW_SEND_DATA) {
                ESP_LOGD(TAG, "SendData callback ignored");
                continue;
            }
            // Handle ZW_SEND_DATA response (transmit status)
            else if (type == RESPONSE && func == FUNC_ID_ZW_SEND_DATA) {
                // if (frame_len >= 2) {
                //     uint8_t transmit_status = rx[sof + 4];
                //     ESP_LOGI(TAG, "ZW_SEND_DATA response - Transmit status: 0x%02X", transmit_status);
                    
                //     // Common transmit status values:
                //     switch(transmit_status) {
                //         case 0x00: ESP_LOGI(TAG, "Transmit complete OK"); break;
                //         case 0x01: ESP_LOGW(TAG, "Transmit complete no ACK"); break;
                //         case 0x02: ESP_LOGE(TAG, "Transmit complete fail"); break;
                //         case 0x03: ESP_LOGW(TAG, "Transmit routing not idle"); break;
                //         case 0x04: ESP_LOGW(TAG, "Transmit complete no route"); break;
                //         default: ESP_LOGI(TAG, "Unknown transmit status"); break;
                //     }
                // }
            }            
            else if (type == REQUEST && func == FUNC_ID_APPLICATION_COMMAND_HANDLER) {

                //uint8_t status   = rx[sof + 4];
                uint8_t srcNode  = rx[sof + 6];
                uint8_t cmd_len  = rx[sof + 7];
                uint8_t *cmd     = &rx[sof + 8];
                node_info_t *node = find_node(srcNode);
                if (!node) {
                    ESP_LOGW(TAG, "Cmd from unknown node %d", srcNode);
                    continue;
                }

                uint8_t cc  = cmd[0];
                uint8_t cmd_id = cmd[1];

                /* ---------------- Manufacturer Specific ---------------- */
                if (cc == COMMAND_CLASS_MANUFACTURER_SPECIFIC && cmd_id == CC_MANUFACTURER_SPECIFIC_REPORT) {

                    node->manufacturer_id = (cmd[2] << 8) | cmd[3];
                    node->product_type    = (cmd[4] << 8) | cmd[5];
                    node->product_id      = (cmd[6] << 8) | cmd[7];
                    ESP_LOGI(TAG, "Manufacturer Report: node=%d man=0x%04X type=0x%04X prod=0x%04X", 
                        srcNode, node->manufacturer_id, node->product_type, node->product_id );
                    interview_msg_t msg = {
                        .node_id = srcNode,
                        .evt = INT_EVT_MANUFACTURER_DONE
                    };
                    xQueueSend(interview_queue, &msg, 0);
                }


                /* ---------------- Multi Channel Endpoint Report ---------------- */
                else if (cc == COMMAND_CLASS_MULTI_CHANNEL && cmd_id == 0x08) {

                    node->endpoint_count = cmd[3];  // # of individual endpoints

                    interview_msg_t msg = {
                        .node_id = srcNode,
                        .evt = INT_EVT_MC_ENDPOINT_DONE
                    };
                    ESP_LOGI(TAG, "Node %d reports %d endpoints", srcNode, node->endpoint_count );                    
                    xQueueSend(interview_queue, &msg, 0);
                }

                /* ---------------- Multi Channel Capability Report ---------------- */
                else if (cc == COMMAND_CLASS_MULTI_CHANNEL && cmd_id == 0x0A) {

                    uint8_t ep = cmd[2];
                    uint8_t gen = cmd[3];
                    uint8_t spec = cmd[4];

                    ESP_LOGI(TAG,
                        "EP %d: Generic=0x%02X Specific=0x%02X",
                        ep, gen, spec
                    );

                    ESP_LOGI(TAG, "EP %d Supported CCs:", ep);
                    for (int i = 5; i < cmd_len; i++) {
                        if (cmd[i] == 0x00) break;
                        ESP_LOGI(TAG, "  CC 0x%02X", cmd[i]);
                    }
                    interview_msg_t msg = {
                        .node_id = srcNode,
                        .evt = INT_EVT_MC_CAPABILITY_DONE,
                        .endpoint = ep
                    };
                    xQueueSend(interview_queue, &msg, 0);                    
                }

                /* ---------------- Central Scene Supported Report ---------------- */
                else if (cc == COMMAND_CLASS_CENTRAL_SCENE && cmd_id == 0x02) {

                    node->scene_count = cmd[2];
                    node->scene_flags = cmd[3];

                    ESP_LOGI(TAG,
                        "Central Scene: node=%d scenes=%d flags=0x%02X",
                        srcNode,
                        node->scene_count,
                        node->scene_flags
                    );
                    interview_msg_t msg = {
                        .node_id = srcNode,
                        .evt = INT_EVT_CENTRAL_SCENE_DONE
                    };
                    xQueueSend(interview_queue, &msg, 0);
                }
                /* ---------------- Central Scene Supported Report ---------------- */
                else if (cc == COMMAND_CLASS_CENTRAL_SCENE && cmd_id == 0x03) {
                    char web_msg[128];
                    snprintf(web_msg, sizeof(web_msg),
                            "REMOTE Device %d was Reported",srcNode);
                    show_Online_msg = true;
                    send_msg_to_web(web_msg);
                    ESP_LOGI(TAG,"REMOTE Node %d Reports",srcNode);
                }
                else if (cc == COMMAND_CLASS_MULTI_CHANNEL && cmd_id == 0x0D) {
                    char web_msg[128];
                    snprintf(web_msg, sizeof(web_msg),
                            "RS485 Device %d was Reported",srcNode);
                    show_Online_msg = true;
                    send_msg_to_web(web_msg);
                    ESP_LOGI(TAG,"RS485 Node %d Reports",srcNode);
                }

                /* ---------------- Multi Channel Association Report Response ---------------- */
                else if (cc == COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION &&
                        cmd_id == MULTI_CHANNEL_ASSOCIATION_REPORT) {

                    ESP_LOGI(TAG_ASSOCIATION,
                        "MC Association Report from node %d", srcNode);

                    parse_multi_channel_association_report(cmd, cmd_len, srcNode);
                }
                /* ---------------- Configuration Report ---------------- */
                else if (cc == COMMAND_CLASS_CONFIGURATION && cmd_id == CONFIGURATION_REPORT) {
                    ESP_LOGI(TAG, "Configuration Report from node %d", srcNode);
                    parse_configuration_report(cmd, cmd_len, srcNode);
                    
                    // Send message to web
                    char web_msg[128];
                    uint8_t param_num = cmd[2];
                    uint8_t param_size = cmd[3];
                    int32_t param_value = 0;
                    
                    for (int i = 0; i < param_size && (4 + i) < cmd_len; i++) {
                        param_value |= (cmd[4 + i] << (8 * i));
                    }
                    
                    snprintf(web_msg, sizeof(web_msg), 
                            "Parameter %d = %ld (RS485 Device %d)", 
                            param_num, param_value, srcNode);
                    show_Online_msg = true;
                    send_msg_to_web(web_msg);
                }

                /* ---------------- Configuration Set Response ---------------- */
                else if (cc == COMMAND_CLASS_CONFIGURATION && cmd_id == CONFIGURATION_SET) {
                    ESP_LOGI(TAG, "Configuration Set response from node %d", srcNode);
                    
                    // Check if successful (first byte after cmd_id should be 0x00 for success)
                    if (cmd_len >= 3 && cmd[2] == 0x00) {
                        ESP_LOGI(TAG, "Parameter set SUCCESSFUL");
                        show_Online_msg = true;
                        send_msg_to_web("Parameter set successful");
                    } else {
                        ESP_LOGW(TAG, "Parameter set FAILED try again!");
                        show_Online_msg = true;
                        send_msg_to_web("Parameter set failed");
                    }
                }                
            }

            else if (type == RESPONSE && func == FUNC_ID_ZW_REQUEST_NODE_INFO) {
                if (frame_len >= 3) {
                    uint8_t nodeId = rx[sof + 4];
                    uint8_t status = rx[sof + 5];
                    
                    ESP_LOGI(TAG, "Node info response: node=%d, status=0x%02X", nodeId, status);
                    
                    if (status == 0x00) {
                        ESP_LOGI(TAG, "Node info request accepted for node %d", nodeId);
                    }
                }
            }
            else if (type == RESPONSE && func == FUNC_ID_ZW_GET_NODE_PROTOCOL_INFO) {
                if (frame_len >= 14) {
                    uint8_t nodeId = rx[sof + 4];
                    uint8_t listening = rx[sof + 5];
                    uint8_t routing = rx[sof + 6];
                    uint8_t version = rx[sof + 7];
                    uint8_t security = rx[sof + 8];
                    
                    ESP_LOGI(TAG, "Protocol info for node %d:", nodeId);
                    ESP_LOGI(TAG, "  Listening: 0x%02X, Routing: 0x%02X", listening, routing);
                    ESP_LOGI(TAG, "  Version: %d, Security: 0x%02X", version, security);
                    
                    // Device type bytes
                    for (int i = 0; i < 3; i++) {
                        if (sof + 9 + i < len) {
                            ESP_LOGI(TAG, "  Device Type[%d]: 0x%02X", i, rx[sof + 9 + i]);
                        }
                    }
                }
            }
            
            kick_watchdog();
        }
        
        if (len <= 0) {
            if (zw_state == ZW_WAIT_EXCLUSION) {
                if (exclusion_timeout++ > 300) { // 30 seconds timeout
                    ESP_LOGW(TAG, "Exclusion timeout");
                    zw_stop_exclusion();
                    exclusion_timeout = 0;
                }
            }
            continue;
        }
    }
}

void send_msg_to_web(const char *msg)
{
    ESP_LOGI(TAG_WEB, "%s", msg);

    if (web_status_mutex &&
        xSemaphoreTake(web_status_mutex, pdMS_TO_TICKS(100))) {

        strncpy(web_status, msg, sizeof(web_status) - 1);
        web_status[sizeof(web_status) - 1] = '\0';

        xSemaphoreGive(web_status_mutex);
    }

    /* Restart one-shot timer */
    esp_timer_stop(web_msg_timer);
    esp_timer_start_once(web_msg_timer, 5 * 1000 * 1000);
}

void set_parameter_config(uint8_t dest_device_id, uint8_t param_num, uint8_t param_value){
    /*
    *Setting Parameters for Individual Buttons
    * Parameter Number - 1 : for Btn-1 Keypad ID
    * Parameter NUmber - 2 : for Btn-1 Keypad ID's Button
    * Parameter Number - 3 : for Btn-2 Keypad ID
    * Parameter NUmber - 4 : for Btn-2 Keypad ID's Button
    * Parameter Number - 5 : for Btn-3 Keypad ID
    * Parameter NUmber - 6 : for Btn-3 Keypad ID's Button
    * Parameter Number - 7 : for Btn-4 Keypad ID
    * Parameter NUmber - 8 : for Btn-4 Keypad ID's Button
    * Parameter Number - 9 : for Btn-5 Keypad ID
    * Parameter NUmber - 10 : for Btn-5 Keypad ID's Button
    * Parameter Number - 11 : for Btn-6 Keypad ID
    * Parameter NUmber - 12 : for Btn-6 Keypad ID's Button
    *
    *Reset Parameters for Individual Buttons
    * Parameter Number - 1 : Parameter Value = 0 : clear Btn-1 Keypad ID
    * Parameter NUmber - 2 : Parameter Value = 0 : clear Btn-1 Keypad ID's Button
    * Parameter Number - 3 : Parameter Value = 0 : clear Btn-2 Keypad ID
    * Parameter NUmber - 4 : Parameter Value = 0 : clear Btn-2 Keypad ID's Button
    * Parameter Number - 5 : Parameter Value = 0 : clear Btn-3 Keypad ID
    * Parameter NUmber - 6 : Parameter Value = 0 : clear Btn-3 Keypad ID's Button
    * Parameter Number - 7 : Parameter Value = 0 : clear Btn-4 Keypad ID
    * Parameter NUmber - 8 : Parameter Value = 0 : clear Btn-4 Keypad ID's Button
    * Parameter Number - 9 : Parameter Value = 0 : clear Btn-5 Keypad ID
    * Parameter NUmber - 10 : Parameter Value = 0 : clear Btn-5 Keypad ID's Button
    * Parameter Number - 11 : Parameter Value = 0 : clear Btn-6 Keypad ID
    * Parameter NUmber - 12 : Parameter Value = 0 : clear Btn-6 Keypad ID's Button
    *
    * Resetting all Parameters
    * Parameter Number - 255 : Parameter Value = 0 : Reset all Mapppings
    */

    /*Parameter Set 
    Send Command : 0x01 0x0C 0x00 0x13 0x66 0x05 0x70 0x04 0x03 0x01 0xFF 0x25 0x01 0x2E 
    01 - SOF  
    0C - LENGTH  
    00 - REQUEST  
    13 - FUNC_ID_ZW_SEND_DATA  
    66 - Destination Node ID = 102 
    05 - Command LENGTH  
    70 - COMMAND_CLASS_CONFIGURATION  
    04 - CONFIGURATION_SET  
    03 - Parameter Number = 3  
    01 - Parameter Size = 1 byte 
    FF - Parameter Value = 0xFF  
    25 - Transmit Options 
    01 - Callback ID 
    4E - CHECKSUM */
    uint8_t frame[] = {
        SOF, 0x0C, REQUEST, FUNC_ID_ZW_SEND_DATA,
        dest_device_id, 0x05,
        COMMAND_CLASS_CONFIGURATION, CONFIGURATION_SET,
        param_num, 0x01, 
        param_value,TX_OPTIONS,
        0x01, 0x00
    };

    frame[13] = calculate_checksum(&frame[1], 12);
    uart_write_bytes(UART_PORT, (char *)frame, sizeof(frame));

    ESP_LOGI(TAG_PARAMETER, "Sent Parameter Set to Destination Node %d", dest_device_id);    
}

void get_parameter_config(uint8_t dest_device_id, uint8_t param_num){
    /*Parameter Get 
    Send Command : 0x01 0x0A 0x00 0x13 0x69 0x03 0x70 0x05 0x03 0x25 0x01 0xDE 
    01 - SOF  
    0A - LENGTH  
    00 - REQUEST  
    13 - FUNC_ID_ZW_SEND_DATA  
    69 - Destination Node ID = 105 
    03 - Command LENGTH  
    70 - COMMAND_CLASS_CONFIGURATION  
    05 - CONFIGURATION_GET  
    03 - Parameter Number = 3  
    25 - Transmit Options 
    01 - Callback ID 
    DE - CHECKSUM */

    uint8_t frame[] = {
        SOF, 0x0A, REQUEST, FUNC_ID_ZW_SEND_DATA,
        dest_device_id, 0x03,
        COMMAND_CLASS_CONFIGURATION, CONFIGURATION_GET,
        param_num, TX_OPTIONS,
        0x01, 0x00
    };

    frame[11] = calculate_checksum(&frame[1], 10);
    uart_write_bytes(UART_PORT, (char *)frame, sizeof(frame));

    ESP_LOGI(TAG_PARAMETER, "Sent Parameter GET to Destination Node %d", dest_device_id);    
}

void set_association_config(uint8_t dest_keypad_id, uint8_t group_no, uint8_t dest_zwave_id, uint8_t dest_ep){
    /*Association Set 
    Send Command : 0x01 0x0D 0x00 0x13 0x64 0x06 0x8E 0x01 0x02 0x00 0x65 0x01 0x25 0x01 0x4E  
    01 - SOF  
    0D - LENGTH  
    00 - REQUEST  
    13 - ZW_SEND_DATA  
    64 - DESTINATION = REMOTE NODE  
    06 - PAYLOAD LENGTH  
    8E - COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION  
    01 - MULTI_CHANNEL_ASSOCIATION_SET  
    02 - GROUP(2)  
    00 - MULTI_CHANNEL_ASSOCIATION_SET_MARKER  
    65 - DESTINATION = POWER NODE  
    01 - ENDPOINT  
    25 - TX OPTIONS  
    01 - CALLBACK ID  
    4E - CHECKSUM */

    uint8_t frame[] = {
        SOF, 0x0D, REQUEST, FUNC_ID_ZW_SEND_DATA,
        dest_keypad_id, 0x06,
        COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION, MULTI_CHANNEL_ASSOCIATION_SET,
        group_no+1, MULTI_CHANNEL_ASSOCIATION_SET_MARKER, 
        dest_zwave_id, dest_ep, TX_OPTIONS,
        0x01, 0x00
    };

    frame[14] = calculate_checksum(&frame[1], 13);
    uart_write_bytes(UART_PORT, (char *)frame, sizeof(frame));

    ESP_LOGI(TAG_ASSOCIATION, "Sent Association Set to Remote Node %d and Power Node %d", dest_keypad_id, dest_zwave_id);
}

void get_association_config(uint8_t dest_keypad_id, uint8_t group_no){
    /*Association Get 
    Send Command : 0x01 0x0A 0x00 0x13 0x67 0x03 0x8E 0x02 0x02 0x25 0x01 0x28 
    01 - SOF
    0A - LENGTH  
    00 - REQUEST  
    13 - ZW_SEND_DATA  
    67 - DESTINATION = REMOTE NODE  
    03 - PAYLOAD LENGTH  
    8E - COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION  
    02 - MULTI_CHANNEL_ASSOCIATION_ GET 
    02 - GROUP(2)  
    25 - TX OPTIONS  
    01 - CALLBACK ID  
    28 - CHECKSUM*/

    uint8_t frame[] = {
        SOF, 0x0A, REQUEST, FUNC_ID_ZW_SEND_DATA,
        dest_keypad_id, 0x03,
        COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION, MULTI_CHANNEL_ASSOCIATION_GET,
        group_no+1, TX_OPTIONS,
        0x01, 0x00
    };

    frame[11] = calculate_checksum(&frame[1], 10);
    uart_write_bytes(UART_PORT, (char *)frame, sizeof(frame));

    ESP_LOGI(TAG_ASSOCIATION, "Sent Association Get to Remote Node %d", dest_keypad_id);
}

void remove_association_config(uint8_t dest_keypad_id, uint8_t group_no){
    /*Association Remove 
    Send Command : 0x01 0x0A 0x00 0x13 0x03 0x03 0x8E 0x04 0x02 0x25 0x01 0x4A 
    01 - SOF  
    0A - LENGTH  
    00 - REQUEST  
    13 - ZW_SEND_DATA  
    03 - DESTINATION = REMOTE NODE  
    03 - PAYLOAD LENGTH  
    8E - COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION  
    04 - MULTI_CHANNEL_ASSOCIATION_REMOVE  
    02 - GROUP(2)
    25 - TX OPTIONS  
    01 - CALLBACK ID  
    4A - CHECKSUM */

    uint8_t frame[] = {
        SOF, 0x0A, REQUEST, FUNC_ID_ZW_SEND_DATA,
        dest_keypad_id, 0x03,
        COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION, MULTI_CHANNEL_ASSOCIATION_REMOVE,
        group_no+1, TX_OPTIONS,
        0x01, 0x00
    };

    frame[11] = calculate_checksum(&frame[1], 10);
    uart_write_bytes(UART_PORT, (char *)frame, sizeof(frame));

    ESP_LOGI(TAG_ASSOCIATION, "Sent Association Remote to Remote Node %d", dest_keypad_id);
}


static esp_err_t status_get_handler(httpd_req_t *req)
{
    char response[256];

    if (web_status_mutex &&
        xSemaphoreTake(web_status_mutex, pdMS_TO_TICKS(100))) {

        strncpy(response, web_status, sizeof(response));
        xSemaphoreGive(web_status_mutex);

    } else {
        strcpy(response, "BUSY");
    }

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// ==================== WEB SERVER HANDLERS ====================
static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html, strlen(index_html));
    return ESP_OK;
}

// Handler for saving parameter configuration
static esp_err_t save_param_set_post_handler(httpd_req_t *req) {
    char buf[256];
    int ret, remaining = req->content_len;
    int total_read = 0;
    
    // Check content length
    if (remaining > sizeof(buf) - 1) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Content too large\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // Read POST data
    while (remaining > 0) {
        ret = httpd_req_recv(req, buf + total_read, remaining);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, "{\"error\":\"Failed to read data\"}", HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }
        total_read += ret;
        remaining -= ret;
    }
    buf[total_read] = '\0';
    
    ESP_LOGI(TAG_WEB, "Received parameter SET config: %s", buf);
    
    // Parse JSON
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Invalid JSON\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // Extract parameters
    cJSON *deviceId = cJSON_GetObjectItem(root, "deviceId");
    cJSON *paramNum = cJSON_GetObjectItem(root, "paramNum");
    cJSON *paramVal = cJSON_GetObjectItem(root, "paramVal");
    
    if (!cJSON_IsString(deviceId) || !cJSON_IsString(paramNum) || !cJSON_IsString(paramVal)) {
        cJSON_Delete(root);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Missing parameters\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // Convert string to uint8_t
    uint8_t dest_device_id = (uint8_t)atoi(deviceId->valuestring);
    uint8_t param_num = (uint8_t)atoi(paramNum->valuestring);
    uint8_t param_value = (uint8_t)atoi(paramVal->valuestring);
    
    ESP_LOGI(TAG_WEB, "Parameter config - Device ID: %u, Param: %u, Value: %u", 
             dest_device_id, param_num, param_value);
    blink_zwave_led();
    set_parameter_config(dest_device_id, param_num, param_value);
    cJSON_Delete(root);
    // Send success response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"success\"}", HTTPD_RESP_USE_STRLEN);
    
    return ESP_OK;
}

// Handler for saving parameter configuration
static esp_err_t save_param_get_post_handler(httpd_req_t *req) {
    char buf[256];
    int ret, remaining = req->content_len;
    int total_read = 0;
    
    // Check content length
    if (remaining > sizeof(buf) - 1) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Content too large\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // Read POST data
    while (remaining > 0) {
        ret = httpd_req_recv(req, buf + total_read, remaining);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, "{\"error\":\"Failed to read data\"}", HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }
        total_read += ret;
        remaining -= ret;
    }
    buf[total_read] = '\0';
    
    ESP_LOGI(TAG_WEB, "Received parameter GET config: %s", buf);
    
    // Parse JSON
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Invalid JSON\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // Extract parameters
    cJSON *deviceId = cJSON_GetObjectItem(root, "deviceId");
    cJSON *paramNum = cJSON_GetObjectItem(root, "paramNum");
    
    if (!cJSON_IsString(deviceId) || !cJSON_IsString(paramNum)) {
        cJSON_Delete(root);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Missing parameters\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // Convert string to uint8_t
    uint8_t dest_device_id = (uint8_t)atoi(deviceId->valuestring);
    uint8_t param_num = (uint8_t)atoi(paramNum->valuestring);
    
    ESP_LOGI(TAG_WEB, "Parameter config - Device: %u, Param: %u", 
                dest_device_id, param_num);
    blink_zwave_led();
    get_parameter_config(dest_device_id, param_num);
    
    cJSON_Delete(root);
    
    // Send success response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"success\"}", HTTPD_RESP_USE_STRLEN);
    
    return ESP_OK;
}

// Handler for saving association configuration
static esp_err_t save_assoc_set_post_handler(httpd_req_t *req) {
    char buf[256];
    int ret, remaining = req->content_len;
    int total_read = 0;
    
    // Check content length
    if (remaining > sizeof(buf) - 1) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Content too large\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // Read POST data
    while (remaining > 0) {
        ret = httpd_req_recv(req, buf + total_read, remaining);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, "{\"error\":\"Failed to read data\"}", HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }
        total_read += ret;
        remaining -= ret;
    }
    buf[total_read] = '\0';
    
    ESP_LOGI(TAG_ASSOCIATION, "Received association SET config: %s", buf);
    
    // Parse JSON
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Invalid JSON\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // Extract parameters
    cJSON *keypadId = cJSON_GetObjectItem(root, "keypadId");
    cJSON *keypadBtn = cJSON_GetObjectItem(root, "keypadBtn");
    cJSON *zwaveId = cJSON_GetObjectItem(root, "zwaveId");
    cJSON *endpoint = cJSON_GetObjectItem(root, "endpoint");
    
    if (!cJSON_IsString(keypadId) || !cJSON_IsString(keypadBtn) || 
        !cJSON_IsString(zwaveId) || !cJSON_IsString(endpoint)) {
        cJSON_Delete(root);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Missing parameters\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // Convert strings to uint8_t
    uint8_t dest_keypad_id = (uint8_t)atoi(keypadId->valuestring);
    uint8_t button = (uint8_t)atoi(keypadBtn->valuestring);
    uint8_t dest_zwave_id = (uint8_t)atoi(zwaveId->valuestring);
    uint8_t ep = (uint8_t)atoi(endpoint->valuestring);
    
    ESP_LOGI(TAG_ASSOCIATION, "Association config - Keypad ID: %u, Button: %u, Z-Wave ID: %u, Endpoint: %u",
             dest_keypad_id, button, dest_zwave_id, ep);
    blink_zwave_led();
    set_association_config(dest_keypad_id, button, dest_zwave_id, ep);
    cJSON_Delete(root);
    
    // Send success response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"success\"}", HTTPD_RESP_USE_STRLEN);
    
    return ESP_OK;
}

// Handler for saving association configuration
static esp_err_t save_assoc_get_post_handler(httpd_req_t *req) {
    char buf[256];
    int ret, remaining = req->content_len;
    int total_read = 0;
    
    // Check content length
    if (remaining > sizeof(buf) - 1) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Content too large\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // Read POST data
    while (remaining > 0) {
        ret = httpd_req_recv(req, buf + total_read, remaining);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, "{\"error\":\"Failed to read data\"}", HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }
        total_read += ret;
        remaining -= ret;
    }
    buf[total_read] = '\0';
    
    ESP_LOGI(TAG_ASSOCIATION, "Received association GET config: %s", buf);
    
    // Parse JSON
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Invalid JSON\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // Extract parameters
    cJSON *keypadId = cJSON_GetObjectItem(root, "keypadId");
    cJSON *keypadBtn = cJSON_GetObjectItem(root, "keypadBtn");
    
    if (!cJSON_IsString(keypadId) || !cJSON_IsString(keypadBtn)) {
        cJSON_Delete(root);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Missing parameters\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // Convert strings to uint8_t
    uint8_t dest_keypad_id = (uint8_t)atoi(keypadId->valuestring);
    uint8_t button = (uint8_t)atoi(keypadBtn->valuestring);
   // uint8_t dest_zwave_id = (uint8_t)atoi(zwaveId->valuestring);
   // uint8_t ep = (uint8_t)atoi(endpoint->valuestring);
    
    ESP_LOGI(TAG_ASSOCIATION, "Association config - Keypad ID: %u, Button: %u",
             dest_keypad_id, button);
    blink_zwave_led();
    get_association_config(dest_keypad_id, button);
    cJSON_Delete(root);
    
    // Send success response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"success\"}", HTTPD_RESP_USE_STRLEN);
    
    return ESP_OK;
}

static esp_err_t save_assoc_remove_post_handler(httpd_req_t *req) {
    char buf[256];
    int ret, remaining = req->content_len;
    int total_read = 0;
    
    // Check content length
    if (remaining > sizeof(buf) - 1) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Content too large\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // Read POST data
    while (remaining > 0) {
        ret = httpd_req_recv(req, buf + total_read, remaining);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, "{\"error\":\"Failed to read data\"}", HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }
        total_read += ret;
        remaining -= ret;
    }
    buf[total_read] = '\0';
    
    ESP_LOGI(TAG_ASSOCIATION, "Received association REMOVE config: %s", buf);
    
    // Parse JSON
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Invalid JSON\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // Extract parameters
    cJSON *keypadId = cJSON_GetObjectItem(root, "keypadId");
    cJSON *keypadBtn = cJSON_GetObjectItem(root, "keypadBtn");
    
    if (!cJSON_IsString(keypadId) || !cJSON_IsString(keypadBtn)) {
        cJSON_Delete(root);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"error\":\"Missing parameters\"}", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // Convert strings to uint8_t
    uint8_t dest_keypad_id = (uint8_t)atoi(keypadId->valuestring);
    uint8_t button = (uint8_t)atoi(keypadBtn->valuestring);
    
    ESP_LOGI(TAG_ASSOCIATION, "Association config - Keypad ID: %u, Button: %u",
             dest_keypad_id, button);
    blink_zwave_led();
    remove_association_config(dest_keypad_id, button);
    cJSON_Delete(root);
    
    // Send success response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"success\"}", HTTPD_RESP_USE_STRLEN);
    
    return ESP_OK;
}

// Handler for clear wifi credentials
static esp_err_t clear_wifi_post_handler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;
    
    // Read POST data if any
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }
    
    ESP_LOGI(TAG, "Wi-Fi reset requested via web");
    
    // Send immediate response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"reset_started\"}", HTTPD_RESP_USE_STRLEN);
    
    // Delay to allow response to be sent
    vTaskDelay(pdMS_TO_TICKS(100));
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("wifi_config", NVS_READWRITE, &nvs);
    if (err == ESP_OK) {
        nvs_erase_all(nvs);
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGI(TAG, "WiFi credentials cleared");
    }
    vTaskDelay(500);
    esp_restart();
    return ESP_OK;
}

// Handler for Z-Wave Network Reset
static esp_err_t zwave_reset_post_handler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;
    
    // Read POST data if any
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }
    
    ESP_LOGI(TAG, "Z-Wave network reset requested via web");
    TURN_OFF_ZWAVE_LED
    // Send immediate response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"reset_started\"}", HTTPD_RESP_USE_STRLEN);
    
    // Delay to allow response to be sent
    vTaskDelay(pdMS_TO_TICKS(100));
    reset_zwave();
    vTaskDelay(500);
    esp_restart();
    return ESP_OK;
}
// Handler for soft reset
static esp_err_t soft_reset_post_handler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;
    
    // Read POST data if any
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }
    
    ESP_LOGI(TAG, "Soft reset requested via web");
    TURN_OFF_ZWAVE_LED
    // Send immediate response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"reset_started\"}", HTTPD_RESP_USE_STRLEN);
    
    // Delay to allow response to be sent
    vTaskDelay(pdMS_TO_TICKS(100));
    nvs_clear_all_nodes();
    vTaskDelay(100);
    reset_zwave();
    vTaskDelay(500);
    esp_restart();
    
    return ESP_OK;
}

// Handler for hard reset
static esp_err_t hard_reset_post_handler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;
    
    // Read POST data if any
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }
    
    ESP_LOGI(TAG, "Hard reset requested via web");
    TURN_OFF_WIFI_LED
    TURN_OFF_ZWAVE_LED
    // This should clear all configurations, nodes, and factory reset
    
     // 1. Clear all nodes from NVS
     nvs_clear_all_nodes();
    
    // 2. Clear WiFi credentials
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("wifi_config", NVS_READWRITE, &nvs);
    if (err == ESP_OK) {
        nvs_erase_all(nvs);
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGI(TAG, "WiFi credentials cleared");
    }
    // Send response
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"hard_reset_complete\"}", HTTPD_RESP_USE_STRLEN);
    
    // Delay to allow response to be sent
    vTaskDelay(pdMS_TO_TICKS(100));
    
    reset_zwave();
    vTaskDelay(500);
    esp_restart();
    
    return ESP_OK;
}

static esp_err_t inclusion_post_handler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;
    
    // Read POST data if any
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }
    
    ESP_LOGI(TAG, "Inclusion process started via web");
    
    // Check if Z-Wave is ready
    if (zw_state != ZW_IDLE) {
        const char *response = "Z-Wave controller not ready. Current state: ";
        char full_response[100];
        sprintf(full_response, "%s%d", response, zw_state);
        httpd_resp_send(req, full_response, strlen(full_response));
        return ESP_OK;
    }
    
    // // Start inclusion process
    // zw_start_inclusion();
    zw_start_inclusion_async();   // 🔥 non-blocking
    // Return immediate response
    const char *response = "Inclusion process started. Put Z-Wave device in inclusion mode within 60 seconds.";
   // const char *response = "Inclusion process started. Please put your Z-Wave device in inclusion mode within 60 seconds.";
    httpd_resp_send(req, response, strlen(response));
    show_Online_msg = false;
    send_msg_to_web(response);
    return ESP_OK;
}

static esp_err_t exclusion_post_handler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;
    
    // Read POST data if any
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }
    
    ESP_LOGI(TAG, "Exclusion process started via web");
    
    // Start exclusion process
    //zw_start_exclusion();
    zw_start_exclusion_async();
    // Return immediate response
    const char *response = "Exclusion process started. Put device in exclusion mode. "
                          "The device will be removed automatically when exclusion completes.";
    httpd_resp_send(req, response, strlen(response));
    show_Online_msg = false;
    send_msg_to_web(response);
    return ESP_OK;
}

/* URI handlers */
static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t inclusion = {
    .uri       = "/inclusion",
    .method    = HTTP_POST,
    .handler   = inclusion_post_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t exclusion = {
    .uri       = "/exclusion",
    .method    = HTTP_POST,
    .handler   = exclusion_post_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t status = {
    .uri       = "/status",
    .method    = HTTP_GET,
    .handler   = status_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t saveParamSet = {
    .uri       = "/saveParamSet",
    .method    = HTTP_POST,
    .handler   = save_param_set_post_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t saveParamGet = {
    .uri       = "/saveParamGet",
    .method    = HTTP_POST,
    .handler   = save_param_get_post_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t saveAssocSet = {
    .uri       = "/saveAssocSet",
    .method    = HTTP_POST,
    .handler   = save_assoc_set_post_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t saveAssocGet = {
    .uri       = "/saveAssocGet",
    .method    = HTTP_POST,
    .handler   = save_assoc_get_post_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t saveAssocRemove = {
    .uri       = "/saveAssocRemove",
    .method    = HTTP_POST,
    .handler   = save_assoc_remove_post_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t clearWifi = {
    .uri       = "/clearWifi",
    .method    = HTTP_POST,
    .handler   = clear_wifi_post_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t zwaveNetworkReset = {
    .uri       = "/zwaveNetworkReset",
    .method    = HTTP_POST,
    .handler   = zwave_reset_post_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t softReset = {
    .uri       = "/softReset",
    .method    = HTTP_POST,
    .handler   = soft_reset_post_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t hardReset = {
    .uri       = "/hardReset",
    .method    = HTTP_POST,
    .handler   = hard_reset_post_handler,
    .user_ctx  = NULL
};
// ==================== WEB SERVER INITIALIZATION ====================
httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    config.stack_size = 8192;
    config.max_uri_handlers = 20;
    config.lru_purge_enable = true;
    
    ESP_LOGI(TAG, "Starting HTTP server on port: %d", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &status);
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &inclusion);
        httpd_register_uri_handler(server, &exclusion);
        httpd_register_uri_handler(server, &saveParamSet);
        httpd_register_uri_handler(server, &saveParamGet);
        httpd_register_uri_handler(server, &saveAssocSet);
        httpd_register_uri_handler(server, &saveAssocGet);
        httpd_register_uri_handler(server, &saveAssocRemove);
        httpd_register_uri_handler(server, &clearWifi);
        httpd_register_uri_handler(server, &zwaveNetworkReset);
        httpd_register_uri_handler(server, &softReset);
        httpd_register_uri_handler(server, &hardReset);
        ESP_LOGI(TAG, "HTTP server started successfully");
        return server;
    }
    
    ESP_LOGE(TAG, "Error starting HTTP server!");
    return NULL;
}


/* ISR */
static void IRAM_ATTR button_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void button_task(void *arg)
{
    uint32_t gpio_num;
    uint32_t current_time;
    
    while (1) {
        if (xQueueReceive(gpio_evt_queue, &gpio_num, portMAX_DELAY)) {
            current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            
            // Debounce
            vTaskDelay(pdMS_TO_TICKS(50));
            
            // Check if button is still pressed after debounce
            if (gpio_get_level(BUTTON_GPIO) == 0) { // Assuming active LOW
                ESP_LOGI(TAGBUTTON, "Button pressed on GPIO %lu", gpio_num);
                
                // Check if this is part of a multi-press sequence
                if (current_time - last_press_time < MULTI_PRESS_TIMEOUT_MS) {
                    button_press_count++;
                    ESP_LOGI(TAGBUTTON, "Multi-press count: %lu", button_press_count);
                    
                    // Check if threshold reached
                    if (button_press_count == MULTI_PRESS_THRESHOLD) {
                        ESP_LOGI(TAGBUTTON, "=== MULTI-PRESS DETECTED ===");
                        ESP_LOGI(TAGBUTTON, "Button pressed %d times in sequence!", MULTI_PRESS_THRESHOLD);
                        ESP_LOGI(TAGBUTTON, "===========================");
                        TURN_OFF_WIFI_LED
                        //Clear WiFi credentials
                        nvs_handle_t nvs;
                        esp_err_t err = nvs_open("wifi_config", NVS_READWRITE, &nvs);
                        if (err == ESP_OK) {
                            nvs_erase_all(nvs);
                            nvs_commit(nvs);
                            nvs_close(nvs);
                            ESP_LOGI(TAG, "WiFi credentials cleared");
                        }
                        vTaskDelay(500);
                        esp_restart();
                        // Reset counter
                        button_press_count = 0;
                    }
                } else {
                    // New press sequence
                    button_press_count = 1;
                    ESP_LOGI(TAGBUTTON, "New press sequence started");
                }
                
                last_press_time = current_time;
            }
        }
    }
}

void blink_zwave_led_task(void *pvParameter)
{
    while (1) {
        TURN_ON_ZWAVE_LED
        vTaskDelay(300 / portTICK_PERIOD_MS);
        TURN_OFF_ZWAVE_LED
        vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}

void init_button_isr(){
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE   // falling edge = press
    };
    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, (void *) BUTTON_GPIO);    
}

void init_led(){
    gpio_config_t io_conf_led = { .pin_bit_mask = (1ULL << WIFI_LED) | (1ULL << ZWAVE_LED), 
        .mode = GPIO_MODE_OUTPUT, 
        .pull_up_en = GPIO_PULLUP_DISABLE, 
        .pull_down_en = GPIO_PULLDOWN_DISABLE, 
        .intr_type = GPIO_INTR_DISABLE 
    }; 
    gpio_config(&io_conf_led); 
    // Initial states to Off
    TURN_OFF_WIFI_LED
    TURN_OFF_ZWAVE_LED

}
// ==================== MAIN APPLICATION ====================
void app_main(void) {
    ESP_LOGI(TAG, "Starting Jaquar Remote Configuration Application");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize watchdog timer
    task_watchdog_timer = xTimerCreate("watchdog", pdMS_TO_TICKS(5000), pdTRUE, 
                                      (void *)0, system_watchdog_callback);
    if (task_watchdog_timer) {
        xTimerStart(task_watchdog_timer, 0);
        ESP_LOGI(TAG, "Watchdog timer started");
    }
    
    // Initialize UART
    init_uart();

    init_button_isr();
    /* Create one-shot timer */
    esp_timer_create_args_t timer_args = {
        .callback = &web_msg_timer_cb,
        .name = "web_msg_5s_timer"
    };
    esp_timer_create(&timer_args, &web_msg_timer);
    web_status_mutex = xSemaphoreCreateMutex();
    //show_Online_msg = true;
    // Start UART RX task immediately
    xTaskCreate(zw_uart_rx_task, "zw_rx", 8192, NULL, 5, NULL);
    
    // Wait for Z-Wave module to boot
    ESP_LOGI(TAG, "Waiting for Z-Wave module to initialize...");
    xTaskCreate(&blink_zwave_led_task, "Setting up Zwave S_API", 2048, NULL, 5, &blink_zwave_led_handle);
    for (int i = 0; i < 30; i++) {  // 3 seconds total
        kick_watchdog();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(
        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL)
    );

    ESP_ERROR_CHECK(
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL)
    );
    init_led();
    send_msg_to_web("Gateway Status : Loading...");
    ESP_LOGI(TAG, "\n\n");
    ESP_LOGI(TAG, "========================================================");
    ESP_LOGI(TAG, "STARTING Z-WAVE SERIAL API INITIALIZATION SEQUENCE");
    ESP_LOGI(TAG, "========================================================");
    
    // Wait for any pending responses first
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // 1. Get Controller Capabilities
    ESP_LOGI(TAG, "\n[1] Getting controller capabilities...");
    zw_get_capabilities();
    vTaskDelay(pdMS_TO_TICKS(200));  // Wait for response
    
    // 2. Get SerialAPI Supported Command Classes
    ESP_LOGI(TAG, "\n[2] Getting SerialAPI supported command classes...");
    zw_get_serialapi_supported();
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // 3. Get SUC Node ID
    ESP_LOGI(TAG, "\n[3] Getting SUC Node ID...");
    zw_get_suc_node_id();
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // 4. Get Z-Wave Library Version
    ESP_LOGI(TAG, "\n[4] Getting Z-Wave library version...");
    zw_get_version();
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // 5. Get Memory ID (Home ID and Node ID) - Already got this from startup
    ESP_LOGI(TAG, "\n[5] Getting Home ID and Node ID...");
    send_memory_get_id();
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // 7. Set TX Power (adjust based on response)
    ESP_LOGI(TAG, "\n[7] Setting TX power...");
    // Try different values if 0x7F fails
    zw_set_tx_power(0x7F, 0x00);  // 0x32 = 0dBm
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 9. Set RF Region to India (only if not already India)
    ESP_LOGI(TAG, "\n[9] Setting RF region to India...");
    zw_set_rf_region(RF_REGION_INDIA);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 10. Verify RF Region
    ESP_LOGI(TAG, "\n[10] Verifying RF region...");
    zw_get_rf_region();
    vTaskDelay(pdMS_TO_TICKS(200));
    
    ESP_LOGI(TAG, "========================================================");
    ESP_LOGI(TAG, "✓ Z-WAVE CONTROLLER INITIALIZATION COMPLETE");
    ESP_LOGI(TAG, "========================================================");
    ESP_LOGI(TAG, "\n\n");
    vTaskDelete(blink_zwave_led_handle);
    TURN_ON_ZWAVE_LED
    interview_queue = xQueueCreate(10, sizeof(interview_msg_t));
    xTaskCreate(zw_interview_task, "zw_interview", 4096, NULL, 5, NULL); 
    
    // Check which mode to start in
    if (should_use_sta_mode()) {
        ESP_LOGI(TAG_WIFI, "Starting in STA mode with saved credentials");
        wifi_init_sta();
    } else {
        ESP_LOGI(TAG_WIFI, "Starting in AP mode for configuration");
        wifi_init_softap();
    }
    nvs_load_node_table();
    print_node_table();
    ESP_LOGI(TAG, "\n\n");
    ESP_LOGI(TAG, "========================================================");
    ESP_LOGI(TAG, "APPLICATION STARTED SUCCESSFULLY");
    ESP_LOGI(TAG, "========================================================");
    ESP_LOGI(TAG, "Z-Wave Controller: READY");
    ESP_LOGI(TAG, "========================================================");
    ESP_LOGI(TAG, "\n\n");

    // Main loop
    while (1) {
        kick_watchdog();
        
        // Periodically log system status
        static uint32_t last_log = 0;
        uint32_t now = esp_log_timestamp();
        if (now - last_log > 30000) {  // Every 30 seconds
            last_log = now;
            
            // Print all nodes information
            if (g_node_count > 0) {
                ESP_LOGI(TAG, "=== CURRENT NODES (%d) ===", g_node_count);
                for (int i = 0; i < g_node_count; i++) {
                    ESP_LOGI(TAG,
                        "Node %d: %-14s NIF:%d bytes",
                        g_included_nodes[i].node_id,
                        dev_type_str(g_included_nodes[i].dev_type),
                        g_included_nodes[i].nif_len
                    );
                }
                ESP_LOGI(TAG, "=====================");
            } else {
                ESP_LOGI(TAG, "=== NO NODES IN NETWORK ===");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(4000));
    }
}