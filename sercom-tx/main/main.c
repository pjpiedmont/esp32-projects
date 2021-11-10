/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/rmt.h"
#include "led_strip.h"

#define DELAY(ms)  vTaskDelay(pdMS_TO_TICKS(ms))

#define TX_PERIOD  20

#define TX_CLK   0
#define TX_EN    1
#define TX_DATA  2
#define GPIO_OUTPUTS  ((1ULL << TX_CLK) | (1ULL << TX_EN) | (1ULL << TX_DATA))

#define RMT_TX_CHANNEL  RMT_CHANNEL_0

char *message = "Hello world!";
led_strip_t *strip;

static void send(void *pvParameters);

void hw_setup(void);
void clrLED(void);
void setLED(void);

void app_main(void)
{
	hw_setup();

	DELAY(1000);

	xTaskCreate(send, "Send Data", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	vTaskDelete(NULL);
}

static void send(void *pvParameters)
{
	char *ptr;
	int bit_pos;
	uint8_t bit;

	while (1)
	{
		ptr = message;

		gpio_set_level(TX_EN, 0);

		while (*ptr != 0)
		{
			char tx_byte = *ptr;
			printf("%c: ", tx_byte);

			for (bit_pos = 0; bit_pos < 8; bit_pos++)
			{
				gpio_set_level(TX_CLK, 1);

				bit = tx_byte & (0x80 >> bit_pos);

				if (bit)
				{
					gpio_set_level(TX_DATA, 1);
					printf("1");
				}
				else
				{
					gpio_set_level(TX_DATA, 0);
					printf("0");
				}

				DELAY(TX_PERIOD / 2);
				gpio_set_level(TX_CLK, 0);
				DELAY(TX_PERIOD / 2);
			}

			printf("\n");
			ptr++;
		}

		gpio_set_level(TX_EN, 1);
		printf("\n");
		DELAY(TX_PERIOD * 5);
	}
}

void hw_setup(void)
{
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = GPIO_OUTPUTS;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);

	gpio_set_level(TX_CLK, 0);
	gpio_set_level(TX_EN, 1);
	gpio_set_level(TX_DATA, 0);

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
		strip->set_pixel(strip, i, 0, 20, 0);
		strip->refresh(strip, 100);
		DELAY(10);
	}
}