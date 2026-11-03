/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cmd_parser.h"
#include "shell.h"
#include "uart_transport.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(shell, CONFIG_LOG_DEFAULT_LEVEL);

#define SHELL_STACK_SIZE 2048
#define SHELL_PRIORITY   7

static void shell_thread_entry(void *p1, void *p2, void *p3);

K_THREAD_DEFINE(shell_thread_id, SHELL_STACK_SIZE, shell_thread_entry, NULL, NULL, NULL,
		SHELL_PRIORITY, 0, K_TICKS_FOREVER);

static void shell_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct cmd_line_ctx line_ctx;
	char line[CONFIG_CMD_BUFFER_SIZE + 1];
	char resp[256];
	uint8_t byte;

	cmd_line_init(&line_ctx);

	/* Envia o prompt inicial */
	uart_transport_send_str("> ");

	while (1) {
		if (uart_transport_read_byte(&byte, K_FOREVER) != 0) {
			continue;
		}

		cmd_feed_res_t res = cmd_line_feed(&line_ctx, (char)byte, line, sizeof(line));

		switch (res) {
		case CMD_FEED_CONTINUE:
			/* Eco do caractere */
			uart_transport_send_char((char)byte);
			break;

		case CMD_FEED_BACKSPACE:
			/* Apaga o ultimo caractere no terminal */
			uart_transport_send_str("\b \b");
			break;

		case CMD_FEED_OVERFLOW:
			/* Linha maior que o buffer: descartada por inteiro com ERR: line too long
			 */
			uart_transport_send_str("\r\nERR: line too long\r\n> ");
			break;

		case CMD_FEED_COMPLETE:
			uart_transport_send_str("\r\n");
			if (line[0] != '\0') {
				(void)cmd_parser_process(line, resp, sizeof(resp));
				if (resp[0] != '\0') {
					uart_transport_send_str(resp);
					uart_transport_send_str("\r\n");
				}
			}
			uart_transport_send_str("> ");
			break;

		case CMD_FEED_IGNORED:
		default:
			break;
		}
	}
}

int shell_init(void)
{
	k_thread_start(shell_thread_id);
	LOG_INF("Thread da mini-shell iniciada");
	return 0;
}
