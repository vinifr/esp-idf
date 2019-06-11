/* Light sleep example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp32/rom/uart.h"
#include "driver/rtc_io.h"
#include "driver/gpio.h"

/* Most development boards have "boot" button attached to GPIO0.
 * You can also change this to another pin.
 */
#define BUTTON_GPIO_NUM_DEFAULT    39// 0

/* "Boot" button on GPIO0 is active low */
#define BUTTON_WAKEUP_LEVEL_DEFAULT     1 //0

#define GPIO_OUTPUT_IO_0    0
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_0))

void gpioConfig()
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}

void app_main()
{
    /* Configure the button GPIO as input, enable wakeup */
    const int button_gpio_num = BUTTON_GPIO_NUM_DEFAULT;
    const int wakeup_level = BUTTON_WAKEUP_LEVEL_DEFAULT;
    gpio_config_t config = {
            .pin_bit_mask = BIT64(button_gpio_num),
            .mode = GPIO_MODE_INPUT
    };
    ESP_ERROR_CHECK(gpio_config(&config));
    gpio_wakeup_enable(button_gpio_num,
            wakeup_level == 0 ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
    //
    gpioConfig();

    while (true) {
        /* Wake up in 10 seconds, or when button is pressed */
        esp_sleep_enable_timer_wakeup(10000000);
        esp_sleep_enable_gpio_wakeup();

        /* Wait until GPIO goes high */
        /*if (rtc_gpio_get_level(button_gpio_num) == wakeup_level) {
            printf("Waiting for GPIO%d to go high...\n", button_gpio_num);
            do {
                vTaskDelay(pdMS_TO_TICKS(10));
            } while (rtc_gpio_get_level(button_gpio_num) == wakeup_level);
        }*/
        gpio_set_level(GPIO_NUM_0, 0);
        vTaskDelay(pdMS_TO_TICKS(10000));

        printf("Entering light sleep\n");
        gpio_set_level(GPIO_NUM_0, 1);
        /* To make sure the complete line is printed before entering sleep mode,
         * need to wait until UART TX FIFO is empty:
         */
        uart_tx_wait_idle(CONFIG_CONSOLE_UART_NUM);

        /* Get timestamp before entering sleep */
        int64_t t_before_us = esp_timer_get_time();

        /* Enter sleep mode */
        esp_light_sleep_start();
        /* Execution continues here after wakeup */

        /* Get timestamp after waking up from sleep */
        int64_t t_after_us = esp_timer_get_time();

        /* Determine wake up reason */
        const char* wakeup_reason;
        switch (esp_sleep_get_wakeup_cause()) {
            case ESP_SLEEP_WAKEUP_TIMER:
                wakeup_reason = "timer";
                break;
            case ESP_SLEEP_WAKEUP_GPIO:
                wakeup_reason = "pin";
                break;
            default:
                wakeup_reason = "other";
                break;
        }

        printf("Returned from light sleep, reason: %s, t=%lld ms, slept for %lld ms\n",
                wakeup_reason, t_after_us / 1000, (t_after_us - t_before_us) / 1000);
    }

}
