// SPDX-License-Identifier: Apache-2.0

#include "led.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(led, CONFIG_LED_LOG_LEVEL);

#ifndef CONFIG_LED_PERIOD_SLOW_MS
#define CONFIG_LED_PERIOD_SLOW_MS 1000
#endif

#ifndef CONFIG_LED_PERIOD_NORMAL_MS
#define CONFIG_LED_PERIOD_NORMAL_MS 500
#endif

#ifndef CONFIG_LED_PERIOD_FAST_MS
#define CONFIG_LED_PERIOD_FAST_MS 200
#endif

/*
 * Devicetree specifications for LEDs obtained from aliases led0, led1, ...
 */
static const struct gpio_dt_spec led_specs[] = {
#if DT_NODE_EXISTS(DT_ALIAS(led0))
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led1))
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led2))
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led3))
	GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led4))
	GPIO_DT_SPEC_GET(DT_ALIAS(led4), gpios),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led5))
	GPIO_DT_SPEC_GET(DT_ALIAS(led5), gpios),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led6))
	GPIO_DT_SPEC_GET(DT_ALIAS(led6), gpios),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led7))
	GPIO_DT_SPEC_GET(DT_ALIAS(led7), gpios),
#endif
};

#define LED_COUNT ARRAY_SIZE(led_specs)

BUILD_ASSERT(ARRAY_SIZE(led_specs) >= 2,
	     "Devicetree must define at least 2 LEDs (aliases led0, led1, ...)");

/*
 * Table-driven blink pattern periods (configured via Kconfig, no magic numbers)
 */
static const uint32_t pattern_periods_ms[LED_PATTERN_COUNT] = {
	[LED_PATTERN_SLOW] = CONFIG_LED_PERIOD_SLOW_MS,
	[LED_PATTERN_NORMAL] = CONFIG_LED_PERIOD_NORMAL_MS,
	[LED_PATTERN_FAST] = CONFIG_LED_PERIOD_FAST_MS,
};

/*
 * State operations interface (Table-driven pattern instead of switch/if-else on state)
 */
struct led_state_ops {
	void (*timer_tick)(int idx, int64_t now);
};

static void blink_timer_tick(int idx, int64_t now);

enum led_state {
	LED_STATE_OFF = 0,
	LED_STATE_ON,
	LED_STATE_BLINKING,
	LED_STATE_COUNT
};

static const struct led_state_ops state_ops[LED_STATE_COUNT] = {
	[LED_STATE_OFF] = {.timer_tick = NULL},
	[LED_STATE_ON] = {.timer_tick = NULL},
	[LED_STATE_BLINKING] = {.timer_tick = blink_timer_tick},
};

/*
 * Single multiplexed kernel timer for all LEDs.
 */
static struct k_timer led_timer;
static struct k_spinlock lock;

/*
 * State structure array indexed by LED ID
 */
static struct led_data led_states[LED_COUNT];

/*
 * Re-arm the single one-shot timer to the earliest upcoming deadline.
 * Must be called with lock held.
 */
static void rearm_timer_locked(int64_t now)
{
	int64_t earliest_deadline = INT64_MAX;
	bool has_blinking = false;

	for (size_t i = 0; i < LED_COUNT; i++) {
		if (led_states[i].is_blinking) {
			has_blinking = true;
			if (led_states[i].deadline_ticks < earliest_deadline) {
				earliest_deadline = led_states[i].deadline_ticks;
			}
		}
	}

	if (has_blinking && earliest_deadline != INT64_MAX) {
		int64_t diff = earliest_deadline - now;

		if (diff < 1) {
			diff = 1;
		}
		k_timer_start(&led_timer, K_TICKS(diff), K_NO_WAIT);
	} else {
		k_timer_stop(&led_timer);
	}
}

/*
 * Blink tick callback executed in ISR context. Short, no logs/printk.
 */
static void blink_timer_tick(int idx, int64_t now)
{
	if (now >= led_states[idx].deadline_ticks) {
		int64_t half_period_ticks = k_ms_to_ticks_ceil32(led_states[idx].period_ms / 2);

		gpio_pin_toggle_dt(&led_specs[idx]);
		led_states[idx].deadline_ticks += half_period_ticks;
		if (led_states[idx].deadline_ticks <= now) {
			led_states[idx].deadline_ticks = now + half_period_ticks;
		}
	}
}

/*
 * Timer ISR callback. Fast: processes ticks, computes deadlines, rearms timer.
 */
static void led_timer_callback(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	k_spinlock_key_t key = k_spin_lock(&lock);
	int64_t now = k_uptime_ticks();

	for (size_t i = 0; i < LED_COUNT; i++) {
		if (led_states[i].ops != NULL && led_states[i].ops->timer_tick != NULL) {
			led_states[i].ops->timer_tick((int)i, now);
		}
	}

	rearm_timer_locked(now);

	k_spin_unlock(&lock, key);
}

/*
 * Command handlers (Command Pattern)
 */
typedef int (*led_cmd_fn)(int idx, void *arg);

static int cmd_turn_on(int idx, void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	led_states[idx].ops = &state_ops[LED_STATE_ON];
	led_states[idx].is_blinking = false;
	rearm_timer_locked(k_uptime_ticks());
	k_spin_unlock(&lock, key);

	return gpio_pin_set_dt(&led_specs[idx], 1);
}

static int cmd_turn_off(int idx, void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	led_states[idx].ops = &state_ops[LED_STATE_OFF];
	led_states[idx].is_blinking = false;
	rearm_timer_locked(k_uptime_ticks());
	k_spin_unlock(&lock, key);

	return gpio_pin_set_dt(&led_specs[idx], 0);
}

static int cmd_toggle(int idx, void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	led_states[idx].ops = &state_ops[LED_STATE_OFF];
	led_states[idx].is_blinking = false;
	rearm_timer_locked(k_uptime_ticks());
	k_spin_unlock(&lock, key);

	return gpio_pin_toggle_dt(&led_specs[idx]);
}

static int cmd_blink(int idx, void *arg)
{
	enum led_pattern pattern = (enum led_pattern)(intptr_t)arg;
	uint32_t period = pattern_periods_ms[pattern];
	int64_t half_period_ticks = k_ms_to_ticks_ceil32(period / 2);

	k_spinlock_key_t key = k_spin_lock(&lock);
	int64_t now = k_uptime_ticks();

	led_states[idx].ops = &state_ops[LED_STATE_BLINKING];
	led_states[idx].is_blinking = true;
	led_states[idx].pattern = pattern;
	led_states[idx].period_ms = period;
	led_states[idx].deadline_ticks = now + half_period_ticks;

	rearm_timer_locked(now);
	k_spin_unlock(&lock, key);

	return gpio_pin_set_dt(&led_specs[idx], 1);
}

static const led_cmd_fn cmd_table[LED_ACTION_COUNT] = {
	[LED_ACTION_ON] = cmd_turn_on,
	[LED_ACTION_OFF] = cmd_turn_off,
	[LED_ACTION_TOGGLE] = cmd_toggle,
	[LED_ACTION_BLINK] = cmd_blink,
};

int led_init(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	k_timer_stop(&led_timer);
	k_spin_unlock(&lock, key);

	for (size_t i = 0; i < LED_COUNT; i++) {
		if (!gpio_is_ready_dt(&led_specs[i])) {
			LOG_ERR("GPIO controller for LED %u not ready", (unsigned int)i);
			return -ENODEV;
		}
	}

	for (size_t i = 0; i < LED_COUNT; i++) {
		int ret = gpio_pin_configure_dt(&led_specs[i], GPIO_OUTPUT_INACTIVE);

		if (ret < 0) {
			LOG_ERR("Failed to configure GPIO for LED %u: %d", (unsigned int)i, ret);
			return ret;
		}
	}

	key = k_spin_lock(&lock);
	for (size_t i = 0; i < LED_COUNT; i++) {
		led_states[i].ops = &state_ops[LED_STATE_OFF];
		led_states[i].is_blinking = false;
		led_states[i].pattern = (enum led_pattern)0;
		led_states[i].period_ms = 0;
		led_states[i].deadline_ticks = 0;
	}
	k_timer_init(&led_timer, led_timer_callback, NULL);
	k_spin_unlock(&lock, key);

	LOG_INF("LED module initialized successfully (%u LEDs)", (unsigned int)LED_COUNT);
	return 0;
}

int led_apply_action(int led_id, enum led_action action, void *arg)
{
	led_cmd_fn handler;

	if (action < 0 || action >= LED_ACTION_COUNT) {
		LOG_ERR("Invalid action: %d", action);
		return -EINVAL;
	}

	if (led_id != LED_ID_ALL && (led_id < 0 || led_id >= (int)LED_COUNT)) {
		LOG_ERR("Invalid LED ID: %d (LED count: %u)", led_id, (unsigned int)LED_COUNT);
		return -EINVAL;
	}

	handler = cmd_table[action];

	if (led_id == LED_ID_ALL) {
		int ret = 0;

		for (int i = 0; i < (int)LED_COUNT; i++) {
			int err = handler(i, arg);

			if (err != 0 && ret == 0) {
				ret = err;
			}
		}
		return ret;
	}

	return handler(led_id, arg);
}

int led_on(int led_id)
{
	return led_apply_action(led_id, LED_ACTION_ON, NULL);
}

int led_off(int led_id)
{
	return led_apply_action(led_id, LED_ACTION_OFF, NULL);
}

int led_toggle(int led_id)
{
	return led_apply_action(led_id, LED_ACTION_TOGGLE, NULL);
}

int led_blink(int led_id, enum led_pattern pattern)
{
	if (pattern < 0 || pattern >= LED_PATTERN_COUNT) {
		LOG_ERR("Invalid blink pattern: %d", pattern);
		return -EINVAL;
	}

	return led_apply_action(led_id, LED_ACTION_BLINK, (void *)(intptr_t)pattern);
}

size_t led_get_count(void)
{
	return LED_COUNT;
}

const struct led_data *led_get_state(int led_id)
{
	if (led_id < 0 || led_id >= (int)LED_COUNT) {
		return NULL;
	}
	return &led_states[led_id];
}
