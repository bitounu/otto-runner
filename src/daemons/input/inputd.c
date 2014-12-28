#include <apis/input/input.h>
#include <application/rpc/rpc.h>
#include <core/core.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <bcm2835.h>
#include <pthread.h>
#include <time.h>

// function prototype for update thread
void* update_encoder(void* arg);

// max and min helper macros
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

// gpio pin definitions
const int pin_shutter = 23;
const int pin_rotary_button = 17;
const int pin_rotary_a = 15;
const int pin_rotary_b = 14;

// global state information
volatile int terminate; // set to true to kill the update thread.

static struct input_state_s{
    int value_shutter;
    int value_rotary_left;
    int value_rotary_right;
    int shutter_state;
    int rotary_switch_State;
    int last_encoded_value;
    int encoder_value;
} input_state;

pthread_t thread_hal_update;

// we'll need an rpc message buffer
struct stak_rpc_msgbuf* message_buffer;

int init() {

    stak_rpc_init();

    // allocate message buffer to double the maximum message size for now
    message_buffer = malloc(256);

    // setup bcm2835 library and gpio pins
    if(!bcm2835_init()) {
        printf("Failed to init BCM2835 library.\n");
        return 1;
    }
    bcm2835_gpio_fsel(pin_rotary_button, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(pin_rotary_a, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(pin_rotary_b, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(pin_shutter, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(pin_shutter, BCM2835_GPIO_PUD_UP);
    bcm2835_gpio_set_pud(pin_rotary_button, BCM2835_GPIO_PUD_UP);
    bcm2835_gpio_set_pud(pin_rotary_a, BCM2835_GPIO_PUD_UP);
    bcm2835_gpio_set_pud(pin_rotary_b, BCM2835_GPIO_PUD_UP);

    // make sure the thread doesn't start terminated
    terminate = 0;

    // setup initial input state
    input_state.value_shutter = LOW;
    input_state.value_rotary_left = LOW;
    input_state.value_rotary_right = LOW;
    input_state.shutter_state = 1;
    input_state.rotary_switch_State = 1;
    input_state.last_encoded_value = 0;
    input_state.value_shutter = 0;

    // launch gpio update thread
    pthread_create(&thread_hal_update, NULL, update_encoder, 0);

    return 0;
}

int shutdown() {
    // shutdown and wait for thread to end
    terminate = 1;
    if(pthread_join(thread_hal_update, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return -1;
    }
    return 0;
}

void* update_encoder(void* arg) {
    uint64_t current_time, delta_time;
    delta_time = current_time = stak_core_get_time();

    // this encoding matrix helps us quickly calculate
    // the encoder's delta value
    const int encoding_matrix[4][4] = {
        { 0,-1, 1, 0},
        { 1, 0, 0,-1},
        {-1, 0, 0, 1},
        { 0, 1,-1, 0}
    };

    while(!terminate) {

        current_time = stak_core_get_time();

        // get current encoder value
        int encoded = (bcm2835_gpio_lev(pin_rotary_a) << 1)
                     | bcm2835_gpio_lev(pin_rotary_b);

        // get the delta value of the encoder based on current and last encoder value
        int change = encoding_matrix[input_state.last_encoded_value][encoded];

        input_state.encoder_value += change;
        input_state.last_encoded_value = encoded;

        // lock update thread to run at 60hz max
        delta_time = (stak_core_get_time() - current_time);
        uint64_t sleep_time = min(16000000L, 16000000L - max(0,delta_time));
        nanosleep((struct timespec[]){{0, sleep_time}}, NULL);
    }
    return 0;
}

int update() {
    long message_buffer[10];
    struct stak_rpc_msgbuf* msg = (struct stak_rpc_msgbuf *)&message_buffer;

    // get any messages available in rpc queue
    if( stak_rpc_message_get(msg) == 0 ) {
        switch(msg->mtype) {

            // RPC FUNCTION: stak_rpc_input_get_rotary_position
            // ARGUMENTS: none
            // RETURNS: int containing rotary encoder position
            case RPC_INPUT_GET_ROTARY_POSITION:
                {
                    long message[] = {
                        RPC_INPUT_GET_ROTARY_POSITION_RESPONSE,
                        1,
                        input_state.encoder_value
                    };
                    stak_rpc_message_send((struct stak_rpc_msgbuf *)&message, 1);
                }
                break;
            default:
                break;
        }
    }
    return 0;
}
