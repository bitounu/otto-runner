#include <io/bq27510/bq27510.h>
#include <linux/i2c-dev.h>
#include <bcm2835.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <errno.h>

#define STAK_USE_I2C_DEVICE

unsigned int transBytes2Int(unsigned char msb, unsigned char lsb)
{
	unsigned int tmp;

    tmp = ((msb << 8) & 0xFF00);
    return ((unsigned int)(tmp + lsb) & 0x0000FFFF);
}

int bq_write(stak_bq27510_device_s* device, unsigned char reg, char data)
{
        int err = 0;
	char i2c_buf[2];

  	i2c_buf[0] = reg;
	i2c_buf[1] = data;
	err = write(device->device_fd, i2c_buf, 2);

	if (err != 2) {
	      fprintf(stderr, "\nError: 0x%x - i2c transaction failure! a \n", err);
	      return -1;
	}
	
	return 0;
}

int bq_read(stak_bq27510_device_s* device, unsigned char reg, int bytes, char *i2c_buf)
{
        int err = 0;

	i2c_buf[0] = reg;
	err = write(device->device_fd, i2c_buf, 1);
	if (err != 1) {
	        fprintf(stderr, "\nError: 0x%x - i2c transaction failure! b \n", err);
	}

	err = read(device->device_fd, i2c_buf, bytes);
	if (err != bytes) {
	        fprintf(stderr, "\nError: 0x%x - i2c transaction failure! bb \n", err);
		return -1;
	}

	return 0;
}
int
i2c_read16(stak_bq27510_device_s* device, char reg) {
	char buf[3], rbuf[2] = {0x00, 0x00};
	struct i2c_rdwr_ioctl_data msgset;
	struct i2c_msg iomsgs[2] = {
		{
			.addr = (unsigned) 0x55,
			.flags = 0,
			.buf = buf,
			.len = 2
		},
		{

			.addr = (unsigned) 0x55,
			.flags = I2C_M_RD,
			.buf = rbuf,
			.len = 2
		}
	};
	int rc;

	//buf[0] = (reg & 0x00FF);
	buf[0] = (reg & 0xFF00) >> 8;
	buf[1] = (reg & 0x00FF);

	msgset.msgs = iomsgs;
	msgset.nmsgs = 2;

	rc = ioctl(device->device_fd,I2C_RDWR,&msgset);
	if(rc < 0) {
		return -1;
	}
	i2c_smbus_read_word_data(device->device_fd, reg);
	/*buf[0] = (reg);
	buf[1] = (reg);
	int err = write(device->device_fd, buf, 2);
	if (err != 2) {
		printf("No ACK bit! %i\n", err);
		return -1;
	}
	usleep(66);
	err = read(device->device_fd, rbuf, 2);
	if (err != 2) {
		printf("No ACK bit! %i\n", err);
		return -1;
	}
	usleep(66);*/
	return (rbuf[0] << 8) | rbuf[1];
}

int stak_bq27510_open(char* filename, stak_bq27510_device_s* device) {
#ifdef STAK_USE_I2C_DEVICE
	//unsigned long i2c_funcs = 0;
	device->device_fd = open("/dev/i2c-1", O_RDWR);

	if (device->device_fd < 0) {
	        printf("\nError: Failed to open /dev/i2c-1!\n");
	        return -1;
	}

	unsigned char i2c_address = 0x55;
	ioctl(device->device_fd, I2C_TENBIT, 0);
	ioctl(device->device_fd, I2C_SLAVE, i2c_address);

	float voltage = (float)i2c_read16(device, 0x08) / 1000.0f;
	int charge = i2c_read16(device, 0x20);

	printf("\tVoltage: %f\n\tCharge: %i%%\n", voltage, charge);

	/*err = write(device->device_fd, &i2c_address, 1);
	if (err != 1) printf("No ACK bit! %i\n", err);
	usleep(66);
	err = read(device->device_fd, &i2c_address, 1);
	printf("Part ID: %d\n", (int)i2c_address);
	usleep(66);

	err = write(device->device_fd, init_sequence, 1);
	if (err != 1) printf("No ACK bit! %i\n", err);
	//err = write(device->device_fd, init_sequence + 2, 1);
	//if (err != 1) printf("No ACK bit! %i\n", err);
	usleep(66);
	err = read(device->device_fd, return_data, 2);
	printf("Charge: %d\n", (int)transBytes2Int(return_data[1],return_data[0]));*/
	
	/*err = bq_write(device, 0x00, 0x02);
	err = bq_write(device, 0x01, 0x00);
	err |= bq_read(device, 0x00, 2, i2c_buf);
//
	if (!err)
	  version = (i2c_buf[0] + (i2c_buf[1] << 8));
	else
	  printf("Unable to get version.");
//
	printf("\nCurrent firmware version: %x \n", version);*/
#else
	bcm2835_i2c_begin();
	bcm2835_i2c_setSlaveAddress(0x55);
	bcm2835_i2c_setClockDivider(BCM2835_I2C_CLOCK_DIVIDER_626);    
	int result = bcm2835_i2c_write(&init_sequence[0],1);
	printf("Write result: %x\n", result);
	result = bcm2835_i2c_write(&init_sequence[1],1);
	printf("Write result: %x\n", result);
    //usleep(2000000L);
	result = bcm2835_i2c_read(&return_data[0],2);
	printf("Read result: %x\n", result);
	printf("data: %x %x\n", return_data[0], return_data[1]);
#endif
	return 0;
}
int stak_bq27510_close(stak_bq27510_device_s* device) {
#ifdef STAK_USE_I2C_DEVICE
	close(device->device_fd);
#else
	bcm2835_i2c_end();
#endif
	return 0;
}