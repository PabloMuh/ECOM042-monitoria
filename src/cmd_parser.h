/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CMD_PARSER_H_
#define CMD_PARSER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef CONFIG_CMD_BUFFER_SIZE
#define CONFIG_CMD_BUFFER_SIZE 64
#endif

typedef int (*cmd_handler_t)(const char *args, char *out_buf, size_t out_buf_len);

struct cmd_entry {
	const char *name;
	cmd_handler_t handler;
};

typedef enum {
	CMD_FEED_CONTINUE = 0,
	CMD_FEED_COMPLETE,
	CMD_FEED_OVERFLOW,
	CMD_FEED_BACKSPACE,
	CMD_FEED_IGNORED
} cmd_feed_res_t;

struct cmd_line_ctx {
	char buf[CONFIG_CMD_BUFFER_SIZE + 1];
	size_t len;
	bool overflow;
	bool last_was_cr;
};

/**
 * @brief Inicializa o contexto de montagem de linha de comando.
 */
void cmd_line_init(struct cmd_line_ctx *ctx);

/**
 * @brief Processa um caractere recebido para acumulacao de linha, backspace e overflow.
 *
 * @param ctx Contexto da linha.
 * @param c Caractere recebido.
 * @param out_line Buffer de saida para a linha terminada (quando retorna CMD_FEED_COMPLETE).
 * @param out_line_len Capacidade do buffer de saida.
 * @return cmd_feed_res_t Acao resultante.
 */
cmd_feed_res_t cmd_line_feed(struct cmd_line_ctx *ctx, char c, char *out_line, size_t out_line_len);

/**
 * @brief Inicializa o despachante de comandos com a tabela de ponteiros de funcao.
 *
 * @param table Vetor de entradas de comandos.
 * @param count Quantidade de entradas no vetor.
 * @return 0 em caso de sucesso, negativo caso contrario.
 */
int cmd_parser_init(const struct cmd_entry *table, size_t count);

/**
 * @brief Processa uma linha de comando e escreve a resposta textual em out_buf.
 *
 * @param line Linha de entrada terminada em null (sem \\r ou \\n).
 * @param out_buf Buffer onde a resposta sera gravada.
 * @param out_buf_len Capacidade maxima do buffer de resposta.
 * @return 0 em caso de comando despachado com sucesso, ou codigo de erro.
 */
int cmd_parser_process(const char *line, char *out_buf, size_t out_buf_len);

#endif /* CMD_PARSER_H_ */
