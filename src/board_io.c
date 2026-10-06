/*******************************************************************
 * @file board_io.c
 *
 * @brief Implementação do módulo de entradas/saídas (Atividade-03).
 *******************************************************************/

#include "board_io.h"

#include <errno.h>

#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

int io_init(void)
{
	int ret;

	if (!gpio_is_ready_dt(&led)) {
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&button)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int led_set(bool on)
{
	return gpio_pin_set_dt(&led, on ? 1 : 0);
}

int button_read(void)
{
	return gpio_pin_get_dt(&button);
}
