/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @author José Félix de Oliveira Neto (jfon@ic.ufal.br)
 * @version 0.1
 * @date 26/08/2026
 *******************************************************************/

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>

#include "board_io.h"

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);

int main(void)
{
	const int sequence[] = {0, 1, 0, 1};
	int ret;

	ret = io_init();
	if (ret < 0) {
		return ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(sequence); i++) {
		ret = gpio_emul_input_set_dt(&button, sequence[i]);
		if (ret < 0) {
			return ret;
		}

		int btn_state = button_read();
		if (btn_state < 0) {
			return btn_state;
		}

		ret = led_set(btn_state != 0);
		if (ret < 0) {
			return ret;
		}

		printk("Button: %d -> LED: %d\n", btn_state, btn_state);
	}

	return 0;
}
