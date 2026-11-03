/*******************************************************************
 * @file main.c
 *
 * @brief Ponto de entrada da aplicacao Mini-shell via UART.
 *******************************************************************/

#include "board_io.h"
#include "cmd_parser.h"
#include "shell.h"
#include "uart_transport.h"

#include <errno.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <strings.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

static int cmd_help(const char *args, char *out_buf, size_t out_buf_len)
{
	if (args != NULL && *args != '\0') {
		snprintf(out_buf, out_buf_len, "ERR: unknown command");
		return -EINVAL;
	}

	snprintf(out_buf, out_buf_len,
		 "HELP\r\n"
		 "LED ON\r\n"
		 "LED OFF\r\n"
		 "LED\r\n"
		 "BTN\r\n"
		 "UPTIME\r\n"
		 "ECHO <texto>");
	return 0;
}

static int cmd_led(const char *args, char *out_buf, size_t out_buf_len)
{
	if (args == NULL || *args == '\0') {
		int state = board_led_get();

		snprintf(out_buf, out_buf_len, (state > 0) ? "LED: ON" : "LED: OFF");
		return 0;
	}

	while (*args == ' ') {
		args++;
	}

	if (strcasecmp(args, "ON") == 0) {
		board_led_set(true);
		snprintf(out_buf, out_buf_len, "OK");
		return 0;
	}

	if (strcasecmp(args, "OFF") == 0) {
		board_led_set(false);
		snprintf(out_buf, out_buf_len, "OK");
		return 0;
	}

	snprintf(out_buf, out_buf_len, "ERR: unknown command");
	return -EINVAL;
}

static int cmd_btn(const char *args, char *out_buf, size_t out_buf_len)
{
	if (args != NULL && *args != '\0') {
		snprintf(out_buf, out_buf_len, "ERR: unknown command");
		return -EINVAL;
	}

	int val = board_btn_read();

	snprintf(out_buf, out_buf_len, "BTN: %d", val > 0 ? 1 : 0);
	return 0;
}

static int cmd_uptime(const char *args, char *out_buf, size_t out_buf_len)
{
	if (args != NULL && *args != '\0') {
		snprintf(out_buf, out_buf_len, "ERR: unknown command");
		return -EINVAL;
	}

	int64_t uptime_ms = k_uptime_get();

	snprintf(out_buf, out_buf_len, "UPTIME: %lld ms", (long long)uptime_ms);
	return 0;
}

static int cmd_echo(const char *args, char *out_buf, size_t out_buf_len)
{
	snprintf(out_buf, out_buf_len, "%s", args ? args : "");
	return 0;
}

static const struct cmd_entry app_commands[] = {
	{.name = "HELP", .handler = cmd_help}, {.name = "LED", .handler = cmd_led},
	{.name = "BTN", .handler = cmd_btn},   {.name = "UPTIME", .handler = cmd_uptime},
	{.name = "ECHO", .handler = cmd_echo},
};

int main(void)
{
	int ret;

	LOG_INF("Inicializando aplicacao Mini-shell UART na placa %s", CONFIG_BOARD);

	ret = board_io_init();
	if (ret < 0) {
		LOG_ERR("Falha ao inicializar IOs da placa: %d", ret);
		return ret;
	}

	ret = cmd_parser_init(app_commands, ARRAY_SIZE(app_commands));
	if (ret < 0) {
		LOG_ERR("Falha ao inicializar parser de comandos: %d", ret);
		return ret;
	}

	ret = uart_transport_init();
	if (ret < 0) {
		LOG_ERR("Falha ao inicializar transporte UART: %d", ret);
		return ret;
	}

	ret = shell_init();
	if (ret < 0) {
		LOG_ERR("Falha ao inicializar shell: %d", ret);
		return ret;
	}

	LOG_INF("Mini-shell UART pronta para comandos");
	return 0;
}
