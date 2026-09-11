/*******************************************************************
 * @file main.c
 *
 * @brief Main file.
 * @author João Matheus Nascimento Dias (jmnd@ic.ufal.br)
 * @author José Félix de Oliveira Neto (jfon@ic.ufal.br)
 * @version 0.1
 * @date 26/08/2026
 *******************************************************************/

#include <zephyr/kernel.h>

#include "reg.h"

int main(void)
{
	uint8_t reg = 0x00;

	reg_set_bit(&reg, 3);
	reg_set_bit(&reg, 0);
	reg_clear_bit(&reg, 3);

	printk("reg = 0x%02X\n", reg);

	return 0;
}
