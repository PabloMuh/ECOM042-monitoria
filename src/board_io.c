/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "board_io.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(board_io, CONFIG_LOG_DEFAULT_LEVEL);

#define LED0_NODE DT_ALIAS(led0)
#define SW0_NODE  DT_ALIAS(sw0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec btn = GPIO_DT_SPEC_GET(SW0_NODE, gpios);

static bool s_led_state;

int board_io_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("Dispositivo GPIO para led0 nao esta pronto");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Falha ao configurar pino do LED (%d)", ret);
		return ret;
	}
	s_led_state = false;

	if (!gpio_is_ready_dt(&btn)) {
		LOG_ERR("Dispositivo GPIO para sw0 nao esta pronto");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&btn, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Falha ao configurar pino do botao (%d)", ret);
		return ret;
	}

	return 0;
}

int board_led_set(bool on)
{
	int ret;

	ret = gpio_pin_set_dt(&led, on ? 1 : 0);
	if (ret < 0) {
		return ret;
	}

	s_led_state = on;
	return 0;
}

int board_led_get(void)
{
	return s_led_state ? 1 : 0;
}

int board_btn_read(void)
{
	int val = gpio_pin_get_dt(&btn);

	if (val < 0) {
		return val;
	}

	return val > 0 ? 1 : 0;
}
