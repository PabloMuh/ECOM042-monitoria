/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cmd_parser.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

static const struct cmd_entry *cmd_table;
static size_t cmd_table_count;

void cmd_line_init(struct cmd_line_ctx *ctx)
{
	if (ctx == NULL) {
		return;
	}
	ctx->len = 0;
	ctx->overflow = false;
	ctx->last_was_cr = false;
	ctx->buf[0] = '\0';
}

cmd_feed_res_t cmd_line_feed(struct cmd_line_ctx *ctx, char c, char *out_line, size_t out_line_len)
{
	if (ctx == NULL) {
		return CMD_FEED_IGNORED;
	}

	/* Tratamento de LF apos CR (CRLF) */
	if (ctx->last_was_cr && c == '\n') {
		ctx->last_was_cr = false;
		return CMD_FEED_IGNORED;
	}
	ctx->last_was_cr = (c == '\r');

	/* Fim de linha */
	if (c == '\r' || c == '\n') {
		if (ctx->overflow) {
			ctx->len = 0;
			ctx->overflow = false;
			ctx->buf[0] = '\0';
			return CMD_FEED_OVERFLOW;
		}

		if (out_line != NULL && out_line_len > 0) {
			strncpy(out_line, ctx->buf, out_line_len - 1);
			out_line[out_line_len - 1] = '\0';
		}

		ctx->len = 0;
		ctx->buf[0] = '\0';
		return CMD_FEED_COMPLETE;
	}

	/* Backspace (0x08 = BS, 0x7F = DEL) */
	if (c == 0x08 || c == 0x7F) {
		if (ctx->overflow) {
			return CMD_FEED_IGNORED;
		}
		if (ctx->len > 0) {
			ctx->len--;
			ctx->buf[ctx->len] = '\0';
			return CMD_FEED_BACKSPACE;
		}
		return CMD_FEED_IGNORED;
	}

	/* Ignora caracteres de controle que nao sejam espaco/caracteres imprimiveis */
	if ((unsigned char)c < 32 || (unsigned char)c == 127) {
		return CMD_FEED_IGNORED;
	}

	/* Caractere normal */
	if (ctx->len < CONFIG_CMD_BUFFER_SIZE) {
		ctx->buf[ctx->len++] = c;
		ctx->buf[ctx->len] = '\0';
		return CMD_FEED_CONTINUE;
	} else {
		ctx->overflow = true;
		return CMD_FEED_IGNORED;
	}
}

int cmd_parser_init(const struct cmd_entry *table, size_t count)
{
	cmd_table = table;
	cmd_table_count = count;
	return 0;
}

int cmd_parser_process(const char *line, char *out_buf, size_t out_buf_len)
{
	if (out_buf == NULL || out_buf_len == 0) {
		return -EINVAL;
	}

	out_buf[0] = '\0';

	if (line == NULL) {
		return 0;
	}

	if (strlen(line) > CONFIG_CMD_BUFFER_SIZE) {
		snprintf(out_buf, out_buf_len, "ERR: line too long");
		return -EINVAL;
	}

	/* Pula espacos iniciais */
	const char *p = line;

	while (*p == ' ') {
		p++;
	}

	if (*p == '\0') {
		/* Linha vazia nao gera resposta de erro */
		return 0;
	}

	/* Extrai o verbo (comando) */
	const char *verb_start = p;

	while (*p != '\0' && *p != ' ') {
		p++;
	}
	size_t verb_len = p - verb_start;

	/* Extrai os argumentos, avancando apenas o espaco delimitador */
	const char *args = NULL;

	if (*p == ' ') {
		args = p + 1;
	}

	if (cmd_table != NULL) {
		for (size_t i = 0; i < cmd_table_count; i++) {
			if (cmd_table[i].name == NULL) {
				continue;
			}
			size_t name_len = strlen(cmd_table[i].name);

			if (verb_len == name_len &&
			    strncasecmp(verb_start, cmd_table[i].name, verb_len) == 0) {
				if (cmd_table[i].handler != NULL) {
					return cmd_table[i].handler(args, out_buf, out_buf_len);
				}
			}
		}
	}

	snprintf(out_buf, out_buf_len, "ERR: unknown command");
	return -ENOENT;
}
