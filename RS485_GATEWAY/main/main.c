#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "esp_random.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_system.h" 
#include "esp_log.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "mdns.h"
#include "esp_mac.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_http_server.h"
#include <ctype.h>
#include <errno.h>
#include "driver/spi_master.h"
#include "esp_eth.h"
#include "driver/spi_common.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <inttypes.h>
#include "esp_crc.h"

#define RS485_MDNS_HOSTNAME "confiors485"
#define TCP_PORT 5000
#define BUFFER_SIZE 256
#define DEVICE_ID_SIZE 8     // First 8 bytes of DeviceRxBuff are device ID
#define UART_NUM UART_NUM_1
#define TXD_PIN GPIO_NUM_37
#define RXD_PIN GPIO_NUM_36
#define BAUD_RATE 9600
#define RS485_WRITE_PIN GPIO_NUM_3
#define RS485_READ_PIN GPIO_NUM_2
#define ETH_SPI_HOST          SPI2_HOST
#define ETH_SPI_SCLK_GPIO     12
#define ETH_SPI_MOSI_GPIO     11
#define ETH_SPI_MISO_GPIO     13
#define ETH_SPI_CS_GPIO       10
#define ETH_SPI_INT_GPIO      14
#define ETH_SPI_RST_GPIO      9
#define ETH_SPI_CLOCK_MHZ     5

#define MAX_ENTRIES 100
#define ENTRY_KEY_SIZE 12
#define ENTRY_VALUE_SIZE 14
uint8_t *accumulated_buffer = NULL;
int accumulated_len = 0;
bool processing_scheduled = false;

uint8_t rxBuff_from_device[254];
uint8_t send_rs485_bytes[32];
TaskHandle_t active_client_task = NULL;
int active_client_sock = -1;
static const char *TAG_RS485_APP = "RS485_APP";
static const char *TAG_RS485_DATA = "RS485_DATA";
static const char *TAG_C4_DATA = "RS485_C4_DATA";
static const char *TAG_NVS = "RS485_NVS";

void process_rs485_data(uint8_t *DeviceRxBuff, int len);
extern void print_ip_address();
extern void start_mdns_service();
extern void init_client_list();
extern void tcp_server_task(void *pvParameters);
void store_to_nvm(uint8_t *key, uint8_t *value);
uint8_t* lookup_from_nvm(uint8_t *key);
bool delete_key_from_nvm(uint8_t *key, size_t key_len);

wifi_config_t wifi_config = {0};


typedef struct {
    uint8_t key[ENTRY_KEY_SIZE];
    uint8_t value[ENTRY_VALUE_SIZE];
    bool valid;
} NvmEntry;

NvmEntry nvm_table[MAX_ENTRIES] = {0};

bool nvm_read(uint8_t *key, size_t key_len, uint8_t **value, size_t *value_len) {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (nvm_table[i].valid && memcmp(nvm_table[i].key, key, key_len) == 0) {
            *value = nvm_table[i].value;
            *value_len = ENTRY_VALUE_SIZE;
            return true;
        }
    }
    return false;
}

bool nvm_delete(uint8_t *key, size_t key_len) {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (nvm_table[i].valid && memcmp(nvm_table[i].key, key, key_len) == 0) {
            nvm_table[i].valid = false;
            return true;
        }
    }
    return false;
}

void compact_nvm_table() {
    int next_free = 0;

    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (nvm_table[i].valid) {
            if (i != next_free) {
                // Move entry
                memcpy(&nvm_table[next_free], &nvm_table[i], sizeof(NvmEntry));
                nvm_table[i].valid = false;
            }
            next_free++;
        }
    }
}


void key_to_short_str(uint8_t *key, char *out_str) {
    uint32_t crc = esp_crc32_le(0, key, ENTRY_KEY_SIZE);
    sprintf(out_str, "%08" PRIX32, crc);
}

void rebuild_nvm_table_from_flash() {
    nvs_iterator_t it = NULL;
    esp_err_t err = nvs_entry_find("nvs", "custom_nvm", NVS_TYPE_BLOB, &it);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_NVS, "No NVS entries found or error: %s\n", esp_err_to_name(err));
        return;
    }

    nvs_handle_t nvs;
    err = nvs_open("custom_nvm", NVS_READONLY, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_NVS, "Failed to open NVS: %s\n", esp_err_to_name(err));
        return;
    }

    int index = 0;
    while (it != NULL && index < MAX_ENTRIES) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);

        uint8_t blob[ENTRY_KEY_SIZE + ENTRY_VALUE_SIZE];
        size_t size = sizeof(blob);

        err = nvs_get_blob(nvs, info.key, blob, &size);
        if (err == ESP_OK && size == sizeof(blob)) {
            memcpy(nvm_table[index].key, blob, ENTRY_KEY_SIZE);
            memcpy(nvm_table[index].value, blob + ENTRY_KEY_SIZE, ENTRY_VALUE_SIZE);
            nvm_table[index].valid = true;
            index++;
        }

        err = nvs_entry_next(&it);
        if (err != ESP_OK) {
            break;
        }
    }

    nvs_close(nvs);
    if (it) {
        nvs_release_iterator(it);
    }
}


bool delete_key_from_nvm(uint8_t *key, size_t key_len) {
    nvs_iterator_t it = NULL;
    esp_err_t err = nvs_entry_find("nvs", "custom_nvm", NVS_TYPE_BLOB, &it);
    if (err != ESP_OK) return false;

    nvs_handle_t nvs;
    if (nvs_open("custom_nvm", NVS_READWRITE, &nvs) != ESP_OK) {
        nvs_release_iterator(it);
        return false;
    }

    while (it != NULL) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);

        uint8_t blob[ENTRY_KEY_SIZE + ENTRY_VALUE_SIZE];
        size_t size = sizeof(blob);

        if (nvs_get_blob(nvs, info.key, blob, &size) == ESP_OK && size == sizeof(blob)) {
            if (memcmp(blob, key, key_len) == 0) {
                err = nvs_erase_key(nvs, info.key);
                nvs_commit(nvs);
                nvs_close(nvs);
                nvs_release_iterator(it);
                return (err == ESP_OK);
            }
        }

        err = nvs_entry_next(&it);
        if (err != ESP_OK) break;
    }

    nvs_close(nvs);
    nvs_release_iterator(it);
    return false;
}




void store_to_nvm(uint8_t *key, uint8_t *value) {
    char key_str[9];  // 8 chars + null
    key_to_short_str(key, key_str);

    uint8_t blob[ENTRY_KEY_SIZE + ENTRY_VALUE_SIZE];
    memcpy(blob, key, ENTRY_KEY_SIZE);
    memcpy(blob + ENTRY_KEY_SIZE, value, ENTRY_VALUE_SIZE);

    nvs_handle_t nvs;
    esp_err_t err = nvs_open("custom_nvm", NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_NVS, "Failed to open NVS: %s\n", esp_err_to_name(err));
        return;
    }

    err = nvs_set_blob(nvs, key_str, blob, sizeof(blob));
    if (err == ESP_OK) {
        err = nvs_commit(nvs);
        if (err == ESP_OK) {
            ESP_LOGI(TAG_NVS,"Saved key %s to NVS\n", key_str);
        } else {
            ESP_LOGE(TAG_NVS,"Failed to commit key %s: %s\n", key_str, esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG_NVS,"Failed to set blob for key %s: %s\n", key_str, esp_err_to_name(err));
    }

    nvs_close(nvs);

    // Update in-memory table
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (!nvm_table[i].valid) {
            memcpy(nvm_table[i].key, key, ENTRY_KEY_SIZE);
            memcpy(nvm_table[i].value, value, ENTRY_VALUE_SIZE);
            nvm_table[i].valid = true;
            break;
        }
    }
}

uint8_t* lookup_from_nvm(uint8_t *key) {
    static uint8_t value[ENTRY_VALUE_SIZE];
    nvs_iterator_t it = NULL;

    if (nvs_entry_find("nvs", "custom_nvm", NVS_TYPE_BLOB, &it) != ESP_OK) {
        return NULL;
    }

    nvs_handle_t nvs;
    if (nvs_open("custom_nvm", NVS_READONLY, &nvs) != ESP_OK) {
        nvs_release_iterator(it);
        return NULL;
    }

    while (it != NULL) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);

        uint8_t blob[ENTRY_KEY_SIZE + ENTRY_VALUE_SIZE];
        size_t size = sizeof(blob);

        if (nvs_get_blob(nvs, info.key, blob, &size) == ESP_OK && size == sizeof(blob)) {
            if (memcmp(blob, key, ENTRY_KEY_SIZE) == 0) {
                memcpy(value, blob + ENTRY_KEY_SIZE, ENTRY_VALUE_SIZE);
                nvs_close(nvs);
                nvs_release_iterator(it);
                return value;
            }
        }

        if (nvs_entry_next(&it) != ESP_OK) {
            break;
        }
    }

    nvs_close(nvs);
    nvs_release_iterator(it);
    return NULL;
}

// Function to initialize UART
void init_uart() {
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    if (uart_driver_install(UART_NUM, 256, 256, 0, NULL, 0) != ESP_OK) {
        ESP_LOGE(TAG_RS485_APP, "UART driver install failed!");
        return;
    }

    uart_param_config(UART_NUM, &uart_config);
    if (uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
        ESP_LOGE(TAG_RS485_APP, "UART pin config failed!");
    }

    gpio_set_direction(RS485_WRITE_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(RS485_READ_PIN, GPIO_MODE_OUTPUT);

    gpio_set_pull_mode(RS485_WRITE_PIN, GPIO_PULLUP_ONLY);
    gpio_set_level(RS485_WRITE_PIN, 1);
    
    gpio_set_level(RS485_READ_PIN, 0);
    uart_set_mode(UART_NUM, UART_MODE_RS485_HALF_DUPLEX);
}

// Function to send RS485 data to devices
void send_rs485_to_device(uint8_t *data, size_t length) {
    // Send data
 //   ESP_LOGI(TAG_RS485_DATA, "Sending data to RS485 device: ");
    /*for (size_t i = 0; i < length; ++i) {
        printf("%02X ", data[i]);
    }
    printf("\n");*/
    //vTaskDelay(pdMS_TO_TICKS(10));
    uart_write_bytes(UART_NUM, (const char *)data, length);
    vTaskDelay(pdMS_TO_TICKS(1));
}

// Initialize NVS
void init_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

// Function to calculate Modbus CRC-16
uint16_t calculate_modbus_crc(uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

uint16_t random_n_to_m(uint16_t n, uint16_t m) {
    return n + (esp_random() % (m - n + 1));
}

void encryptFrameandSend(uint8_t *originalFrame, uint8_t *encryptedFrame) {
    // Encrypt fixed positions (0 to 7)
    encryptedFrame[0] = originalFrame[0] + 9;
    encryptedFrame[1] = originalFrame[1] - 4;
    encryptedFrame[2] = originalFrame[2] + 5;
    encryptedFrame[3] = originalFrame[3] - 2;
    encryptedFrame[4] = originalFrame[4] + 1;
    encryptedFrame[5] = originalFrame[5] - 7;
    encryptedFrame[6] = originalFrame[6] + 9;
    encryptedFrame[7] = originalFrame[7] - 2;

    uint8_t type0 = originalFrame[8];
    if (type0 == 0x02) encryptedFrame[8] = random_n_to_m(1, 10); //2pb
    else if (type0 == 0x04) encryptedFrame[8] = random_n_to_m(11, 20); //4pb
    else if (type0 == 0x06) encryptedFrame[8] = random_n_to_m(21, 30); //6pb
    else if (type0 == 0x08) encryptedFrame[8] = random_n_to_m(31, 40); //8pb
    //IVA 6PB
    else if (type0 == 0x66) encryptedFrame[8] = random_n_to_m(41, 50); //IVA 6pb
    //ATLAS 3PB
    else if (type0 == 0x03) encryptedFrame[8] = random_n_to_m(51, 60); //ATLAS
    else if (type0 == 0x55) encryptedFrame[8] = random_n_to_m(61, 70); //Remote

    // Type-based random values
    uint8_t type1 = originalFrame[9];
    uint8_t type2 = originalFrame[10];
    uint8_t type3 = originalFrame[11];

    // Type1
    if (type1 == 0x01) encryptedFrame[9] = random_n_to_m(1, 10);
    else if (type1 == 0x02) encryptedFrame[9] = random_n_to_m(11, 20);
    else if (type1 == 0x03) encryptedFrame[9] = random_n_to_m(21, 30);
    else if (type1 == 0x04) encryptedFrame[9] = random_n_to_m(31, 40);
    else if (type1 == 0x05) encryptedFrame[9] = random_n_to_m(41, 50);
    else if (type1 == 0x06) encryptedFrame[9] = random_n_to_m(51, 60);
    else if (type1 == 0x07) encryptedFrame[9] = random_n_to_m(61, 70);
    else if (type1 == 0x08) encryptedFrame[9] = random_n_to_m(71, 80);

    // Type2 key generation
    if (type2 == 0x01) encryptedFrame[10] = random_n_to_m(1, 20);
    else if (type2 == 0x02) encryptedFrame[10] = random_n_to_m(21, 40);
    else if (type2 == 0x03) encryptedFrame[10] = random_n_to_m(41, 60);
    else if (type2 == 0x04) encryptedFrame[10] = random_n_to_m(61, 80);
    else if (type2 == 0x0D) encryptedFrame[10] = random_n_to_m(81, 99);

    // Type3 key generation
    if (type3 == 0x00) encryptedFrame[11] = random_n_to_m(1, 50); //On
    else if (type3 == 0x01) encryptedFrame[11] = random_n_to_m(51, 99); //Off
    else encryptedFrame[11] = originalFrame[11] + 120; //Atlas-Dimming
    // CRC
    encryptedFrame[12] = originalFrame[12] + 6;
    encryptedFrame[13] = originalFrame[13] - 3;
    
   // ESP_LOGI(TAG_RS485_DATA, "\nEncrypted Frame:\n");
   /* for (int i = 0; i < 14; i++) {
        printf("%02X ", encryptedFrame[i]);
    }
    printf("\n");*/
}

void process_rs485_data(uint8_t *DeviceRxBuff, int len) {
    if (len < 2) return;

    int start = 0;
   // int end = len - 1;
    uint16_t crcByte1 = 0, crcByte2 = 0;
    bool crc_valid = false;
    
    // Print the full buffer values
    ESP_LOGI(TAG_RS485_DATA, "Processed Response:");
    /*for (int i = start; i <= end; i++) {
        printf("%02X ", DeviceRxBuff[i]);
    }
    printf("\n");*/

    if (len >= 14) {
        uint16_t crc = calculate_modbus_crc(DeviceRxBuff, 12);
        crcByte1 = crc & 0xFF;
        crcByte2 = (crc >> 8) & 0xFF;
    
       // printf("\nCRC Calculated: %02X %02X\n", crcByte1, crcByte2);
    
        if (DeviceRxBuff[12] == crcByte1 && DeviceRxBuff[13] == crcByte2) {
            crc_valid = true;
            ESP_LOGI(TAG_RS485_DATA, "rs485--CRC is in correct Format--");
        } else {
            ESP_LOGE(TAG_RS485_DATA, "rs485--CRC is not in correct Format--");
        }
    }


    if(crc_valid) {
        uint8_t device_id[DEVICE_ID_SIZE];
        memcpy(device_id, &DeviceRxBuff[start], DEVICE_ID_SIZE);

            if(DeviceRxBuff[10] == 0x04) {
                ESP_LOGI(TAG_RS485_DATA, "\nDEVICE INCLUSION PROCESSING\n");
                vTaskDelay(pdMS_TO_TICKS(1000));
                uint8_t encryptedData[14];
                encryptFrameandSend(DeviceRxBuff, encryptedData);
                send_rs485_to_device(encryptedData, 14);
                uint8_t device_id[DEVICE_ID_SIZE];
                memcpy(device_id, DeviceRxBuff, DEVICE_ID_SIZE);
            }
            else{
               // printf("BTN PROCESSING\n");
            }
    if (active_client_sock != -1) {
        send(active_client_sock, DeviceRxBuff, len, 0);
        ESP_LOGI(TAG_RS485_APP, "Sent data to client %d\n", active_client_sock);
    }

    // Remote configuration Logic
    if(DeviceRxBuff[8] == 0x55 && DeviceRxBuff[10] != 0x0D) // 0x55 Indicates Remote Data
     { 
    // 54 0F 57 FF FE 0A BC C3 55 02 01 01
        
        ESP_LOGI(TAG_RS485_DATA, "\n---Remote Data---\n");
        uint8_t key[ENTRY_KEY_SIZE];
        memcpy(key, DeviceRxBuff, ENTRY_KEY_SIZE);

        uint8_t *value = lookup_from_nvm(key);
        if (value != NULL) {
            ESP_LOGI(TAG_NVS, "Mapped value from NVM: ");
            /*for (int i = 0; i < ENTRY_VALUE_SIZE; i++) {
                printf("%02X ", value[i]);
            }
            printf("\n");*/

            if (active_client_sock != -1) {
                send(active_client_sock, value, ENTRY_VALUE_SIZE, 0);
                 ESP_LOGI(TAG_NVS, "Sent mapped value to client.\n");
            }
        } else {
            ESP_LOGE(TAG_NVS, "No matching key found in NVM.\n");
        }

     }
    if (DeviceRxBuff[8] == 0x55 && DeviceRxBuff[10] == 0x0D) {
         ESP_LOGI(TAG_NVS, "Matched delete condition: 0x55 && 0x0D\n");

        int deleted_count = 0;

        for (int i = 0; i < MAX_ENTRIES; i++) {
            if (nvm_table[i].valid && memcmp(nvm_table[i].key, DeviceRxBuff, 9) == 0) {
                if (delete_key_from_nvm(nvm_table[i].key, ENTRY_KEY_SIZE)) {
                     ESP_LOGI(TAG_NVS, "Deleted key from NVS: ");
                } else {
                     ESP_LOGE(TAG_NVS, "Failed to delete key from NVS: ");
                }

                for (int k = 0; k < ENTRY_KEY_SIZE; k++) {
                     ESP_LOGI(TAG_NVS, "%02X ", nvm_table[i].key[k]);
                }
                //printf("\n");

                nvm_table[i].valid = false;
                deleted_count++;
            }
        }

        if (deleted_count > 0) {
            compact_nvm_table();  // Only if deletions happened
             ESP_LOGI(TAG_NVS, "Deleted %d matching keys and compacted NVM table.\n", deleted_count);
        } else {
             ESP_LOGE(TAG_NVS, "No matching keys found for deletion.\n");
        }

        return;
    }

    crc_valid = false;
    }
}


static uint8_t add_sub_transform(uint8_t byte, int8_t value) {
    return (uint8_t)(byte + value);
}

void decryptFrame(uint8_t *frame, size_t len) {
    if (len < 14) return;

    // Revert Byte 0 to 7
    int8_t transforms[8] = {-9, 4, -5, 2, -1, 7, -9, 2};
    for (int i = 0; i < 8; i++) {
        frame[i] = add_sub_transform(frame[i], transforms[i]);
    }
    // Decode Byte 8
    uint8_t val8 = frame[8];
    if (val8 >= 1  && val8 <= 10) frame[8] = 0x02;
    else if (val8 >= 11 && val8 <= 20) frame[8] = 0x04;
    else if (val8 >= 21 && val8 <= 30) frame[8] = 0x06;
    else if (val8 >= 31 && val8 <= 40) frame[8] = 0x08;
    else if (val8 >= 41 && val8 <= 50) frame[8] = 0x66;
    else if (val8 >= 51 && val8 <= 60) frame[8] = 0x03;
    else if (val8 >= 61 && val8 <= 70) frame[8] = 0x55;

    // Decode Byte 9
    uint8_t val9 = frame[9];
    if      (val9 >= 1  && val9 <= 10) frame[9] = 0x01;
    else if (val9 >= 11 && val9 <= 20) frame[9] = 0x02;
    else if (val9 >= 21 && val9 <= 30) frame[9] = 0x03;
    else if (val9 >= 31 && val9 <= 40) frame[9] = 0x04;
    else if (val9 >= 41 && val9 <= 50) frame[9] = 0x05;
    else if (val9 >= 51 && val9 <= 60) frame[9] = 0x06;
    else if (val9 >= 61 && val9 <= 70) frame[9] = 0x07;
    else if (val9 >= 71 && val9 <= 80) frame[9] = 0x08;

    // Decode Byte 10
    uint8_t val10 = frame[10];
    if      (val10 >= 1  && val10 <= 20) frame[10] = 0x01;
    else if (val10 >= 21 && val10 <= 40) frame[10] = 0x02;
    else if (val10 >= 41 && val10 <= 60) frame[10] = 0x03;
    else if (val10 >= 61 && val10 <= 80) frame[10] = 0x04;
    else if (val10 >= 81 && val10 <= 99) frame[10] = 0x0D;

    // Decode Byte 11
    uint8_t val11 = frame[11];
    if      (val11 >= 1  && val11 <= 50) frame[11] = 0x00;//On
    else if (val11 >= 51 && val11 <= 99) frame[11] = 0x01;//Off
    else if (val11 >= 120) frame[11] = add_sub_transform(frame[11], -120);//ATLAS-Dimming
    // Revert Byte 12 and 13
    frame[12] = add_sub_transform(frame[12], -6);
    frame[13] = add_sub_transform(frame[13], 3);
    //printf("-->Received Btn/Knob %d btn state %d\n", frame[9], frame[11]);
}

void get_data_from_device(void *pvParameters) {
    while (1) {
        int len = uart_read_bytes(UART_NUM, rxBuff_from_device, sizeof(rxBuff_from_device), pdMS_TO_TICKS(10));

        if (len > 0) {
            ESP_LOGI(TAG_RS485_DATA, "Raw Response and len %d: ", len);
            /*for (int k = 0; k < len; k++) {
                printf("%02X ", rxBuff_from_device[k]);
            }
           printf("\n");*/

           // Decryption RS485 Data
           decryptFrame(rxBuff_from_device, len);

           ESP_LOGI(TAG_RS485_DATA, "Decrypted Response: ");
           /*for (int k = 0; k < len; k++) {
               printf("%02X ", rxBuff_from_device[k]);
           }
           printf("\n");*/

            // Process data if at least 14 bytes received
            if (len == 14) {
                process_rs485_data(rxBuff_from_device, len);
            }
            memset(rxBuff_from_device, 0, sizeof(rxBuff_from_device));
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}


void print_ip_address()
{
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_next_unsafe(NULL);

    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK)
    {
        ESP_LOGI(TAG_RS485_APP,"ESP32 IP Address: " IPSTR "\n", IP2STR(&ip_info.ip));
    }
    else
    {
        ESP_LOGE(TAG_RS485_APP,"Failed to get IP Address\n");
    }
}

void start_mdns_service(void) {
    static bool mdns_started = false;
    if (!mdns_started) {
        ESP_ERROR_CHECK(mdns_init());
        ESP_ERROR_CHECK(mdns_hostname_set("confiors485"));
        ESP_ERROR_CHECK(mdns_instance_name_set("Confio RS485 Bridge"));
        ESP_ERROR_CHECK(mdns_service_add(NULL, "_confiors485", "_tcp", TCP_PORT, NULL, 0));
        ESP_LOGI(TAG_RS485_APP,"mDNS service started with hostname 'confiors485' on port %d\n", TCP_PORT);
        mdns_started = true;
    } else {
        ESP_LOGI(TAG_RS485_APP,"mDNS already started, skipping service add.\n");
    }
}

void send_C4_rs485Data(void *param)
{
    vTaskDelay(pdMS_TO_TICKS(300));  // wait 300 ms

    if (accumulated_len == 0 || accumulated_buffer == NULL) {
        ESP_LOGW(TAG_C4_DATA, "No data to process.");
        processing_scheduled = false;
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG_C4_DATA, "Processing accumulated data (%d bytes):", accumulated_len);
    /*for (int i = 0; i < accumulated_len; i++) {
        printf("%02X ", accumulated_buffer[i]);
    }
    printf("\n");*/

    int frame_len = 14;
    int num_frames = accumulated_len / frame_len;

    for (int f = 0; f < num_frames; f++) {
        uint8_t frame[14];
        uint8_t remote_frame[26];        
        memcpy(frame, &accumulated_buffer[f * frame_len], frame_len);
        memcpy(remote_frame, &accumulated_buffer[f * 26], 26);
        uint16_t crc = calculate_modbus_crc(frame, 12);
        uint8_t crcByte1 = crc & 0xFF;
        uint8_t crcByte2 = (crc >> 8) & 0xFF;
        if (accumulated_len >= 26 && remote_frame[8] == 0x55) {
            uint8_t key[ENTRY_KEY_SIZE];
            uint8_t value[ENTRY_VALUE_SIZE];

            memcpy(key, remote_frame, ENTRY_KEY_SIZE);                  // Bytes 0-11 = key
            memcpy(value, &remote_frame[ENTRY_KEY_SIZE], ENTRY_VALUE_SIZE); // Bytes 12-25 = value

            ESP_LOGI(TAG_NVS, "Storing Key: ");
           /* for (int i = 0; i < ENTRY_KEY_SIZE; i++) {
                printf("%02X ", key[i]);
            }
            printf("\n");*/

            ESP_LOGI(TAG_NVS, "Storing Value: ");
            for (int i = 0; i < ENTRY_VALUE_SIZE; i++) {
                ESP_LOGE(TAG_NVS, "%02X ", value[i]);
            }
           // printf("\n");

            store_to_nvm(key, value);
        }
        else if (frame[12] == crcByte1 && frame[13] == crcByte2) {
            ESP_LOGI(TAG_C4_DATA, "--CRC is in correct Format--");

            uint8_t encryptedData[14];
            encryptFrameandSend(frame, encryptedData);
            
            send_rs485_to_device(encryptedData, 14);
            ESP_LOGI(TAG_C4_DATA, "Sent encrypted data over RS485.");
            vTaskDelay(pdMS_TO_TICKS(50));
        } else {
            ESP_LOGE(TAG_C4_DATA, "--CRC is not in correct Format--");
        }
    }

    // Clean up
    free(accumulated_buffer);
    accumulated_buffer = NULL;
    accumulated_len = 0;
    processing_scheduled = false;

    vTaskDelete(NULL);  // Delete self
}


void handle_client_task(void *param) {
    int client_sock = *(int *)param;
    free(param);

    // if (active_client_sock != -1) {
    //     ESP_LOGI(TAG_RS485_APP,"Another client is already connected. Closing new connection.\n");
    //     close(client_sock);
    //     vTaskDelete(NULL);
    //     return;
    // }
    // If a previous client exists, close and delete it
    if (active_client_sock != -1) {
        ESP_LOGW(TAG_RS485_APP, "Existing client on socket %d found. Disconnecting old client.", active_client_sock);
        shutdown(active_client_sock, SHUT_RDWR);
        close(active_client_sock);

        if (active_client_task != NULL && active_client_task != xTaskGetCurrentTaskHandle()) {
            vTaskDelete(active_client_task);  // kill old task
            ESP_LOGW(TAG_RS485_APP, "Old client task deleted.");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
    active_client_sock = client_sock;
    active_client_task = xTaskGetCurrentTaskHandle();
    ESP_LOGI(TAG_RS485_APP,"Client connected on socket %d\n", client_sock);
    // Set recv timeout
    struct timeval timeout;
    timeout.tv_sec = 30;  // recv() timeout after 5 seconds idle
    timeout.tv_usec = 0;
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Timer for heartbeat
    TickType_t last_ping_time = xTaskGetTickCount();

    while (1) {
        uint8_t buffer[64];
        int recv_len = recv(client_sock, buffer, sizeof(buffer), 0);
        int err = errno;
        if (recv_len < 0) {
            
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // recv() timed out; send heartbeat
                TickType_t now = xTaskGetTickCount();
                if (now - last_ping_time >= pdMS_TO_TICKS(30000)) {
                    const char *ping = "PING";
                    send(client_sock, ping, strlen(ping), 0);
                    last_ping_time = now;
                    ESP_LOGI(TAG_RS485_APP, "Sent heartbeat to client.\n");
                }
                continue;  // no data, but keep socket alive
            } 
            else if (err == EHOSTUNREACH) {
                ESP_LOGE(TAG_RS485_APP, "Socket error: No route to host (errno=113). Restarting ESP.\n");
                //esp_restart();
            }
            else {
                ESP_LOGE(TAG_RS485_APP, "Socket error on recv: errno=%d\n", err);
                break;
            }
        } else if (recv_len == 0) {
            // Client closed connection
           ESP_LOGI(TAG_RS485_APP, "Client disconnected from socket %d\n", client_sock);
            break;
        } else {
            // ESP_LOGI(TAG_C4_DATA, "Received %d bytes from client: ", recv_len);
            // for (int i = 0; i < recv_len; i++) {
            //     //printf("%02X ", buffer[i]);
            // }
            // //printf("\n");

            // int frame_len = 14;
            // int num_frames = recv_len / frame_len;

            // for (int f = 0; f < num_frames; f++) {
            //     uint8_t frame[14];
            //     uint8_t remote_frame[26];
            //     memcpy(frame, &buffer[f * frame_len], frame_len);
            //     memcpy(remote_frame, &buffer[f * 26], 26);
            //     uint16_t crc = calculate_modbus_crc(frame, 12);
            //     uint8_t crcByte1 = crc & 0xFF;
            //     uint8_t crcByte2 = (crc >> 8) & 0xFF;

            //     if (recv_len >= 26 && remote_frame[8] == 0x55) {
            //         uint8_t key[ENTRY_KEY_SIZE];
            //         uint8_t value[ENTRY_VALUE_SIZE];

            //         memcpy(key, remote_frame, ENTRY_KEY_SIZE);                  // Bytes 0-11 = key
            //         memcpy(value, &remote_frame[ENTRY_KEY_SIZE], ENTRY_VALUE_SIZE); // Bytes 12-25 = value

            //         ESP_LOGI(TAG_NVS, "Storing Key: ");
            //         for (int i = 0; i < ENTRY_KEY_SIZE; i++) {
            //             //printf("%02X ", key[i]);
            //         }
            //         //printf("\n");

            //         ESP_LOGI(TAG_NVS, "Storing Value: ");
            //         for (int i = 0; i < ENTRY_VALUE_SIZE; i++) {
            //             ESP_LOGE(TAG_NVS, "%02X ", value[i]);
            //         }
            //         //printf("\n");

            //         store_to_nvm(key, value);
            //     }


            //     else if (frame[12] == crcByte1 && frame[13] == crcByte2) {
            //         ESP_LOGI(TAG_C4_DATA,"--CRC is in correct Format--\n");

            //         uint8_t encryptedData[14];
            //         encryptFrameandSend(frame, encryptedData);
            //         send_rs485_to_device(encryptedData, 14);
            //         ESP_LOGI(TAG_C4_DATA, "Sent encrypted data over RS485.\n");
            //     } else {
            //         ESP_LOGE(TAG_C4_DATA, "--CRC is not in correct Format--\n");
            //     }
            // }
            ESP_LOGI(TAG_C4_DATA, "Received %d bytes from client:", recv_len);
           /* for (int i = 0; i < recv_len; i++) {
                printf("%02X ", buffer[i]);
            }
            printf("\n");*/

            // Reallocate buffer to append new data
            uint8_t *new_buffer = realloc(accumulated_buffer, accumulated_len + recv_len);
            if (new_buffer == NULL) {
                ESP_LOGE(TAG_C4_DATA, "Memory allocation failed, resetting buffer.");
                free(accumulated_buffer);
                accumulated_buffer = NULL;
                accumulated_len = 0;
                return;
            }

            accumulated_buffer = new_buffer;
            memcpy(&accumulated_buffer[accumulated_len], buffer, recv_len);
            accumulated_len += recv_len;

            // Schedule processing only once per accumulation cycle
            if (!processing_scheduled) {
                processing_scheduled = true;
                xTaskCreate(send_C4_rs485Data, "process_after_sometime", 4096, NULL, 5, NULL);
            }
        }
    }

    // Cleanup
    ESP_LOGI(TAG_RS485_APP, "Cleaning up socket %d", client_sock);
    shutdown(client_sock, SHUT_RDWR);
    close(client_sock);
    active_client_sock = -1;
    active_client_task = NULL;
    vTaskDelete(NULL);
}


void tcp_server_task(void *pvParameters) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int listen_sock, *new_client_sock;

    // Create socket
    listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_sock < 0) {
        ESP_LOGE(TAG_RS485_APP,"Socket creation failed, errno=%d\n", errno);
        vTaskDelete(NULL);
        return;
    }

    // Allow socket reuse
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind to port
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TCP_PORT);

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(TAG_RS485_APP,"Socket bind failed, errno=%d\n", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    // Start listening
    if (listen(listen_sock, 1) < 0) {
        ESP_LOGE(TAG_RS485_APP,"Socket listen failed, errno=%d\n", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG_RS485_APP, "TCP server listening on port %d\n", TCP_PORT);

    while (1) {
        //ESP_LOGI(TAG_RS485_APP, "Waiting for new client...\n");
        int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            ESP_LOGE(TAG_RS485_APP, "Accept failed, errno=%d\n", errno);
            continue;
        }

        ESP_LOGI(TAG_RS485_APP, "New client connected, socket %d\n", client_sock);

        // Enable TCP keepalive
        int keepalive = 1;
        int keepidle = 10;     // idle before sending keepalive probe
        int keepintvl = 5;     // interval between probes
        int keepcnt = 3;       // number of probes before closing
        setsockopt(client_sock, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
        setsockopt(client_sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle));
        setsockopt(client_sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl));
        setsockopt(client_sock, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt));

        new_client_sock = malloc(sizeof(int));
        if (new_client_sock == NULL) {
            ESP_LOGE(TAG_RS485_APP, "Failed to allocate memory for client socket\n");
            close(client_sock);
            continue;
        }

        // *new_client_sock = client_sock;
        // if (xTaskCreate(handle_client_task, "handle_client", 4096, new_client_sock, 5, NULL) != pdPASS) {
        //     ESP_LOGE(TAG_RS485_APP, "Failed to create client handler task\n");
        //     free(new_client_sock);
        //     close(client_sock);
        // }
        *new_client_sock = client_sock;
        BaseType_t result = xTaskCreate(handle_client_task, "handle_client", 4096, new_client_sock, 5, NULL);
        if (result != pdPASS) {
            ESP_LOGE(TAG_RS485_APP, "Failed to create client handler task\n");
            close(client_sock);     // Close socket on task failure
            free(new_client_sock);
        }
    }

    close(listen_sock);
    vTaskDelete(NULL);
}

static void generate_eth_mac(uint8_t *mac_addr) {
    uint8_t base_mac[6];
    ESP_ERROR_CHECK(esp_read_mac(base_mac, ESP_MAC_WIFI_STA));
    mac_addr[0] = 0x02;
    mac_addr[1] = base_mac[1];
    mac_addr[2] = base_mac[2];
    mac_addr[3] = base_mac[3];
    mac_addr[4] = base_mac[4];
    mac_addr[5] = base_mac[5] + 1;
    
    ESP_LOGI(TAG_RS485_APP, "Generated ETH MAC: %02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2],
             mac_addr[3], mac_addr[4], mac_addr[5]);
}

static void eth_event_handler(void *arg, esp_event_base_t event_base,
    int32_t event_id, void *event_data) {
    uint8_t mac_addr[6] = {0};
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG_RS485_APP, "Ethernet Link Up");
        ESP_LOGI(TAG_RS485_APP, "MAC: %02x:%02x:%02x:%02x:%02x:%02x",
        mac_addr[0], mac_addr[1], mac_addr[2],
        mac_addr[3], mac_addr[4], mac_addr[5]);
    

        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG_RS485_APP, "Ethernet Link Down");
        break;
    default:
        break;
    }
}

static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
    int32_t event_id, void *event_data) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG_RS485_APP, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    start_mdns_service();
}

void ethernet_config(void){
    ESP_ERROR_CHECK(esp_netif_init());
    gpio_install_isr_service(0);
    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = ETH_SPI_MISO_GPIO,
        .mosi_io_num = ETH_SPI_MOSI_GPIO,
        .sclk_io_num = ETH_SPI_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(ETH_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // Configure SPI device for W5500
    spi_device_interface_config_t devcfg = {
        .command_bits = 16,
        .address_bits = 8,
        .mode = 0,
        .clock_speed_hz = ETH_SPI_CLOCK_MHZ * 1000 * 1000,
        .spics_io_num = ETH_SPI_CS_GPIO,
        .queue_size = 20,
    };

    // Configure W5500
    eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(ETH_SPI_HOST, &devcfg);
    w5500_config.int_gpio_num = ETH_SPI_INT_GPIO;
    // if (ETH_SPI_RST_GPIO >= 0) {
    //     w5500_config.reset_gpio_num = ETH_SPI_RST_GPIO;
    // }
// if (ETH_SPI_RST_GPIO >= 0) {
//     gpio_set_direction(ETH_SPI_RST_GPIO, GPIO_MODE_OUTPUT);
//     gpio_set_level(ETH_SPI_RST_GPIO, 0);   // Pull low to reset
//     vTaskDelay(pdMS_TO_TICKS(100));
//     gpio_set_level(ETH_SPI_RST_GPIO, 1);   // Release reset
//     vTaskDelay(pdMS_TO_TICKS(100));        // Wait for chip to stabilize
// }
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);
    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &eth_handle));
    uint8_t eth_mac[6];
    generate_eth_mac(eth_mac);
    ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle, ETH_CMD_S_MAC_ADDR, eth_mac));
    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *netif = esp_netif_new(&netif_cfg);
    ESP_ERROR_CHECK(esp_netif_attach(netif, esp_eth_new_netif_glue(eth_handle)));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    // Start Ethernet
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));

    ESP_LOGI(TAG_RS485_APP, "Ethernet initialized");
}



void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_NONE);
   // esp_log_level_set("*", ESP_LOG_ERROR);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ethernet_config();
    vTaskDelay(pdMS_TO_TICKS(1000));
    init_nvs();
    rebuild_nvm_table_from_flash();
    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 7, NULL);
    init_uart();
    xTaskCreate(get_data_from_device, "get_data_from_device", 4096, NULL, 6, NULL);
}

