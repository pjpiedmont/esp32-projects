/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/rmt.h"
#include "led_strip.h"

#define DELAY(ms) vTaskDelay(pdMS_TO_TICKS(ms))

#define RX_CLK   0
#define RX_EN    1
#define RX_DATA  2
#define GPIO_INPUTS ((1ULL << RX_CLK) | (1ULL << RX_EN) | (1ULL << RX_DATA))

#define INT  6
#define GPIO_OUTPUTS (1ULL << INT)

#define ESP_INTR_FLAG_DEFAULT 0

#define RMT_TX_CHANNEL RMT_CHANNEL_0

SemaphoreHandle_t sig = NULL;
static xQueueHandle gpio_evt_queue = NULL;
led_strip_t *strip;

static void receive(void* pvParameters);
static void IRAM_ATTR onCLK(void* arg);

void hw_setup(void);
void clrLED(void);
void setLED(void);

void app_main(void)
{
	hw_setup();

	DELAY(1000);

	sig = xSemaphoreCreateBinary();
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

	if (gpio_evt_queue != NULL)
	{
		xTaskCreate(receive, "Receive Data", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	}
	else
	{
		printf("Semaphore failed to create\n");
	}

	vTaskDelete(NULL);
}

static void receive(void* pvParameters)
{
	// bool rx_bit = digitalRead(RX_DATA);

	// if (bit_position == 8)
	// {
	// 	rx_byte = 0;
	// 	bit_position = 0;
	// }

	// if (rx_bit)
	// {
	// 	rx_byte |= (0x80 >> bit_position);
	// }

	// bit_position += 1;

	// if (bit_position == 8)
	// {
	// 	strncat(message, (const char *)&rx_byte, 1);
	// }

	// update_lcd = true;

	// xSemaphoreTake(sig, 0);

	// while (1)
	// {
	// 	xSemaphoreTake(sig, portMAX_DELAY);
	// 	printf("CLK high\n");
	// }

	uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
        }
    }
}

static void IRAM_ATTR onCLK(void* arg)
{
	static int level = 1;
	// portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	// xSemaphoreGiveFromISR(sig, &xHigherPriorityTaskWoken);
	uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
	
	gpio_set_level(INT, level);
	level = !level;
}

void hw_setup(void)
{
	gpio_config_t input_conf;
	input_conf.pin_bit_mask = GPIO_INPUTS;
	input_conf.intr_type = GPIO_INTR_POSEDGE;
	input_conf.mode = GPIO_MODE_INPUT;
	input_conf.pull_up_en = 1;
	input_conf.pull_down_en = 0;
	gpio_config(&input_conf);

	gpio_config_t output_conf;
	output_conf.pin_bit_mask = GPIO_OUTPUTS;
	output_conf.intr_type = GPIO_INTR_DISABLE;
	output_conf.mode = GPIO_MODE_OUTPUT;
	output_conf.pull_up_en = 0;
	output_conf.pull_down_en = 0;
	gpio_config(&output_conf);

	gpio_set_intr_type(RX_DATA, GPIO_INTR_DISABLE);
	gpio_set_intr_type(RX_EN, GPIO_INTR_DISABLE);

	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
	gpio_isr_handler_add(RX_CLK, onCLK, (void*) RX_CLK);

	rmt_config_t config = RMT_DEFAULT_CONFIG_TX(CONFIG_EXAMPLE_RMT_TX_GPIO, RMT_TX_CHANNEL);
	// set counter clock to 40MHz
	config.clk_div = 2;
	rmt_config(&config);
	rmt_driver_install(config.channel, 0, 0);

	// install ws2812 driver
	led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(CONFIG_EXAMPLE_STRIP_LED_NUMBER, (led_strip_dev_t)config.channel);
	strip = led_strip_new_rmt_ws2812(&strip_config);
	
	clrLED();
	setLED();
}

void clrLED(void)
{
	strip->clear(strip, 100);
}

void setLED(void)
{
	int i;
	for (i = 0; i < 3; i++)
	{
		strip->set_pixel(strip, i, 0, 0, 20);
		strip->refresh(strip, 100);
		DELAY(10);
	}
}