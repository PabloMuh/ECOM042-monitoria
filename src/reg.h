/*******************************************************************
 * @file reg.h
 *
 * @brief Interface de manipulação de bits de registrador (Atividade-05).
 *******************************************************************/

#ifndef REG_H_
#define REG_H_

#include <stdbool.h>
#include <stdint.h>

void reg_set_bit(uint8_t *reg, uint8_t bit);
void reg_clear_bit(uint8_t *reg, uint8_t bit);
bool reg_read_bit(uint8_t reg, uint8_t bit);

#endif /* REG_H_ */
