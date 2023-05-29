// NB_IOT use UART for communication AT commands 
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "freertos/timers.h"
#include <driver/gpio.h>
#include "freertos/event_groups.h"
#include "input_iot.h"
#include "output_iot.h"
#include "driver/uart.h"
#include <string.h>
#include <esp_log.h>

#define BIT_EVENT_BUTTON_PRESS	( 1 << 0 )
#define BIT_EVENT_UART	        ( 1 << 1 )
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)

static const int RX_BUF_SIZE = 1024*4;
// Khai báo một biến đánh dấu trạng thái
static int currentCommand = 0;
static int success = 0;
static int flagCommandCENG = 0;
static char RSRP_INDEX[20];
static char RSSI_INDEX[20];
static char RSRQ_INDEX[20];

void init(void) {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void initPower()
{
    output_io_create(5);
    output_io_set(5,0);
    ets_delay_us(1500000);
    output_io_set(5,1);
}

int sendData(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

void sendMulData(void *arg)
{
    while(1)
    {
        switch(currentCommand) 
            {
                case 0:
                    sendData("DATA", "ATE0\r");
                    if (success)
                    {
                        currentCommand = currentCommand +1;
                        ESP_LOGI("TAG", "currentCommand = %d", currentCommand);
                        success = 0; 
                    } 
                    break;
                case 1:
                    
                    sendData("DATA", "AT+CEREG?\r");
                    if (success)
                    {
                        currentCommand = currentCommand +1;
                        ESP_LOGI("TAG", "currentCommand = %d", currentCommand);
                        success = 0; 
                    } 
                    break;
                    
                case 2:
                    sendData("DATA", "AT+CENG?\r");
                    flagCommandCENG = 1;
                    if (success)
                    {
                        currentCommand = currentCommand +1;
                        ESP_LOGI("TAG", "currentCommand = %d", currentCommand);
                        success = 0;
                    } 
                    break; 
                case 3:
                    sendData("DATA", "AT\r");
                    if (success)
                    {
                        currentCommand = currentCommand +1;
                        ESP_LOGI("TAG", "currentCommand = %d", currentCommand);
                        success = 0;
                    } 
                    break;
                case 4:
                    sendData("DATA", "AT\r");
                    if (success)
                    {
                        currentCommand = currentCommand +1;
                        ESP_LOGI("TAG", "currentCommand = %d", currentCommand);
                        success = 0; 
                    } 
                    break;
                 case 5:
                    sendData("DATA", "AT\r");
                    if (success)
                    {
                        currentCommand = 0;
                        ESP_LOGI("TAG", "currentCommand = %d", currentCommand);
                        success = 0; 
                    } 
                    break;
             }
        vTaskDelay(pdMS_TO_TICKS(2000)); 
    }
}

void convertDataCENG(char *data)
{
    char dataArray[200];
    char *tokens[10];
    int i = 0;

    strcpy(dataArray, data);
    
    tokens[i] = strtok(dataArray, ",");
    
    while (tokens[i] != NULL && i < 10) {
        i++;
        tokens[i] = strtok(NULL, ",");
    }
    // Gán từng biến
    strcpy(RSRP_INDEX, tokens[6]);
    strcpy(RSSI_INDEX, tokens[7]);
    strcpy(RSRQ_INDEX, tokens[8]);
}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            if (strstr((char*)data, "OK") != NULL)
            success = 1;
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
            if (flagCommandCENG==1){
                convertDataCENG((char *)data);
                ESP_LOGI("TAG", "RSRP_INDEX = %s", RSRP_INDEX);
                ESP_LOGI("TAG", "RSSI_INDEX = %s", RSSI_INDEX);
                ESP_LOGI("TAG", "RSRQ_INDEX = %s", RSRQ_INDEX); 
            }
        }
    }
    free(data);
}

void app_main(void)
{
    init();
    initPower();
    ets_delay_us(1500000);
    xTaskCreate (sendMulData,"sendMulData",1024*2,NULL,6,NULL);
    xTaskCreate (rx_task, "uart_rx_task",1024*2, NULL,5, NULL);
}
