// SPDX-License-Identifier: Apache-2.0

#include "led.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
	int ret;
	size_t count;

	LOG_INF("Starting LED Controller Application on %s", CONFIG_BOARD);

	ret = led_init();
	if (ret < 0) {
		LOG_ERR("LED initialization failed: %d", ret);
		return ret;
	}

	count = led_get_count();
	LOG_INF("Initialized %u LEDs successfully", (unsigned int)count);

	/* Start LED 0 blinking SLOW */
	ret = led_blink(0, LED_PATTERN_SLOW);
	if (ret < 0) {
		LOG_ERR("Failed to start blink on LED 0: %d", ret);
	} else {
		LOG_INF("LED 0 configured to blink SLOW");
	}

	/* Start LED 1 blinking FAST */
	if (count > 1) {
		ret = led_blink(1, LED_PATTERN_FAST);
		if (ret < 0) {
			LOG_ERR("Failed to start blink on LED 1: %d", ret);
		} else {
			LOG_INF("LED 1 configured to blink FAST");
		}
	}

	/* Start LED 2 blinking NORMAL if present */
	if (count > 2) {
		ret = led_blink(2, LED_PATTERN_NORMAL);
		if (ret < 0) {
			LOG_ERR("Failed to start blink on LED 2: %d", ret);
		} else {
			LOG_INF("LED 2 configured to blink NORMAL");
		}
	}

	LOG_INF("All LEDs running independently with a single kernel timer.");

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
