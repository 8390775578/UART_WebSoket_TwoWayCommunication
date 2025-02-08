#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "esp_http_server.h"

/* UART Configuration */
#define ECHO_TEST_TXD (43)
#define ECHO_TEST_RXD (44)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM      (0)
#define ECHO_UART_BAUD_RATE     (115200)
#define BUF_SIZE                (1024)
#define ECHO_TASK_STACK_SIZE    (4096)  

static const char *TAG = "UART_WS_INTEGRATION";

static QueueHandle_t uart_to_ws_queue = NULL;
static httpd_handle_t server = NULL;
static int websocket_fd = -1; 


static void uart_task(void *arg) {
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, BUF_SIZE - 1, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0';  
            ESP_LOGI(TAG, "UART Received: %s", (char *)data);

            // Send data to WebSocket if connected
            if (websocket_fd != -1) {
                xQueueSend(uart_to_ws_queue, data, portMAX_DELAY);
            }
        }
    }
}


static esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "WebSocket Handshake done.");
        websocket_fd = httpd_req_to_sockfd(req);
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    // Get frame length
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error getting WS frame length: %d", ret);
        return ret;
    }

    if (ws_pkt.len) {
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Memory allocation failed for WS payload");
            return ESP_ERR_NO_MEM;
        }

        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to receive WS frame: %d", ret);
            free(buf);
            return ret;
        }

        ESP_LOGI(TAG, "WebSocket Received: %s", ws_pkt.payload);

        uart_write_bytes(ECHO_UART_PORT_NUM, (const char *)ws_pkt.payload, ws_pkt.len);
        free(buf);
    }

    return ESP_OK;
}

static void ws_send_task(void *arg) {
    uint8_t data[BUF_SIZE];

    while (1) {
        if (xQueueReceive(uart_to_ws_queue, data, portMAX_DELAY) == pdTRUE) {
            httpd_ws_frame_t ws_pkt;
            memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
            ws_pkt.payload = data;
            ws_pkt.len = strlen((char *)data);
            ws_pkt.type = HTTPD_WS_TYPE_TEXT;

            if (websocket_fd != -1) {
                esp_err_t ret = httpd_ws_send_frame_async(server, websocket_fd, &ws_pkt);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to send data via WebSocket: %d", ret);
                }
            }
        }
    }
}


static httpd_handle_t start_webserver(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_uri_t ws_uri = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .is_websocket = true
    };

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &ws_uri);
        ESP_LOGI(TAG, "WebServer started successfully.");
        return server;
    }

    ESP_LOGE(TAG, "Failed to start WebServer.");
    return NULL;
}


void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    uart_to_ws_queue = xQueueCreate(10, BUF_SIZE);

    start_webserver();

    xTaskCreate(uart_task, "uart_task", 4096, NULL, 10, NULL);
    xTaskCreate(ws_send_task, "ws_send_task", 4096, NULL, 9, NULL);
}