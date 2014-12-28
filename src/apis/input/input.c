#include <apis/input/input.h>
#include <application/rpc/rpc.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

int stak_rpc_input_get_rotary_position() {
	// create a simple message to tell the daemon which function
	// we want to call.
	{
		long message[] = {
			RPC_INPUT_GET_ROTARY_POSITION,
			0
		};

		// send message to daemon
		if(stak_rpc_message_send((void*)&message, 0))
	        return -1;
	}
	
	{
		long message[3];
	    // wait for a response or return -1 if an error occurred or
	    // call was interrupted by system
	    if(stak_rpc_message_wait((void*)&message, RPC_INPUT_GET_ROTARY_POSITION_RESPONSE))
	        return -1;

	    return stak_rpc_buffer_get_int((struct stak_rpc_msgbuf*)&message, 0);
	}
}