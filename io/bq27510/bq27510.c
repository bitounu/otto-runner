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
#include <pthread.h>

#define STAK_USE_I2C_DEVICE
volatile int stak_bq27510_update_thread_terminate = 0;
pthread_t stak_bq27510_update_thread_poll;

volatile struct {

}stak_bq27510_battery_state;

void* stak_bq27510_update_thread(void* arg);


unsigned int transBytes2Int(unsigned char msb, unsigned char lsb)
{
	unsigned int tmp;

    tmp = ((msb << 8) & 0xFF00);
    return ((unsigned int)(tmp + lsb) & 0xFFFF);
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
	usleep(10);

	err = read(device->device_fd, i2c_buf, bytes);
	if (err != bytes) {
	    fprintf(stderr, "\nError: 0x%x - i2c transaction failure! bb \n", err);
		return -1;
	}

	return 0;
}

int stak_bq27510_open(char* filename, stak_bq27510_device_s* device) {
#ifdef STAK_USE_I2C_DEVICE
	device->device_fd = open("/dev/i2c-1", O_RDWR);

	if (device->device_fd < 0) {
	        printf("\nError: Failed to open /dev/i2c-1!\n");
	        return -1;
	}

	unsigned char i2c_address = 0x55;
	ioctl(device->device_fd, I2C_TENBIT, 0);
	if (ioctl(device->device_fd, I2C_SLAVE, i2c_address) < 0) {
		printf("Unable to get bus access to talk to slave\n");
		return -1;
	}
	char buf[10];

	bq_read(device, 0x08, 2, buf);
	printf("\tVoltage: %f\n", ((float)transBytes2Int(buf[1],buf[0]))/1000.0f);

	bq_read(device, 0x20, 2, buf);
	printf("\tCapacity: %i%%\n", ((int)transBytes2Int(buf[1], buf[0])));

	//pthread_create(&stak_bq27510_update_thread_poll, NULL, stak_bq27510_update_thread, NULL);
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
void* stak_bq27510_update_thread(void* arg) {
	/*while(!stak_bq27510_update_thread_terminate) {
		bq_read(device, 0x08, 2, buf);
		voltage = ((float)transBytes2Int(buf[1],buf[0]))/1000.0f;
	}*/
	return 0;
}
int stak_bq27510_close(stak_bq27510_device_s* device) {
#ifdef STAK_USE_I2C_DEVICE
	/*stak_bq27510_update_thread_terminate = true;
	if(pthread_join(stak_bq27510_update_thread_poll, NULL)) {
		fprintf(stderr, "Error joining thread\n");
		return;
	}*/
	close(device->device_fd);
#else
	bcm2835_i2c_end();
#endif
	return 0;
}