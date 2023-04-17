// add cac thu vien
#include <stdio.h>
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

// dinh nghia cac chan lcd
#define LCD_ADDR 0x27
#define SDA_PIN 21
#define SCL_PIN 22
#define LCD_COLS 16
#define LCD_ROWS 2

//dinh nghia cac chan in out
#define ESP_INR_FLAG_DEFAULT 0
#define LED_PIN_1 23
#define LED_PIN_2 27
#define LED_PIN_3 33
#define speaker_PIN 19
#define PUSH_BUTTON_PIN_1 15
#define PUSH_BUTTON_PIN_2 14
#define PUSH_BUTTON_PIN_3 17
#define pin_sensor 18

TaskHandle_t ISR_1 = NULL;
TaskHandle_t ISR_2 = NULL;
TaskHandle_t ISR_3 = NULL;
TaskHandle_t ISR_4 = NULL;

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
        gpio_set_level(LED_PIN_3, led_status);
        printf("Button pressed_task3!\n");
    }
}

// task 4 thuc hien khi ngat tren pin sensor 18

void interrupt_task_4(void *arg)
{
    bool speaker_status ;
    while (1)
    {
        vTaskSuspend(NULL);
         speaker_status = true;
        gpio_set_level(speaker_PIN, speaker_status);
        vTaskDelay(5000 / portTICK_RATE_MS);
       speaker_status= !speaker_status;
         gpio_set_level(speaker_PIN, speaker_status);
    } 
}
// ham read dht11

void DHT_reader_task(void *pvParameter)
{
    setDHTgpio(GPIO_NUM_25);
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

        vTaskDelay(5000 / portTICK_RATE_MS);
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

        LCD_setCursor(0, 2);
        sprintf(num, "Temp_%.2f_degC", getTemperature());
        LCD_writeStr(num);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void set_up()
{
    // set up chan gpio
    gpio_pad_select_gpio(PUSH_BUTTON_PIN_1|PUSH_BUTTON_PIN_2|PUSH_BUTTON_PIN_3|pin_sensor);
    gpio_pad_select_gpio(LED_PIN_1|LED_PIN_2|LED_PIN_3|speaker_PIN);
   // set in out
    gpio_set_direction(PUSH_BUTTON_PIN_1|PUSH_BUTTON_PIN_2|PUSH_BUTTON_PIN_3|pin_sensor, GPIO_MODE_INPUT);
    gpio_set_direction(LED_PIN_1|LED_PIN_2|LED_PIN_3|speaker_PIN, GPIO_MODE_OUTPUT);
    // set ngat cạnh lên và mode kéo xuống trên button
    gpio_set_intr_type(PUSH_BUTTON_PIN_1|PUSH_BUTTON_PIN_2|PUSH_BUTTON_PIN_3, GPIO_INTR_POSEDGE);
    gpio_set_pull_mode(PUSH_BUTTON_PIN_1|PUSH_BUTTON_PIN_2|PUSH_BUTTON_PIN_3, GPIO_PULLDOWN_ONLY);
// set ngắt cạnh xuống và mode kéo lên trên sensor SR602
    gpio_set_intr_type(pin_sensor, GPIO_INTR_NEGEDGE);
    gpio_set_pull_mode(pin_sensor, GPIO_PULLUP_ONLY);
// tạo cac ham ngắt tương ứng với các chân
    gpio_install_isr_service(ESP_INR_FLAG_DEFAULT);
    gpio_isr_handler_add(PUSH_BUTTON_PIN_1, button_isr_handler_1, NULL);
    gpio_isr_handler_add(PUSH_BUTTON_PIN_2, button_isr_handler_2, NULL);
    gpio_isr_handler_add(PUSH_BUTTON_PIN_3, button_isr_handler_3, NULL);
    gpio_isr_handler_add(pin_sensor, button_isr_handler_4, NULL);
}


void app_main()
{
    set_up();
    // tạo các task
    xTaskCreate(&DHT_reader_task, "DHT_reader_task", 2048, NULL, 10, NULL);
    xTaskCreate(interrupt_task_1, "interrupt_task_1", 4096, NULL, 12, &ISR_1);
    xTaskCreate(interrupt_task_2, "interrupt_task_2", 4096, NULL, 10, &ISR_2);
    xTaskCreate(interrupt_task_3, "interrupt_task_3", 4096, NULL, 10, &ISR_3);
    xTaskCreate(interrupt_task_4, "interrupt_task_4", 4096, NULL, 10, &ISR_4);

    LCD_init(LCD_ADDR, SDA_PIN, SCL_PIN, LCD_COLS, LCD_ROWS);
    xTaskCreate(&LCD_DemoTask, "Demo Task", 2048, NULL, 10, NULL);

}
