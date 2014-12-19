#ifndef STAK_SEPS114A_H
#define STAK_SEPS114A_H
#include <stdint.h>

typedef struct {
	uint16_t* framebuffer;
	int spi_fd;
}stak_seps114a_s;

stak_seps114a_s* stak_seps114a_create();
int stak_seps114a_destroy(stak_seps114a_s* device);
int stak_seps114a_update(stak_seps114a_s* device);

int stak_seps114a_write_byte(stak_seps114a_s* device, char data_value);
int stak_seps114a_write_data(stak_seps114a_s* device, char* data_value, uint32_t size);
int stak_seps114a_write_command(stak_seps114a_s* device, char reg_index);
int stak_seps114a_write_command_value(stak_seps114a_s* device, char reg_index, char reg_value);

#endif