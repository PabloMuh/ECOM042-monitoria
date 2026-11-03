/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BOARD_IO_H_
#define BOARD_IO_H_

#include <stdbool.h>

/**
 * @brief Inicializa os pinos de GPIO do LED e do botao via Devicetree.
 *
 * @return 0 em caso de sucesso, codigo de erro negativo caso contrario.
 */
int board_io_init(void);

/**
 * @brief Altera o estado do LED (led0).
 *
 * @param on true para ligar, false para desligar.
 * @return 0 em caso de sucesso, codigo de erro negativo caso contrario.
 */
int board_led_set(bool on);

/**
 * @brief Consulta o estado atual do LED.
 *
 * @return 1 se ligado, 0 se desligado, codigo de erro negativo caso contrario.
 */
int board_led_get(void);

/**
 * @brief Le o estado do botao (sw0).
 *
 * @return 1 se pressionado/ativo, 0 se solto/inativo, negativo em erro.
 */
int board_btn_read(void);

#endif /* BOARD_IO_H_ */
