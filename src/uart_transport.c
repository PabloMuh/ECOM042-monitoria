/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uart_transport.h"

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_transport, CONFIG_LOG_DEFAULT_LEVEL);

#define UART_NODE DT_NODELABEL(uart1)

static const struct device *const uart_dev = DEVICE_DT_GET(UART_NODE);

K_MSGQ_DEFINE(uart_rx_msgq, sizeof(uint8_t), 128, 4);

static void uart_fifo_callback(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	uart_irq_update(dev);

	while (uart_irq_rx_ready(dev)) {
		uint8_t c;

		if (uart_fifo_read(dev, &c, 1) <= 0) {
			break;
		}

		/* A ISR apenas enfileira o byte, sem parsing */
		(void)k_msgq_put(&uart_rx_msgq, &c, K_NO_WAIT);
	}
}

int uart_transport_init(void)
{
	int ret;

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("Dispositivo UART1 nao esta pronto");
		return -ENODEV;
	}

	ret = uart_irq_callback_user_data_set(uart_dev, uart_fifo_callback, NULL);
	if (ret < 0) {
		LOG_ERR("Falha ao registrar callback de interrupcao UART (%d)", ret);
		return ret;
	}

	uart_irq_rx_enable(uart_dev);
	LOG_INF("UART1 inicializada com sucesso em modo interrupcao");
	return 0;
}

int uart_transport_read_byte(uint8_t *byte, k_timeout_t timeout)
{
	if (byte == NULL) {
		return -EINVAL;
	}

	return k_msgq_get(&uart_rx_msgq, byte, timeout);
}

void uart_transport_send_char(char c)
{
	uart_poll_out(uart_dev, (unsigned char)c);
}

void uart_transport_send_str(const char *str)
{
	if (str == NULL) {
		return;
	}

	while (*str != '\0') {
		uart_transport_send_char(*str);
		str++;
	}
}
