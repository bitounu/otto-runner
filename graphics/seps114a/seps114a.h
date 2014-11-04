#ifndef STAK_SEPS114A_H
#define STAK_SEPS114A_H
#include <stdint.h>

typedef struct {
	uint16_t* framebuffer;
}stak_seps114a_s;

int stak_seps114a_init(stak_seps114a_s* device);
int stak_seps114a_close(stak_seps114a_s* device);
int stak_seps114a_update(stak_seps114a_s* device);

int stak_seps114a_write_byte(char data_value);
int stak_seps114a_write_data(char* data_value, uint32_t size);
int stak_seps114a_write_command(unsigned char reg_index);
int stak_seps114a_write_command_value(unsigned char reg_index, unsigned char reg_value);

#endif