/* SPDX-License-Identifier: Apache-2.0 */

#ifndef LED_H_
#define LED_H_

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Special identifier to apply an action to all configured LEDs.
 */
#define LED_ID_ALL (-1)

/**
 * @brief Predefined blink patterns.
 */
enum led_pattern {
	LED_PATTERN_SLOW = 0,
	LED_PATTERN_NORMAL,
	LED_PATTERN_FAST,
	LED_PATTERN_COUNT
};

/**
 * @brief Command / Action identifiers for table-driven execution.
 */
enum led_action {
	LED_ACTION_ON = 0,
	LED_ACTION_OFF,
	LED_ACTION_TOGGLE,
	LED_ACTION_BLINK,
	LED_ACTION_COUNT
};

struct led_state_ops;

/**
 * @brief Individual LED state structure.
 *
 * Indexed by LED ID. Stores current pattern, period, next deadline,
 * and whether the LED is currently blinking.
 */
struct led_data {
	const struct led_state_ops *ops;
	enum led_pattern pattern;
	uint32_t period_ms;
	int64_t deadline_ticks;
	bool is_blinking;
};

/**
 * @brief Initialize the LED module.
 *
 * Verifies that all GPIO controllers are ready and configures all LEDs
 * as outputs in the OFF (inactive) state.
 * Safe to be called multiple times without corrupting the timer.
 *
 * @retval 0 On success.
 * @retval -ENODEV If any GPIO controller is not ready.
 * @retval Negative errno on pin configuration failure.
 */
int led_init(void);

/**
 * @brief Turn ON a LED (or all LEDs if id is LED_ID_ALL).
 *
 * Cancels any ongoing blink on the target LED(s).
 *
 * @param led_id 0-based LED index or LED_ID_ALL.
 * @retval 0 On success.
 * @retval -EINVAL If led_id is invalid.
 */
int led_on(int led_id);

/**
 * @brief Turn OFF a LED (or all LEDs if id is LED_ID_ALL).
 *
 * Cancels any ongoing blink on the target LED(s).
 *
 * @param led_id 0-based LED index or LED_ID_ALL.
 * @retval 0 On success.
 * @retval -EINVAL If led_id is invalid.
 */
int led_off(int led_id);

/**
 * @brief Toggle the state of a LED (or all LEDs if id is LED_ID_ALL).
 *
 * Cancels any ongoing blink on the target LED(s).
 *
 * @param led_id 0-based LED index or LED_ID_ALL.
 * @retval 0 On success.
 * @retval -EINVAL If led_id is invalid.
 */
int led_toggle(int led_id);

/**
 * @brief Start blinking a LED (or all LEDs if id is LED_ID_ALL) with chosen pattern.
 *
 * @param led_id 0-based LED index or LED_ID_ALL.
 * @param pattern One of the patterns from enum led_pattern.
 * @retval 0 On success.
 * @retval -EINVAL If led_id or pattern is invalid.
 */
int led_blink(int led_id, enum led_pattern pattern);

/**
 * @brief Dispatch any action to a single LED or all LEDs.
 *
 * Central dispatcher helper for single or all LEDs using the Command Pattern.
 *
 * @param led_id 0-based LED index or LED_ID_ALL.
 * @param action One of the actions from enum led_action.
 * @param arg Optional action argument (e.g., enum led_pattern for BLINK).
 * @retval 0 On success.
 * @retval -EINVAL If led_id or action is invalid.
 */
int led_apply_action(int led_id, enum led_action action, void *arg);

/**
 * @brief Get the number of LEDs defined in devicetree.
 *
 * @return Total number of available LEDs.
 */
size_t led_get_count(void);

/**
 * @brief Query the current state of a LED.
 *
 * @param led_id 0-based LED index.
 * @return Pointer to struct led_data, or NULL if led_id is invalid.
 */
const struct led_data *led_get_state(int led_id);

#ifdef __cplusplus
}
#endif

#endif /* LED_H_ */
