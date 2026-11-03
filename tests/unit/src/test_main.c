// SPDX-License-Identifier: Apache-2.0

#include "led.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static const struct device *const gpio_dev = DEVICE_DT_GET(DT_NODELABEL(test_gpio));

#define LED0_PIN 0
#define LED1_PIN 1
#define LED2_PIN 2

ZTEST(led_suite, test_01_init_fails_when_not_ready)
{
	/*
	 * test_gpio has 'zephyr,deferred-init;', so at boot it is not initialized.
	 * Check device_is_ready is false, then verify led_init() fails with -ENODEV.
	 */
	if (!device_is_ready(gpio_dev)) {
		int ret = led_init();

		zassert_equal(ret, -ENODEV,
			      "Expected -ENODEV when GPIO controller is not ready, got %d", ret);

		/* Now initialize the device manually */
		ret = device_init(gpio_dev);
		zassert_equal(ret, 0, "Failed to initialize test_gpio device: %d", ret);
	}

	zassert_true(device_is_ready(gpio_dev), "test_gpio should now be ready");
}

ZTEST(led_suite, test_02_init_success_and_starts_off)
{
	int ret = led_init();
	size_t count;

	zassert_equal(ret, 0, "led_init() should succeed, got %d", ret);

	count = led_get_count();
	zassert_true(count >= 2, "Expected at least 2 LEDs, got %u", (unsigned int)count);

	for (size_t i = 0; i < count; i++) {
		int val = gpio_emul_output_get(gpio_dev, (gpio_pin_t)i);
		const struct led_data *st = led_get_state((int)i);

		zassert_equal(val, 0, "LED %u should start turned off (level 0), got %d",
			      (unsigned int)i, val);
		zassert_not_null(st, "LED state should not be NULL");
		zassert_false(st->is_blinking, "LED %u should not be blinking after init",
			      (unsigned int)i);
	}

	/* Verify calling led_init() again does not corrupt anything */
	ret = led_init();
	zassert_equal(ret, 0, "Repeated led_init() should succeed, got %d", ret);
	for (size_t i = 0; i < count; i++) {
		int val = gpio_emul_output_get(gpio_dev, (gpio_pin_t)i);

		zassert_equal(val, 0, "LED %u should remain off after re-init", (unsigned int)i);
	}
}

ZTEST(led_suite, test_03_on_off_toggle)
{
	int ret;

	/* Test LED 0 */
	ret = led_on(0);
	zassert_equal(ret, 0, "led_on(0) failed: %d", ret);
	zassert_equal(gpio_emul_output_get(gpio_dev, LED0_PIN), 1,
		      "LED 0 pin should be 1 after led_on");

	ret = led_off(0);
	zassert_equal(ret, 0, "led_off(0) failed: %d", ret);
	zassert_equal(gpio_emul_output_get(gpio_dev, LED0_PIN), 0,
		      "LED 0 pin should be 0 after led_off");

	ret = led_toggle(0);
	zassert_equal(ret, 0, "led_toggle(0) failed: %d", ret);
	zassert_equal(gpio_emul_output_get(gpio_dev, LED0_PIN), 1,
		      "LED 0 pin should be 1 after toggle");

	ret = led_toggle(0);
	zassert_equal(ret, 0, "led_toggle(0) failed: %d", ret);
	zassert_equal(gpio_emul_output_get(gpio_dev, LED0_PIN), 0,
		      "LED 0 pin should be 0 after second toggle");
}

ZTEST(led_suite, test_04_blink_observed_high_and_low)
{
	int ret = led_blink(0, LED_PATTERN_FAST);
	bool saw_high = false;
	bool saw_low = false;

	zassert_equal(ret, 0, "led_blink(0, FAST) failed: %d", ret);

	/*
	 * FAST pattern period is 40ms (20ms high, 20ms low).
	 * Sample over ~160ms with 5ms sleep intervals.
	 */
	for (int i = 0; i < 32; i++) {
		int val = gpio_emul_output_get(gpio_dev, LED0_PIN);

		if (val == 1) {
			saw_high = true;
		}
		if (val == 0) {
			saw_low = true;
		}
		if (saw_high && saw_low) {
			break;
		}
		k_msleep(5);
	}

	zassert_true(saw_high, "Blinking LED 0 was never observed HIGH");
	zassert_true(saw_low, "Blinking LED 0 was never observed LOW");

	/* Clean up */
	led_off(0);
}

ZTEST(led_suite, test_05_two_leds_different_frequencies)
{
	int toggles_fast = 0;
	int toggles_slow = 0;
	int last_fast;
	int last_slow;

	/* Stop all LEDs first */
	led_off(LED_ID_ALL);

	/* LED 0 FAST (40ms period = toggle every 20ms) */
	zassert_ok(led_blink(0, LED_PATTERN_FAST));
	/* LED 1 SLOW (200ms period = toggle every 100ms) */
	zassert_ok(led_blink(1, LED_PATTERN_SLOW));

	last_fast = gpio_emul_output_get(gpio_dev, LED0_PIN);
	last_slow = gpio_emul_output_get(gpio_dev, LED1_PIN);

	/* Sample over 300ms at 5ms resolution */
	for (int i = 0; i < 60; i++) {
		int cur_fast;
		int cur_slow;

		k_msleep(5);
		cur_fast = gpio_emul_output_get(gpio_dev, LED0_PIN);
		cur_slow = gpio_emul_output_get(gpio_dev, LED1_PIN);

		if (cur_fast != last_fast) {
			toggles_fast++;
			last_fast = cur_fast;
		}
		if (cur_slow != last_slow) {
			toggles_slow++;
			last_slow = cur_slow;
		}
	}

	zassert_true(toggles_fast > toggles_slow,
		     "Fast LED should toggle more times than Slow LED (fast=%d, slow=%d)",
		     toggles_fast, toggles_slow);

	led_off(LED_ID_ALL);
}

ZTEST(led_suite, test_06_cancel_blink_with_actions)
{
	int before_toggle;
	int after_toggle;

	/* 1. Cancel via led_off */
	zassert_ok(led_blink(0, LED_PATTERN_FAST));
	zassert_true(led_get_state(0)->is_blinking);
	k_msleep(15);
	zassert_ok(led_off(0));
	zassert_false(led_get_state(0)->is_blinking, "Blink state was not cancelled by led_off");
	zassert_equal(gpio_emul_output_get(gpio_dev, LED0_PIN), 0);

	/* Wait 60ms (1.5x fast period) and ensure it stayed 0 */
	k_msleep(60);
	zassert_equal(gpio_emul_output_get(gpio_dev, LED0_PIN), 0,
		      "LED 0 pin toggled after led_off");

	/* 2. Cancel via led_on */
	zassert_ok(led_blink(0, LED_PATTERN_FAST));
	zassert_true(led_get_state(0)->is_blinking);
	k_msleep(15);
	zassert_ok(led_on(0));
	zassert_false(led_get_state(0)->is_blinking, "Blink state was not cancelled by led_on");
	zassert_equal(gpio_emul_output_get(gpio_dev, LED0_PIN), 1);

	/* Wait 60ms and ensure it stayed 1 */
	k_msleep(60);
	zassert_equal(gpio_emul_output_get(gpio_dev, LED0_PIN), 1,
		      "LED 0 pin toggled after led_on");

	/* 3. Cancel via led_toggle */
	zassert_ok(led_blink(0, LED_PATTERN_FAST));
	zassert_true(led_get_state(0)->is_blinking);
	k_msleep(15);
	before_toggle = gpio_emul_output_get(gpio_dev, LED0_PIN);
	zassert_ok(led_toggle(0));
	zassert_false(led_get_state(0)->is_blinking, "Blink state was not cancelled by led_toggle");
	after_toggle = gpio_emul_output_get(gpio_dev, LED0_PIN);
	zassert_equal(after_toggle, !before_toggle, "Pin did not invert on toggle");

	/* Wait 60ms and ensure it stayed in toggled state */
	k_msleep(60);
	zassert_equal(gpio_emul_output_get(gpio_dev, LED0_PIN), after_toggle,
		      "LED 0 pin toggled after led_toggle");

	led_off(LED_ID_ALL);
}

ZTEST(led_suite, test_07_action_on_all_leds)
{
	size_t count = led_get_count();

	/* 1. Turn ON all LEDs */
	zassert_ok(led_on(LED_ID_ALL));
	for (size_t i = 0; i < count; i++) {
		zassert_equal(gpio_emul_output_get(gpio_dev, (gpio_pin_t)i), 1,
			      "LED %u was not turned ON by LED_ID_ALL", (unsigned int)i);
		zassert_false(led_get_state((int)i)->is_blinking);
	}

	/* 2. Turn OFF all LEDs */
	zassert_ok(led_off(LED_ID_ALL));
	for (size_t i = 0; i < count; i++) {
		zassert_equal(gpio_emul_output_get(gpio_dev, (gpio_pin_t)i), 0,
			      "LED %u was not turned OFF by LED_ID_ALL", (unsigned int)i);
		zassert_false(led_get_state((int)i)->is_blinking);
	}

	/* 3. Toggle all LEDs */
	zassert_ok(led_toggle(LED_ID_ALL));
	for (size_t i = 0; i < count; i++) {
		zassert_equal(gpio_emul_output_get(gpio_dev, (gpio_pin_t)i), 1,
			      "LED %u was not toggled by LED_ID_ALL", (unsigned int)i);
		zassert_false(led_get_state((int)i)->is_blinking);
	}

	/* 4. Blink all LEDs */
	zassert_ok(led_blink(LED_ID_ALL, LED_PATTERN_NORMAL));
	for (size_t i = 0; i < count; i++) {
		zassert_true(led_get_state((int)i)->is_blinking,
			     "LED %u is not blinking after led_blink(LED_ID_ALL)", (unsigned int)i);
	}

	/* Clean up */
	zassert_ok(led_off(LED_ID_ALL));
	for (size_t i = 0; i < count; i++) {
		zassert_false(led_get_state((int)i)->is_blinking);
		zassert_equal(gpio_emul_output_get(gpio_dev, (gpio_pin_t)i), 0);
	}
}

ZTEST(led_suite, test_08_invalid_arguments)
{
	size_t count = led_get_count();

	/* Invalid LED IDs */
	zassert_equal(led_on(-2), -EINVAL);
	zassert_equal(led_on((int)count), -EINVAL);
	zassert_equal(led_on(99), -EINVAL);

	zassert_equal(led_off(-3), -EINVAL);
	zassert_equal(led_off((int)count + 1), -EINVAL);

	zassert_equal(led_toggle(-10), -EINVAL);
	zassert_equal(led_toggle((int)count), -EINVAL);

	/* Invalid patterns */
	zassert_equal(led_blink(0, -1), -EINVAL);
	zassert_equal(led_blink(0, LED_PATTERN_COUNT), -EINVAL);
	zassert_equal(led_blink(0, 999), -EINVAL);

	/* Invalid LED ID with valid pattern */
	zassert_equal(led_blink(-2, LED_PATTERN_SLOW), -EINVAL);
	zassert_equal(led_blink((int)count, LED_PATTERN_FAST), -EINVAL);

	/* Invalid action */
	zassert_equal(led_apply_action(0, LED_ACTION_COUNT, NULL), -EINVAL);
	zassert_equal(led_apply_action(0, -1, NULL), -EINVAL);
}

ZTEST_SUITE(led_suite, NULL, NULL, NULL, NULL, NULL);
