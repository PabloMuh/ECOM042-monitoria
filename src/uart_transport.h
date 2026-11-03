/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UART_TRANSPORT_H_
#define UART_TRANSPORT_H_

#include <stdint.h>
#include <zephyr/kernel.h>

/**
 * @brief Inicializa a UART1 em modo interrupcao e configura a fila de RX.
 *
 * @return 0 em caso de sucesso, codigo negativo de erro caso contrario.
 */
int uart_transport_init(void);

/**
 * @brief Le um byte recebido da fila de recepcao da UART.
 *
 * @param byte Ponteiro onde o byte lido sera armazenado.
 * @param timeout Tempo limite de espera.
 * @return 0 se um byte foi lido, negativo em caso de timeout/erro.
 */
int uart_transport_read_byte(uint8_t *byte, k_timeout_t timeout);

/**
 * @brief Transmite um caractere pela UART1.
 *
 * @param c Caractere a ser transmitido.
 */
void uart_transport_send_char(char c);

/**
 * @brief Transmite uma string terminada em null pela UART1.
 *
 * @param str String a ser transmitida.
 */
void uart_transport_send_str(const char *str);

#endif /* UART_TRANSPORT_H_ */
