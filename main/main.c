// add cac thu vien
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "rom/ets_sys.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "DHT11.h"
#include "HD44780.h"
#include <driver/i2c.h>
#include <string.h>
#include "esp_http_client.h"
// cert
#include "esp_crt_bundle.h"

// wifi 
#include "connect_wifi.h"

// dinh nghia cac chan lcd
#define LCD_ADDR 0x27
#define SDA_PIN 6
#define SCL_PIN 5
#define LCD_COLS 16
#define LCD_ROWS 2

//dinh nghia cac chan in out
#define ESP_INR_FLAG_DEFAULT 0
#define LED_PIN_1 18
#define LED_PIN_2 19
#define LED_PIN_3 20
#define speaker_PIN 0
#define PUSH_BUTTON_PIN_1 1
#define PUSH_BUTTON_PIN_2 2
#define PUSH_BUTTON_PIN_3 3
#define pin_sensor 4

TaskHandle_t ISR_1 = NULL;
TaskHandle_t ISR_2 = NULL;
TaskHandle_t ISR_3 = NULL;
TaskHandle_t ISR_4 = NULL;
// data on mcu
char temp_d[5];
char humi_d[5] ;
char detect_d[5];
char device_1_d[5];
char device_2_d[5];
char device_3_d[5];
// data on sever
char temp[5];
char humi[5];
char detect[5];
char device_1[5];
char device_2[5];
char device_3[5];

#define JSON_URL "https://api.jsonbin.io/v3/b/643934a2c0e7653a05a44ad6"
char X_MASTER_KEY[] = "$2b$10$1mN5y.XkrvbsFt6ovDD.R.2t.YaD262gKkvTZyfJr0S/3Oq/j1kQ6";
char X_ACCESS_KEY[] = "$2b$10$O.WIHfTbkkgTb6IB9XKlFuL9CIYC5kcrnXnA8Hi299J1MMHSt1UpW";


char data[200];
char data_default[] = "{\"temparature\":\"%s\",\"humi\":\"%s\",\"detect\":\"%s\",\"device_1\":\"%s\",\"device_2\":\"%s\",\"device_3\": \"%s\"}";

// tôi đang thay đổi code 

esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        printf("HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);
		char *data_string = (char *)evt->data;
		strncpy(temp, data_string+26, 5);
		strncpy(humi, data_string+41, 5);
		strncpy(detect, data_string+58, 2);
		strncpy(device_1, data_string+74, 2);
		strncpy(device_2, data_string+90, 2);
		strncpy(device_3, data_string+106, 2);
        printf(data_string);
        break;

    default:
        break;
    }
    return ESP_OK;
}
esp_err_t client_event_put_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        printf("HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);
        break;

    default:
        break;
    }
    return ESP_OK;
}


void rest_get()
{
    
        /* code */
    vTaskDelay(15000/portTICK_PERIOD_MS);
    esp_http_client_config_t config_get = {
        .url = JSON_URL,
        .method = HTTP_METHOD_GET,
		.transport_type = HTTP_TRANSPORT_OVER_SSL,  //Specify transport type
		.crt_bundle_attach = esp_crt_bundle_attach, //Attach the certificate bundle 
		.event_handler =client_event_get_handler
	};
	esp_http_client_handle_t client = esp_http_client_init(&config_get);
    // Add header to request
	esp_http_client_set_header(client, "X-Master-Key", X_MASTER_KEY);
	esp_http_client_set_header(client, "X-Access-Key", X_ACCESS_KEY);
	// Perform 
    esp_http_client_perform(client);
	// clean
    esp_http_client_cleanup(client);
    
}

void rest_put(char data[])
{
    esp_http_client_config_t config_get = {
        .url = JSON_URL,
        .method = HTTP_METHOD_PUT,
		.transport_type = HTTP_TRANSPORT_OVER_SSL,  //Specify transport type
		.crt_bundle_attach = esp_crt_bundle_attach, //Attach the certificate bundle 
		.event_handler =client_event_put_handler
	};
	esp_http_client_handle_t client = esp_http_client_init(&config_get);
    // Add header to request
	esp_http_client_set_header(client, "Content-Type", "application/json");
	esp_http_client_set_header(client, "X-Master-Key", X_MASTER_KEY);
	esp_http_client_set_header(client, "X-Access-Key", X_ACCESS_KEY);
	// add post data 
	esp_http_client_set_post_field(client, data, strlen(data));
	// Perform 
    esp_http_client_perform(client);
	// clean
    esp_http_client_cleanup(client);
}



void auto_update(void)
{
	while(1)
	{	
		rest_get();
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		// format data_default replace %s
		sprintf(data, data_default, temp, humi, detect, device_1, device_2, device_3);
		printf("data: %s\n", data);
	}
	
	
}



// convert float to string 


// ham ngat khi pin 15 nhan

void IRAM_ATTR button_isr_handler_1(void *arg)
{
    xTaskResumeFromISR(ISR_1);
}

// ham ngat khi pin 14 nhan

void IRAM_ATTR button_isr_handler_2(void *arg)
{
    xTaskResumeFromISR(ISR_2);
}

// ham ngat khi pin 17 nhan

void IRAM_ATTR button_isr_handler_3(void *arg)
{
    xTaskResumeFromISR(ISR_3);
}
// ham ngat khi pin sensor 18 nhan

void IRAM_ATTR button_isr_handler_4(void *arg)
{
    xTaskResumeFromISR(ISR_4);
}

// task 1 thuc hien khi ngat tren pin 15
void interrupt_task_1(void *arg)
{
    bool led_status = false;
    while (1)
    {
        vTaskSuspend(NULL);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        led_status = !led_status;
        if(led_status == true){
            strcpy(device_1_d, "ON");

        }else{
            strcpy(device_1_d, "OF");
        }
        gpio_set_level(LED_PIN_1, led_status);
        printf("Button pressed_task1!\n");
    }
}

// task 2 thuc hien khi ngat tren pin 14

void interrupt_task_2(void *arg)
{
    bool led_status = false;
    while (1)
    {
        vTaskSuspend(NULL);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        led_status = !led_status;
        if(led_status == true){
            strcpy(device_2_d, "ON");
        }else{
            strcpy(device_2_d, "OF");
        }
        gpio_set_level(LED_PIN_2, led_status);
        printf("Button pressed_task2!\n");
    }
}

// task 3 thuc hien khi ngat tren pin 17

void interrupt_task_3(void *arg)
{
    bool led_status = false;
    while (1)
    {
        vTaskSuspend(NULL);
vTaskDelay(200 / portTICK_PERIOD_MS);
        led_status = !led_status;
        if(led_status == true){
            strcpy(device_3_d, "ON");
        }else{
            strcpy(device_3_d, "OF");
        }
        gpio_set_level(LED_PIN_3, led_status);
        printf("Button pressed_task3!\n");
    }
}

// task 4 thuc hien khi ngat tren pin sensor 18

void interrupt_task_4(void *arg)
{
    bool speaker_status;
    while (1)
    {
        vTaskSuspend(NULL);
        speaker_status = true;
        strcpy(detect_d, "ON");
        gpio_set_level(speaker_PIN, speaker_status);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
       speaker_status= !speaker_status;
         gpio_set_level(speaker_PIN, speaker_status);
    } 
}
// ham read dht11

void DHT_reader_task(void *pvParameter)
{
    setDHTgpio(GPIO_NUM_7);
    while (1)
    {
        int ret = readDHT();
        errorHandler(ret);
        if (getHumidity() >= (float)90.00)
        {
            vTaskResume(ISR_4);
        }
        printf("Humidity %.2f %%\n", getHumidity());
        printf("Temperature %.2f degC\n\n", getTemperature());

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

// hien thi lcd

void LCD_DemoTask(void *param)
{
    char num[20];
    while (true)
    {
        LCD_home();
        LCD_clearScreen();

        sprintf(num, "Hum_%.2f_%%", getHumidity());
        LCD_writeStr(num);
        strncpy(humi_d, num+4, 5);
        LCD_setCursor(0, 2);
        sprintf(num, "Temp_%.2f_degC", getTemperature());
        strncpy(temp_d, num+5, 5);
        LCD_writeStr(num);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void set_up()
{
    // set up chan gpio
    gpio_pad_select_gpio(PUSH_BUTTON_PIN_1);
     gpio_pad_select_gpio(PUSH_BUTTON_PIN_2);
      gpio_pad_select_gpio(PUSH_BUTTON_PIN_3);
       gpio_pad_select_gpio(pin_sensor);
    gpio_pad_select_gpio(LED_PIN_1);
    gpio_pad_select_gpio(LED_PIN_2);
    gpio_pad_select_gpio(LED_PIN_3);
    gpio_pad_select_gpio(speaker_PIN);
   // set in out
    gpio_set_direction(PUSH_BUTTON_PIN_1, GPIO_MODE_INPUT);
     gpio_set_direction(PUSH_BUTTON_PIN_2, GPIO_MODE_INPUT);
      gpio_set_direction(PUSH_BUTTON_PIN_3, GPIO_MODE_INPUT);
      gpio_set_direction(pin_sensor, GPIO_MODE_INPUT);
    gpio_set_direction(LED_PIN_1, GPIO_MODE_OUTPUT);
     gpio_set_direction(LED_PIN_2, GPIO_MODE_OUTPUT);
      gpio_set_direction(LED_PIN_3, GPIO_MODE_OUTPUT);
       gpio_set_direction(speaker_PIN, GPIO_MODE_OUTPUT);
    // set ngat nut nhan, mode kéo xuong tren button
    gpio_set_intr_type(PUSH_BUTTON_PIN_1, GPIO_INTR_POSEDGE);
    gpio_set_pull_mode(PUSH_BUTTON_PIN_1, GPIO_PULLDOWN_ONLY);
    gpio_set_intr_type(PUSH_BUTTON_PIN_2 ,GPIO_INTR_POSEDGE);
    gpio_set_pull_mode(PUSH_BUTTON_PIN_2, GPIO_PULLDOWN_ONLY);
    gpio_set_intr_type(PUSH_BUTTON_PIN_3 ,GPIO_INTR_POSEDGE);
    gpio_set_pull_mode(PUSH_BUTTON_PIN_3, GPIO_PULLDOWN_ONLY);
// set ngat, mode keo len sensor SR602
    gpio_set_intr_type(pin_sensor, GPIO_INTR_NEGEDGE);
    gpio_set_pull_mode(pin_sensor, GPIO_PULLUP_ONLY);
// tao ham ngat tren cac chan tuong ung
    gpio_install_isr_service(ESP_INR_FLAG_DEFAULT);
    gpio_isr_handler_add(PUSH_BUTTON_PIN_1, button_isr_handler_1, NULL);
    gpio_isr_handler_add(PUSH_BUTTON_PIN_2, button_isr_handler_2, NULL);
    gpio_isr_handler_add(PUSH_BUTTON_PIN_3, button_isr_handler_3, NULL);
    gpio_isr_handler_add(pin_sensor, button_isr_handler_4, NULL);
}


void app_main()
{
	
	// Initialize NVS.
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
    set_up();
    connect_wifi();
    vTaskDelay(5000/portTICK_PERIOD_MS);
    rest_get();

    // tao cac task
    xTaskCreate(&DHT_reader_task, "DHT_reader_task", 2048, NULL, 10, NULL);
    xTaskCreate(interrupt_task_1, "interrupt_task_1", 4096, NULL, 12, &ISR_1);
    xTaskCreate(interrupt_task_2, "interrupt_task_2", 4096, NULL, 10, &ISR_2);
    xTaskCreate(interrupt_task_3, "interrupt_task_3", 4096, NULL, 10, &ISR_3);
    xTaskCreate(interrupt_task_4, "interrupt_task_4", 4096, NULL, 10, &ISR_4);
    LCD_init(LCD_ADDR, SDA_PIN, SCL_PIN, LCD_COLS, LCD_ROWS);
    xTaskCreate(&LCD_DemoTask, "Demo Task", 2048, NULL, 10, NULL);

}
