///#include <io/dma.h>
#include "seps114a.h"
#include <bcm2835.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <assert.h>
#include <errno.h>
//#include <linux/spi/spi.h>
#define GPIO_ON(pin) bcm2835_gpio_set(pin)
#define GPIO_OFF(pin) bcm2835_gpio_clr(pin)



#define SEPS114A_SOFT_RESET 0x01
#define SEPS114A_DISPLAY_ON_OFF 0x02
#define SEPS114A_ANALOG_CONTROL 0x0F    //
#define SEPS114A_STANDBY_ON_OFF 0x14
#define SEPS114A_OSC_ADJUST 0x1A
#define SEPS114A_ROW_SCAN_DIRECTION 0x09
#define SEPS114A_DISPLAY_X1 0x30
#define SEPS114A_DISPLAY_X2 0x31
#define SEPS114A_DISPLAY_Y1 0x32
#define SEPS114A_DISPLAY_Y2 0x33
#define SEPS114A_DISPLAYSTART_X 0x38
#define SEPS114A_DISPLAYSTART_Y 0x39
#define SEPS114A_CPU_IF 0x0D
#define SEPS114A_MEM_X1 0x34
#define SEPS114A_MEM_X2 0x35
#define SEPS114A_MEM_Y1 0x36
#define SEPS114A_MEM_Y2 0x37
#define SEPS114A_MEMORY_WRITE_READ 0x1D
#define SEPS114A_DDRAM_DATA_ACCESS_PORT 0x08
#define SEPS114A_DISCHARGE_TIME 0x18
#define SEPS114A_PEAK_PULSE_DELAY 0x16
#define SEPS114A_PEAK_PULSE_WIDTH_R 0x3A
#define SEPS114A_PEAK_PULSE_WIDTH_G 0x3B
#define SEPS114A_PEAK_PULSE_WIDTH_B 0x3C
#define SEPS114A_PRECHARGE_CURRENT_R 0x3D
#define SEPS114A_PRECHARGE_CURRENT_G 0x3E
#define SEPS114A_PRECHARGE_CURRENT_B 0x3F
#define SEPS114A_COLUMN_CURRENT_R 0x40
#define SEPS114A_COLUMN_CURRENT_G 0x41
#define SEPS114A_COLUMN_CURRENT_B 0x42
#define SEPS114A_ROW_OVERLAP 0x48
#define SEPS114A_SCAN_OFF_LEVEL 0x49
#define SEPS114A_ROW_SCAN_ON_OFF 0x17
#define SEPS114A_ROW_SCAN_MODE 0x13
#define SEPS114A_SCREEN_SAVER_CONTEROL 0xD0
#define SEPS114A_SS_SLEEP_TIMER 0xD1
#define SEPS114A_SCREEN_SAVER_MODE 0xD2
#define SEPS114A_SS_UPDATE_TIMER 0xD3
#define SEPS114A_RGB_IF 0xE0
#define SEPS114A_RGB_POL 0xE1
#define SEPS114A_DISPLAY_MODE_CONTROL 0xE5


const int STAK_SEPS114A_PIN_RST = 24;
const int STAK_SEPS114A_PIN_DC = 25;
const int STAK_SEPS114A_PIN_CS = 7;
const int STAK_SEPS114A_SPI_MODE = SPI_MODE_0;
const int STAK_SEPS114A_SPI_BPW = 8;
const int STAK_SEPS114A_SPI_SPEED = 4000000;

//#define STAK_SEPS114A_USE_SPIDEV

stak_seps114a_s* stak_seps114a_create() {
    stak_seps114a_s* device = calloc(1, sizeof(stak_seps114a_s));
    if(!bcm2835_init()) {
        printf("Failing on bcm2835_init\n");
        return 0;
    }
    bcm2835_gpio_fsel(STAK_SEPS114A_PIN_RST, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(STAK_SEPS114A_PIN_DC, BCM2835_GPIO_FSEL_OUTP);
#ifdef STAK_SEPS114A_USE_SPIDEV
    bcm2835_gpio_fsel(STAK_SEPS114A_PIN_CS, BCM2835_GPIO_FSEL_OUTP);
#endif

    GPIO_OFF(STAK_SEPS114A_PIN_RST);
    usleep(5000);
    GPIO_ON(STAK_SEPS114A_PIN_RST);
    usleep(5000);

#ifdef STAK_SEPS114A_USE_SPIDEV
    device->spi_fd = open("/dev/spidev0.0", O_RDWR);
    if(device->spi_fd < 0) {
        // error out
        printf("Failed to open spi device\n");
        return 0;
    }
    if( ioctl (device->spi_fd, SPI_IOC_WR_MODE, &STAK_SEPS114A_SPI_MODE) == -1) {
        printf("Failed to set write mode on spidev. %s\n", strerror(errno));
        return 0;
    }
    //  if( ioctl (device->spi_fd, SPI_IOC_RD_MODE, &STAK_SEPS114A_SPI_MODE) == -1) {
    //      printf("Failed to set read mode on spidev. %s\n", strerror(errno));
    //      return 0;
    //  }
    if( ioctl (device->spi_fd, SPI_IOC_WR_BITS_PER_WORD, &STAK_SEPS114A_SPI_BPW) == -1) {
        printf("Failed to set write bpw on spidev. %s\n", strerror(errno));
        return 0;
    }
    //  if( ioctl (device->spi_fd, SPI_IOC_RD_BITS_PER_WORD, &STAK_SEPS114A_SPI_BPW) == -1) {
    //      printf("Failed to set read bpw on spidev. %s\n", strerror(errno));
    //      return 0;
    //  }
    if( ioctl (device->spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &STAK_SEPS114A_SPI_SPEED) == -1) {
        printf("Failed to set write speed on spidev. %s\n", strerror(errno));
        return 0;
    }
    //  if( ioctl (device->spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &STAK_SEPS114A_SPI_SPEED) == -1) {
    //      printf("Failed to set read speed on spidev. %s\n", strerror(errno));
    //      return 0;
    //  }
    printf("SPI initialized\n");
#else
    bcm2835_spi_begin();
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_LSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_8);    // ~ 4 MHz
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default
#endif


    stak_seps114a_write_command_value(device, SEPS114A_SOFT_RESET,0x00);      
    // Standby ON/OFF
    stak_seps114a_write_command_value(device, SEPS114A_STANDBY_ON_OFF,0x01);          // Standby on
    usleep(5000);                                           // Wait for 5ms (1ms Delay Minimum)
    stak_seps114a_write_command_value(device, SEPS114A_STANDBY_ON_OFF,0x00);          // Standby off
    usleep(5000);                                           // 1ms Delay Minimum (1ms Delay Minimum)
    // Display OFF
    stak_seps114a_write_command_value(device, SEPS114A_DISPLAY_ON_OFF,0x00);
    // Set Oscillator operation
    stak_seps114a_write_command_value(device, SEPS114A_ANALOG_CONTROL,0x00);          // using external resistor and internal OSC
    // Set frame rate
    stak_seps114a_write_command_value(device, SEPS114A_OSC_ADJUST,0x03);              // frame rate : 95Hz
    // Set active display area of panel
    stak_seps114a_write_command_value(device, SEPS114A_DISPLAY_X1,0x00);
    stak_seps114a_write_command_value(device, SEPS114A_DISPLAY_X2,0x5F);
    stak_seps114a_write_command_value(device, SEPS114A_DISPLAY_Y1,0x00);
    stak_seps114a_write_command_value(device, SEPS114A_DISPLAY_Y2,0x5F);
    // Select the RGB data format and set the initial state of RGB interface port
    stak_seps114a_write_command_value(device, SEPS114A_RGB_IF,0x00);                 // RGB 8bit interface
    // Set RGB polarity
    stak_seps114a_write_command_value(device, SEPS114A_RGB_POL,0x00);
    // Set display mode control
    stak_seps114a_write_command_value(device, SEPS114A_DISPLAY_MODE_CONTROL,0x80);   // SWAP:BGR, Reduce current : Normal, DC[1:0] : Normal
    // Set MCU Interface
    stak_seps114a_write_command_value(device, SEPS114A_CPU_IF,0x00);                 // MPU External interface mode, 8bits
    // Set Memory Read/Write mode
    stak_seps114a_write_command_value(device, SEPS114A_MEMORY_WRITE_READ,0x00);
    // Set row scan direction
    stak_seps114a_write_command_value(device, SEPS114A_ROW_SCAN_DIRECTION,0x00);     // Column : 0 --> Max, Row : 0 --> Max
    // Set row scan mode
    stak_seps114a_write_command_value(device, SEPS114A_ROW_SCAN_MODE,0x00);          // Alternate scan mode
    // Set column current
    stak_seps114a_write_command_value(device, SEPS114A_COLUMN_CURRENT_R,0x6E);
    stak_seps114a_write_command_value(device, SEPS114A_COLUMN_CURRENT_G,0x4F);
    stak_seps114a_write_command_value(device, SEPS114A_COLUMN_CURRENT_B,0x77);
    // Set row overlap
    stak_seps114a_write_command_value(device, SEPS114A_ROW_OVERLAP,0x00);            // Band gap only
    // Set discharge time
    stak_seps114a_write_command_value(device, SEPS114A_DISCHARGE_TIME,0x01);         // Discharge time : normal discharge
    // Set peak pulse delay
    stak_seps114a_write_command_value(device, SEPS114A_PEAK_PULSE_DELAY,0x00);
    // Set peak pulse width
    stak_seps114a_write_command_value(device, SEPS114A_PEAK_PULSE_WIDTH_R,0x02);
    stak_seps114a_write_command_value(device, SEPS114A_PEAK_PULSE_WIDTH_G,0x02);
    stak_seps114a_write_command_value(device, SEPS114A_PEAK_PULSE_WIDTH_B,0x02);
    // Set precharge current
    stak_seps114a_write_command_value(device, SEPS114A_PRECHARGE_CURRENT_R,0x14);
    stak_seps114a_write_command_value(device, SEPS114A_PRECHARGE_CURRENT_G,0x50);
    stak_seps114a_write_command_value(device, SEPS114A_PRECHARGE_CURRENT_B,0x19);
    // Set row scan on/off 
    stak_seps114a_write_command_value(device, SEPS114A_ROW_SCAN_ON_OFF,0x00);        // Normal row scan
    // Set scan off level
    stak_seps114a_write_command_value(device, SEPS114A_SCAN_OFF_LEVEL,0x04);         // VCC_C*0.75
    // Set memory access point
    stak_seps114a_write_command_value(device, SEPS114A_DISPLAYSTART_X,0x00);
    stak_seps114a_write_command_value(device, SEPS114A_DISPLAYSTART_Y,0x01);
    // Display ON
    stak_seps114a_write_command_value(device, SEPS114A_DISPLAY_ON_OFF,0x01);
    stak_seps114a_write_command_value(device, SEPS114A_MEMORY_WRITE_READ,0x00);
    printf("Display enabled\n");

    device->framebuffer = NULL;
    printf("Initializing framebuffer at address: 0x%8x\n", (uint32_t) device->framebuffer);
    device->framebuffer = calloc(96*96, sizeof(uint16_t));
    printf("Initializing framebuffer at address: 0x%8x\n", (uint32_t) device->framebuffer);
    //memset(device->framebuffer, 0x33,96*96*2);
    return device;
}
inline uint16_t swap_rgb (uint16_t rgb)
{
    return  (((rgb << 8) & 0xff00) |
             ((rgb >> 8) & 0xff));
}
inline void stak_seps114a_spidev_write(stak_seps114a_s* device, char* data, int length) {
    int split = 0;
    int offset = 0;
    if(length > 2048) {
        int remainder = (length % 2048);
        int chunks = (length - remainder) / 2048 + 1;
        struct spi_ioc_transfer* message = calloc(chunks, sizeof(struct spi_ioc_transfer) );
        for(;split < chunks; split++) {
            offset = (split * 2048);
            int size = (length - (split * 2048)) % 2048;
            if(size == 0) size = 2048;
            message[split].tx_buf = ((uint64_t) data + offset);
            message[split].rx_buf = ((uint64_t) data + offset);
            message[split].len = size;
            message[split].delay_usecs = 10;
            message[split].speed_hz = STAK_SEPS114A_SPI_SPEED;
            message[split].bits_per_word = STAK_SEPS114A_SPI_BPW;
            message[split].cs_change = 0;
            if( (ioctl(device->spi_fd, SPI_IOC_MESSAGE(1), &message[split])) == -1 ) {
                printf("Failed to write message to spidev. %s\n", strerror(errno));
                printf("\tAddress: 0x%8x\n", data + offset);
            }
        }
    }
    else {
        struct spi_ioc_transfer message;
        message.tx_buf = ((uint64_t) data );
        message.rx_buf = ((uint64_t) data );
        message.len = length;
        message.delay_usecs = 10;
        message.speed_hz = STAK_SEPS114A_SPI_SPEED;
        message.bits_per_word = STAK_SEPS114A_SPI_BPW;
        message.cs_change = 0;
        if( (ioctl(device->spi_fd, SPI_IOC_MESSAGE(1), &message)) == -1 ) {
            printf("Failed to write message to spidev. %s\n", strerror(errno));
            printf("\tAddress: 0x%8x\n", data + offset);
        }
    }
}
int stak_seps114a_destroy(stak_seps114a_s* device) {

    memset(device->framebuffer, 0x00,96*96*2);
    stak_seps114a_update(device);
    if(device->framebuffer != NULL)
        free(device->framebuffer);

#ifdef STAK_SEPS114A_USE_SPIDEV
    close(device->spi_fd);
#else
    bcm2835_spi_end();
    bcm2835_close();
#endif
    return 0;
}
int stak_seps114a_update(stak_seps114a_s* device) {
    uint16_t *vmem16 = (uint16_t *)device->framebuffer;
    int x, y;

    stak_seps114a_write_command(device, SEPS114A_DDRAM_DATA_ACCESS_PORT);

    for (x = 0; x < 96; x++) {
        for (y = 0; y < 96; y++) {
            vmem16[y*96+x] = swap_rgb(vmem16[y*96+x]);
        }
    }
    GPIO_ON(STAK_SEPS114A_PIN_DC);

    stak_seps114a_write_data(device, (char*)vmem16, 96*96*2);
    return 0;
}
int stak_seps114a_write_byte(stak_seps114a_s* device, char data_value) {
    GPIO_ON(STAK_SEPS114A_PIN_DC);

#ifdef STAK_SEPS114A_USE_SPIDEV
    GPIO_OFF(STAK_SEPS114A_PIN_CS);
    stak_seps114a_spidev_write(device, &data_value, 1);
    GPIO_ON(STAK_SEPS114A_PIN_CS);
#else
    bcm2835_spi_transfer(data_value);
#endif
    return 0;
}
int stak_seps114a_write_data(stak_seps114a_s* device, char* data_value, uint32_t size) {
    GPIO_ON(STAK_SEPS114A_PIN_DC);

#ifdef STAK_SEPS114A_USE_SPIDEV
    GPIO_OFF(STAK_SEPS114A_PIN_CS);
    stak_seps114a_spidev_write(device, data_value, size);
    GPIO_ON(STAK_SEPS114A_PIN_CS);
#else
    bcm2835_spi_transfernb(data_value, data_value, size);
#endif
    return 0;
}
int stak_seps114a_write_command(stak_seps114a_s* device, char reg_index) {
    //Select index addr
    GPIO_OFF(STAK_SEPS114A_PIN_DC);
#ifdef STAK_SEPS114A_USE_SPIDEV
    GPIO_OFF(STAK_SEPS114A_PIN_CS);
    stak_seps114a_spidev_write(device, &reg_index, 1);
    GPIO_ON(STAK_SEPS114A_PIN_CS);
#else
    bcm2835_spi_transfer(reg_index);
#endif
    return 0;
}
int stak_seps114a_write_command_value(stak_seps114a_s* device, char reg_index, char reg_value) {
    //Select index addr
#ifdef STAK_SEPS114A_USE_SPIDEV
    GPIO_OFF(STAK_SEPS114A_PIN_DC);
    GPIO_OFF(STAK_SEPS114A_PIN_CS);
    stak_seps114a_spidev_write(device, &reg_index, 1);
    GPIO_ON(STAK_SEPS114A_PIN_CS);
    GPIO_ON(STAK_SEPS114A_PIN_DC);
    GPIO_OFF(STAK_SEPS114A_PIN_CS);
    stak_seps114a_spidev_write(device, &reg_value, 1);
    GPIO_ON(STAK_SEPS114A_PIN_CS);
#else
    GPIO_OFF(STAK_SEPS114A_PIN_DC);
    bcm2835_spi_transfer(reg_index);
    GPIO_ON(STAK_SEPS114A_PIN_DC);
    bcm2835_spi_transfer(reg_value);
#endif
    return 0;
}