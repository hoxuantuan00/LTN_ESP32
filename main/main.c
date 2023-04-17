
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

#define LCD_ADDR 0x27
#define SDA_PIN 21
#define SCL_PIN 22
#define LCD_COLS 16
#define LCD_ROWS 2

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

void IRAM_ATTR button_isr_handler_1(void *arg)
{
    xTaskResumeFromISR(ISR_1);
}
void IRAM_ATTR button_isr_handler_2(void *arg)
{
    xTaskResumeFromISR(ISR_2);
}
void IRAM_ATTR button_isr_handler_3(void *arg)
{
    xTaskResumeFromISR(ISR_3);
}
void IRAM_ATTR button_isr_handler_4(void *arg)
{
    xTaskResumeFromISR(ISR_4);
}


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

void interrupt_task_4(void *arg)
{
    bool speaker_status ;
    while (1)
    {
        vTaskSuspend(NULL);
         speaker_status = true;
        gpio_set_level(speaker_PIN, speaker_status);
        vTaskDelay(3000 / portTICK_RATE_MS);
       speaker_status= !speaker_status;
         gpio_set_level(speaker_PIN, speaker_status);
    } 
}

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
    gpio_pad_select_gpio(PUSH_BUTTON_PIN_1|PUSH_BUTTON_PIN_2|PUSH_BUTTON_PIN_3);
   // gpio_pad_select_gpio(PUSH_BUTTON_PIN_2);
    //gpio_pad_select_gpio(PUSH_BUTTON_PIN_3);
    gpio_pad_select_gpio(pin_sensor);
    gpio_pad_select_gpio(LED_PIN_1);
    gpio_pad_select_gpio(LED_PIN_2);
    gpio_pad_select_gpio(LED_PIN_3);
    gpio_pad_select_gpio(speaker_PIN);

    gpio_set_direction(PUSH_BUTTON_PIN_1, GPIO_MODE_INPUT);
    gpio_set_direction(PUSH_BUTTON_PIN_2, GPIO_MODE_INPUT);
    gpio_set_direction(PUSH_BUTTON_PIN_3, GPIO_MODE_INPUT);
    gpio_set_direction(pin_sensor, GPIO_MODE_INPUT);
    gpio_set_direction(LED_PIN_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_PIN_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_PIN_3, GPIO_MODE_OUTPUT);
    gpio_set_direction(speaker_PIN, GPIO_MODE_OUTPUT);

    gpio_set_intr_type(PUSH_BUTTON_PIN_1, GPIO_INTR_POSEDGE);
    gpio_set_pull_mode(PUSH_BUTTON_PIN_1, GPIO_PULLDOWN_ONLY);

    gpio_set_intr_type(PUSH_BUTTON_PIN_2, GPIO_INTR_POSEDGE);
    gpio_set_pull_mode(PUSH_BUTTON_PIN_2, GPIO_PULLDOWN_ONLY);

    gpio_set_intr_type(PUSH_BUTTON_PIN_3, GPIO_INTR_POSEDGE);
    gpio_set_pull_mode(PUSH_BUTTON_PIN_3, GPIO_PULLDOWN_ONLY);

    gpio_set_intr_type(pin_sensor, GPIO_INTR_NEGEDGE);
    gpio_set_pull_mode(pin_sensor, GPIO_PULLUP_ONLY);

    gpio_install_isr_service(ESP_INR_FLAG_DEFAULT);
    gpio_isr_handler_add(PUSH_BUTTON_PIN_1, button_isr_handler_1, NULL);
    gpio_isr_handler_add(PUSH_BUTTON_PIN_2, button_isr_handler_2, NULL);
    gpio_isr_handler_add(PUSH_BUTTON_PIN_3, button_isr_handler_3, NULL);
    gpio_isr_handler_add(pin_sensor, button_isr_handler_4, NULL);
}


void app_main()
{
    set_up();
    xTaskCreate(&DHT_reader_task, "DHT_reader_task", 2048, NULL, 10, NULL);
    xTaskCreate(interrupt_task_1, "interrupt_task_1", 4096, NULL, 12, &ISR_1);
    xTaskCreate(interrupt_task_2, "interrupt_task_2", 4096, NULL, 10, &ISR_2);
    xTaskCreate(interrupt_task_3, "interrupt_task_3", 4096, NULL, 10, &ISR_3);
    xTaskCreate(interrupt_task_4, "interrupt_task_4", 4096, NULL, 10, &ISR_4);

    LCD_init(LCD_ADDR, SDA_PIN, SCL_PIN, LCD_COLS, LCD_ROWS);
    xTaskCreate(&LCD_DemoTask, "Demo Task", 2048, NULL, 10, NULL);

}
