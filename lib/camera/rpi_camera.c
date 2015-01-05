#include <lib/camera/rpi_camera.h>
#include <stdlib.h>
#include <stdio.h>
#include <stak.h>

#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2

static void rpi_camera_control_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer) {

}

static void rpi_camera_video_buffer_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer) {
	struct rpi_camera_output* output = ( struct rpi_camera_output* ) port->userdata;
	if ( !output )
		return stak_error_throw( "Output not initialized" );

	stak_log("camera: 0x%08x", (int) output);
	//to handle the user not reading frames, remove and return any pre-existing ones
	if ( mmal_queue_length( output->output_queue )>=2 )
	{
		MMAL_BUFFER_HEADER_T* existing_buffer = mmal_queue_get( output->output_queue );
		if ( existing_buffer )
		{
		stak_log("%s","");
			mmal_buffer_header_release( existing_buffer );
			if ( port->is_enabled )
			{
				MMAL_BUFFER_HEADER_T *new_buffer;
				MMAL_STATUS_T status;
				new_buffer = mmal_queue_get( output->buffer_pool->queue );
				if ( new_buffer )
					status = mmal_port_send_buffer(port, new_buffer);
				if (!new_buffer || status != MMAL_SUCCESS)
					return stak_error_throw( "Unable to return a buffer to the video port" );
			}	
		}
	}
	stak_log("Nothing put onto queue...%s", "");
	stak_log("%s","");
	//add the buffer to the output queue
	mmal_queue_put( output->output_queue, buffer );
}

int rpi_camera_output_create( struct rpi_camera_output* output, int width, int height, MMAL_COMPONENT_T* input_component, int input_port_idx, int do_argb_conversion ) {
	if ( !output ) {
		output = calloc( 1, sizeof( struct rpi_camera_output ) );
		if ( !output )
			return stak_error_throw( "Could not allocate memory for output structure" );
	}

	//printf("Init camera output with %d/%d\n",width,height);
	output->width = width;
	output->height = height;

	output->resizer_component = null_ptr();
	output->connection        = null_ptr();
	output->buffer_pool       = null_ptr();
	output->output_queue      = null_ptr();
	output->buffer_port       = null_ptr();

	//got the port we're receiving from
	MMAL_PORT_T* input_port = input_component->output[ input_port_idx ];

	//check if user wants conversion to argb or the width or height is different to the input
	if( do_argb_conversion ||
		width != input_port->format->es->video.width ||
		height != input_port->format->es->video.height ) {

		//create the resizing component, reading from the splitter output
		if ( rpi_camera_output_create_resize_component( output, input_port, do_argb_conversion ) )
			return stak_error_throw( "Could not create resize component" );

		stak_log("input_port: 0x%08x", (int) input_port);

		//create and enable a connection between the video output and the resizer input
		;
		if (mmal_connection_create( &output->connection, input_port, output->resizer_component->input[0], MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT ) != MMAL_SUCCESS)
			return stak_error_throw( "Failed to create connection" );

		stak_log("output->connection: 0x%08x", (int) output->connection);

		if( mmal_connection_enable( output->connection ) != MMAL_SUCCESS)
			return stak_error_throw( "Failed to create connection" );

		//set the buffer pool port to be the resizer output
		output->buffer_port = output->resizer_component->output[0];
		stak_log("output->buffer_port: 0x%08x", (int) output->buffer_port);
	}
	else
	{
		//no convert/resize needed so just set the buffer pool port to be the input port
		output->buffer_port = input_port;
	stak_log("buffer_port: 0x%08x", (int) output->buffer_port);
	}

	//setup the video buffer callback
	rpi_camera_output_enable_port_callback_and_create_buffer_pool( output, output->buffer_port, rpi_camera_video_buffer_callback, 1);
	// output->buffer_pool = EnablePortCallbackAndCreateBufferPool( output->buffer_pool, VideoBufferCallback, 3);
	stak_log("buffer_pool: 0x%08x", (int) output->buffer_pool);
	if( !output->buffer_pool )
		return stak_error_throw( "Failed to create buffer pool" );

	//create the output queue
	output->output_queue = mmal_queue_create();
	stak_log("output_queue: 0x%08x", (int) output->output_queue);
	if(!output->output_queue)
		return stak_error_throw( "Failed to create output queue" );

	return stak_error_none( );
}
int rpi_camera_output_destroy( struct rpi_camera_output* output ) {
	if ( !output )
		return stak_error_throw( "Camera output not initialized" );

	return stak_error_none( );
}
int rpi_camera_output_read_frame( struct rpi_camera_output* output, void** buffer, int buffer_size, int *out_buffer_size ) {
	if ( !output )
		return stak_error_throw( "Camera output not initialized" );

	//get buffer
	void* new_buf = 0;
	int new_buffer_size = 0;
	if( !rpi_camera_output_read_frame_begin( output, &new_buf, &new_buffer_size ) )
	{
		if( buffer_size >= new_buffer_size )
		{
			//got space - copy it in and return size
			memcpy( buffer, new_buf, new_buffer_size );
			*out_buffer_size = new_buffer_size;
			stak_log("copied buffer of %i bytes", new_buffer_size);
		}
		else
		{
			*out_buffer_size = 0;
			stak_log("Not enough space in buffer for new frame %i", new_buffer_size);
			return stak_error_throw( "Not enough space in buffer for new frame" ); //res = -1;
		}
		rpi_camera_output_read_frame_end( output );
	}
	// stak_log("Camera frame not read %i", new_buffer_size);
	return stak_error_throw( "Camera frame not read" );
}

int rpi_camera_output_read_frame_begin( struct rpi_camera_output* output, void** out_buffer, int* out_buffer_size ) {
	if ( !output )
		return stak_error_throw( "Camera output not initialized" );

	MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get( output->output_queue );
	//stak_log("output->output_queue: 0x%08x", (int) output->output_queue);
	//stak_log("buffer: 0x%08x", (int) buffer);
	if( buffer )
	{

		//lock it
		mmal_buffer_header_mem_lock( buffer );

		//store it
		output->locked_buffer = buffer;

		//fill out the output variables and return success
		*out_buffer = buffer->data;
		*out_buffer_size = buffer->length;
		return stak_error_none( );
	}
	//no buffer - return false
	return stak_error_throw( "MMAL did not return a buffer from the output queue" );
}

int rpi_camera_output_read_frame_end( struct rpi_camera_output* output ) {
	if ( !output )
		return stak_error_throw( "Camera output not initialized" );

	if( output->locked_buffer )
	{
		// unlock and then release buffer back to the pool from whence it came
		mmal_buffer_header_mem_unlock( output->locked_buffer );
		mmal_buffer_header_release( output->locked_buffer );
		output->locked_buffer = null_ptr();

		// and send it back to the port (if still open)
		if ( output->buffer_port->is_enabled )
		{
			MMAL_BUFFER_HEADER_T *new_buffer = mmal_queue_get( output->buffer_pool->queue );
			if ( new_buffer || mmal_port_send_buffer( output->buffer_port, new_buffer ) != MMAL_SUCCESS )
				return stak_error_throw( "Unable to return a buffer to the video port" );
		}
		return stak_error_throw( "Output port is not enabled" );
	}
	return stak_error_none( );
}
int rpi_camera_output_create_resize_component( struct rpi_camera_output* output, MMAL_PORT_T* video_output_port, int do_argb_conversion ) {
	if ( !output )
		return stak_error_throw( "Camera output not initialized" );

	MMAL_COMPONENT_T *resizer = null_ptr();
	MMAL_PORT_T *input_port = null_ptr(),
				*output_port = null_ptr();
	MMAL_STATUS_T status;

	//create the camera component
	status = mmal_component_create( "vc.ril.resize", &resizer );

	if ( status != MMAL_SUCCESS ) {
		mmal_component_destroy(resizer);
		return stak_error_throw( "Failed to create resize component" );
	}

	//check we have output ports
	if (resizer->output_num != 1 || resizer->input_num != 1) {
		mmal_component_destroy(resizer);
		return stak_error_throw( "Resizer doesn't have correct ports" );
	}

	//get the ports
	input_port = resizer->input[0];
	output_port = resizer->output[0];

	mmal_format_copy( input_port->format, video_output_port->format );
	input_port->buffer_num = 3;
	status = mmal_port_format_commit( input_port );

	if ( status != MMAL_SUCCESS ) {
		mmal_component_destroy(resizer);
		return stak_error_throw( "Couldn't set resizer input port format" );
	}

	mmal_format_copy( output_port->format,input_port->format );

	if( do_argb_conversion )
	{
		output_port->format->encoding = MMAL_ENCODING_RGBA;
		output_port->format->encoding_variant = MMAL_ENCODING_RGBA;
	}
	output_port->format->es->video.width = output->width;
	output_port->format->es->video.height = output->height;
	output_port->format->es->video.crop.x = 0;
	output_port->format->es->video.crop.y = 0;
	output_port->format->es->video.crop.width = output->width;
	output_port->format->es->video.crop.height = output->height;

	stak_log( "output_port: 0x%08x", (int) output_port );
	stak_log( "output width: %i", (int) output->width );
	stak_log( "output width: %i", (int) output->height );

	if ( mmal_port_format_commit( output_port ) != MMAL_SUCCESS )
		return stak_error_throw( "Couldn't set resizer output port format" );

	output->resizer_component = resizer;
	// return resizer;

/*error:
	if(resizer)
		mmal_component_destroy(resizer);
	return NULL;
*/
	return stak_error_none( );
}
int rpi_camera_output_enable_port_callback_and_create_buffer_pool( struct rpi_camera_output* output, MMAL_PORT_T* port, MMAL_PORT_BH_CB_T cb, int buffer_count ) {
	if ( !output )
		return stak_error_throw( "Camera output not initialized" );

	MMAL_STATUS_T status;
	MMAL_POOL_T* buffer_pool = 0;

	//setup video port buffer and a pool to hold them
	port->buffer_num = buffer_count;
	port->buffer_size = port->buffer_size_recommended;
	stak_log("Creating pool with %d buffers of size %d", port->buffer_num, port->buffer_size);
	buffer_pool = mmal_port_pool_create(port, port->buffer_num, port->buffer_size);
	if ( !buffer_pool ) {
		if( buffer_pool ) mmal_port_pool_destroy( port, buffer_pool );
		return stak_error_throw( "Couldn't create video buffer pool" );
	}

	//enable the port and hand it the callback
    port->userdata = (struct MMAL_PORT_USERDATA_T *)output;
	status = mmal_port_enable( port, cb );
	if (status != MMAL_SUCCESS) {
		if( buffer_pool ) mmal_port_pool_destroy( port, buffer_pool );
		return stak_error_throw( "Failed to set video buffer callback" );
	}

	//send all the buffers in our pool to the video port ready for use
	{
		int num = mmal_queue_length( buffer_pool->queue );
		int q;
		for (q=0;q<num;q++)
		{
			MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get( buffer_pool->queue );

			if ( !buffer ) {
				if( buffer_pool ) mmal_port_pool_destroy( port, buffer_pool );
				return stak_error_throw( "Unable to get a required buffer from pool queue" );
			}
			else if ( mmal_port_send_buffer(port, buffer)!= MMAL_SUCCESS ) {
				if( buffer_pool ) mmal_port_pool_destroy( port, buffer_pool );
				return stak_error_throw( "Unable to send a buffer to port" );
			}
		}
	}

	output->buffer_pool = buffer_pool;
	return stak_error_none( );
}

//----------
// function: rpi_camera_create_component
// parameters:
//   object:
//   outputs:
//   inputs:
//----------
int rpi_camera_create_camera_component_and_setup_ports( struct rpi_camera* camera, struct rpi_camera_settings *settings ) {
	camera->component         = null_ptr();
	MMAL_ES_FORMAT_T *format  = null_ptr();
	MMAL_PORT_T *preview_port = null_ptr(),
				*video_port   = null_ptr(),
				*still_port   = null_ptr();

	if ( mmal_component_create( MMAL_COMPONENT_DEFAULT_CAMERA, &camera->component ) != MMAL_SUCCESS )
		return stak_error_throw( "Failed to create camera component" );

	MMAL_PARAMETER_INT32_T camera_num = {{MMAL_PARAMETER_CAMERA_NUM, sizeof(camera_num)}, 0};
	if ( mmal_port_parameter_set( camera->component->control, &camera_num.hdr ) != MMAL_SUCCESS) {
		stak_log( "Error setting camera number %i", 0 );
	}
	if ( !camera->component->output_num ) {
		mmal_component_destroy( camera->component );
		return stak_error_throw( "Camera doesn't have output ports" );
	}

	// get the three ports
	preview_port = camera->component->output[ MMAL_CAMERA_PREVIEW_PORT ];
	video_port   = camera->component->output[ MMAL_CAMERA_VIDEO_PORT   ];
	still_port   = camera->component->output[ MMAL_CAMERA_CAPTURE_PORT ];

	if ( mmal_port_enable( camera->component->control, rpi_camera_control_callback ) != MMAL_SUCCESS ) {
		mmal_component_destroy( camera->component );
		return stak_error_throw( "Unable to enable control port" );
	}

	// setup camera configuration
	{
		MMAL_PARAMETER_CAMERA_CONFIG_T cam_config = {
			.hdr = {
				.id = MMAL_PARAMETER_CAMERA_CONFIG,
				.size = sizeof(cam_config)
			},
			.max_stills_w = settings->width,
			.max_stills_h = settings->height,
			.stills_yuv422 = 0,
			.one_shot_stills = 0,
			.max_preview_video_w = settings->width,
			.max_preview_video_h = settings->height,
			.num_preview_video_frames = 3,
			.stills_capture_circular_buffer_height = 0,
			.fast_preview_resume = 0,
			.use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC
		};
		mmal_port_parameter_set( camera->component->control, &cam_config.hdr );
	}

	// setup preview port
	format = preview_port->format;
	format->encoding = MMAL_ENCODING_OPAQUE;
	format->encoding_variant = MMAL_ENCODING_I420;
	format->es->video.width = settings->width;
	format->es->video.height = settings->height;
	format->es->video.crop.x = 0;
	format->es->video.crop.y = 0;
	format->es->video.crop.width = settings->width;
	format->es->video.crop.height = settings->height;
	format->es->video.frame_rate.num = settings->frame_rate;
	format->es->video.frame_rate.den = 1;

	if ( mmal_port_format_commit( preview_port ) != MMAL_SUCCESS ) {
		mmal_component_destroy( camera->component );
		return stak_error_throw( "Couldn't set preview port format" );
	}

	// setup video port
	format = video_port->format;
	format->encoding = MMAL_ENCODING_I420;
	format->encoding_variant = MMAL_ENCODING_I420;
	format->es->video.width = settings->width;
	format->es->video.height = settings->height;
	format->es->video.crop.x = 0;
	format->es->video.crop.y = 0;
	format->es->video.crop.width = settings->width;
	format->es->video.crop.height = settings->height;
	format->es->video.frame_rate.num = settings->frame_rate;
	format->es->video.frame_rate.den = 1;

	if ( mmal_port_format_commit( video_port ) != MMAL_SUCCESS ) {
		mmal_component_destroy( camera->component );
		return stak_error_throw( "Couldn't set video port format" );
	}

	// setup preview port
	format = still_port->format;
	format->encoding = MMAL_ENCODING_OPAQUE;
	format->encoding_variant = MMAL_ENCODING_I420;
	format->es->video.width = settings->width;
	format->es->video.height = settings->height;
	format->es->video.crop.x = 0;
	format->es->video.crop.y = 0;
	format->es->video.crop.width = settings->width;
	format->es->video.crop.height = settings->height;
	format->es->video.frame_rate.num = 1;
	format->es->video.frame_rate.den = 1;

	if ( mmal_port_format_commit( still_port ) != MMAL_SUCCESS ) {
		mmal_component_destroy( camera->component );
		return stak_error_throw( "Couldn't set still port format" );
	}

	camera->settings.width = settings->width;
	camera->settings.height = settings->height;
	camera->settings.frame_rate = settings->frame_rate;

	camera->camera_parameters.sharpness = 0;
	camera->camera_parameters.contrast = 0;
	camera->camera_parameters.brightness = 50;
	camera->camera_parameters.saturation = 0;
	camera->camera_parameters.iso = 0;                    // 0 = auto
	camera->camera_parameters.video_stabilisation = 0;
	camera->camera_parameters.exposure_compensation = 0;
	camera->camera_parameters.exposure_mode = MMAL_PARAM_EXPOSUREMODE_AUTO;
	camera->camera_parameters.exposure_meter_mode = MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE;
	camera->camera_parameters.awb_mode = MMAL_PARAM_AWBMODE_AUTO;
	camera->camera_parameters.image_effect = MMAL_PARAM_IMAGEFX_NONE;
	camera->camera_parameters.colour_effects.enable = 0;
	camera->camera_parameters.colour_effects.u = 128;
	camera->camera_parameters.colour_effects.v = 128;
	camera->camera_parameters.rotation = 0;
	camera->camera_parameters.hflip = 0;
	camera->camera_parameters.vflip = 0;
	camera->camera_parameters.roi.x = 0.0;
	camera->camera_parameters.roi.y = 0.0;
	camera->camera_parameters.roi.w = 1.0;
	camera->camera_parameters.roi.h = 1.0;
	camera->camera_parameters.shutter_speed = 0;          // 0 = auto

	//apply all camera parameters
	int error = 0;
	#if 1
	error += rpi_camera_control_set_saturation ( camera, camera->camera_parameters.sharpness );
	error += rpi_camera_control_set_sharpness ( camera, camera->camera_parameters.contrast );
	error += rpi_camera_control_set_brightness ( camera, camera->camera_parameters.brightness );
	error += rpi_camera_control_set_iso ( camera, camera->camera_parameters.iso );
	error += rpi_camera_control_set_video_stabilisation ( camera, camera->camera_parameters.video_stabilisation );
	error += rpi_camera_control_set_exposure_compensation ( camera, camera->camera_parameters.exposure_compensation );
	error += rpi_camera_control_set_metering_mode ( camera, camera->camera_parameters.exposure_meter_mode );
	error += rpi_camera_control_set_awb_mode ( camera, camera->camera_parameters.awb_mode );
	error += rpi_camera_control_set_image_fx ( camera, camera->camera_parameters.image_effect );
	error += rpi_camera_control_set_colour_fx ( camera, &camera->camera_parameters.colour_effects );
	error += rpi_camera_control_set_rotation ( camera, camera->camera_parameters.rotation );
	error += rpi_camera_control_set_flips ( camera, camera->camera_parameters.hflip, camera->camera_parameters.vflip );
	error += rpi_camera_control_set_roi ( camera, camera->camera_parameters.roi );
	error += rpi_camera_control_set_shutter_speed ( camera, camera->camera_parameters.shutter_speed );
	#endif

	if( error )
		return stak_error_throw( "Error setting default parameters for camera" );


	if ( mmal_component_enable( camera->component ) != MMAL_SUCCESS ) {
		mmal_component_destroy( camera->component );
		return stak_error_throw( "Couldn't enable camera" );
	}

	// everything succeeded!
	return stak_error_none( );
}

//----------
// function: rpi_camera_create
// parameters:
//   object:
//     camera - pointer to camera instance. If null pointer, allocates structure
//   outputs:
//   inputs:
//     settings - structure containing camera settings
//----------
int rpi_camera_create_splitter_component_and_setup_ports( struct rpi_camera* camera, MMAL_PORT_T* video_output_port, MMAL_COMPONENT_T* splitter_component ) {
	// set output to null for now
	//video_output_port          = null_ptr();
	//MMAL_COMPONENT_T *splitter = null_ptr();
	//MMAL_ES_FORMAT_T *format;
	MMAL_PORT_T *input_port    = null_ptr(),
				*output_port   = null_ptr();

	//create the camera component
	if ( mmal_component_create( MMAL_COMPONENT_DEFAULT_VIDEO_SPLITTER, &splitter_component ) != MMAL_SUCCESS ) {
		if( splitter_component )
			mmal_component_destroy( splitter_component );
		return stak_error_throw( "Failed to create splitter component" );
	}

	//check we have output ports
	if ( splitter_component->output_num != 4 || splitter_component->input_num != 1 ) {

		if( splitter_component )
			mmal_component_destroy( splitter_component );
		return stak_error_throw( "Splitter doesn't have correct ports" );
	}

	//get the ports
	input_port = splitter_component->input[0];
	mmal_format_copy( input_port->format, video_output_port->format );

	input_port->buffer_num = 3;
	if ( mmal_port_format_commit( input_port ) != MMAL_SUCCESS ) {
		if( splitter_component )
			mmal_component_destroy( splitter_component );
		return stak_error_throw( "Couldn't set resizer input port format" );
	}

	int i;
	for(i = 0; i < splitter_component->output_num; i++)
	{
		output_port = splitter_component->output[i];
		output_port->buffer_num = 3;
		mmal_format_copy(output_port->format,input_port->format);
		if ( mmal_port_format_commit( output_port ) != MMAL_SUCCESS ) {
		if ( camera->splitter_component )
			mmal_component_destroy( splitter_component );
		return stak_error_throw( "Couldn't set resizer output port format" );
		}
	}
	camera->splitter_component = splitter_component;

	// everything succeeded!
	return stak_error_none( );
}

//----------
// function: rpi_camera_create
// parameters:
//   object:
//     camera - pointer to camera instance. If null pointer, allocates structure
//   outputs:
//   inputs:
//     settings - structure containing camera settings
//----------
int rpi_camera_create( struct rpi_camera* camera, struct rpi_camera_settings settings ) {

	bcm_host_init();

	if ( !camera ) {
		camera = calloc( 1, sizeof( struct rpi_camera ) );
		stak_log( "camera created: 0x%08x", (int) camera );
		if ( !camera )
			return stak_error_throw( "Could not allocate memory for camera structure" );
	}else memset( camera, 0, sizeof(struct rpi_camera) );
	// now that we are fairly sure camera is initialized, let's do something with it.
	camera->component = null_ptr();
	camera->splitter_component = null_ptr();
	camera->vid_to_split_connection = null_ptr();
	MMAL_PORT_T *video_port = null_ptr();

	if( rpi_camera_create_camera_component_and_setup_ports( camera, &settings ) )
		return stak_error_throw( "Could not setup camera" );

	if( !camera->component ) {
		mmal_component_destroy( camera->component );
		return stak_error_throw( "Camera not properly created" );
	}

	//get the video port
	video_port = camera->component->output[ MMAL_CAMERA_VIDEO_PORT ];
	video_port->buffer_num = 3;

	if( rpi_camera_create_splitter_component_and_setup_ports( camera, video_port, camera->splitter_component) )
		return stak_error_throw( "Could not setup camera" );

	if( !camera->splitter_component ) {
		mmal_component_destroy( camera->component );
		return stak_error_throw( "Camera not properly created" );
	}

	// create and enable a connection between the video output and the resizer input
	if ( mmal_connection_create( &camera->vid_to_split_connection,
								 video_port,
								 camera->splitter_component->input[0],
								 MMAL_CONNECTION_FLAG_TUNNELLING |
								 MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT) != MMAL_SUCCESS) {
		return stak_error_throw( "Failed to create connection" );
	}
	if ( mmal_connection_enable(camera->vid_to_split_connection) != MMAL_SUCCESS ) {
		return stak_error_throw( "Failed to enable connection" );
	}

	//setup all the outputs
	int i;
	for(i = 0; i < settings.num_levels; i++)
	{
		stak_log( "creating output with width %i and height %i", (int) settings.width, settings.height );
		if ( rpi_camera_output_create( &camera->outputs[i],
			settings.width,
			settings.height,
			camera->splitter_component,
			i,
			settings.do_argb_conversion) )
		{
			return stak_error_throw( "Failed to initialize output" );
		}
		stak_log( "output created: 0x%08x", (int) &camera->outputs[i] );
	}


	stak_log( "camera: 0x%08x", (int) camera );
	//begin capture
	if ( mmal_port_parameter_set_boolean( video_port, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS )
		return stak_error_throw( "Failed to start capture" );

	stak_log( "camera: 0x%08x", (int) camera );

	return stak_error_none( );
}

//----------
// function: rpi_camera_destroy
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_destroy( struct rpi_camera* camera ) {

	stak_log( "camera: 0x%08x", (int) camera );
	if ( !camera )
		return stak_error_throw( "No camera specified" );

	int i;
	for( i = 0; i < 4; i++)
	{
		if( camera->outputs[i].resizer_component )
		{
			rpi_camera_output_destroy( &camera->outputs[i] );
			camera->outputs[i].resizer_component = null_ptr();
		}
	}
	if( camera->vid_to_split_connection )
		mmal_connection_destroy( camera->vid_to_split_connection );

	if( camera->component )
		mmal_component_destroy( camera->component );

	if( camera->splitter_component )
		mmal_component_destroy( camera->splitter_component );

	camera->vid_to_split_connection = null_ptr();
	camera->component               = null_ptr();
	camera->splitter_component      = null_ptr();

	//
	return stak_error_none( );
}

//----------
// function:
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_read_frame( struct rpi_camera* camera, int level, void** buffer, int buffer_size ) {
	if ( !camera )
		return stak_error_throw( "No camera specified" );
	
	int new_size = 0;
	if ( rpi_camera_output_read_frame( &camera->outputs[level], buffer, buffer_size, &new_size ) )
		return stak_error_throw( "Could not read frame" );

	return stak_error_none( );
}

//----------
// function:
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_read_frame_begin( struct rpi_camera* camera, int level, void** out_buffer, int* out_buffer_size ) {
	if ( !camera )
		return stak_error_throw( "No camera specified" );

	if ( rpi_camera_output_read_frame_begin( &camera->outputs[level], out_buffer,out_buffer_size ) )
		return stak_error_throw( "Could not read frame" );

	//
	return stak_error_none( );
}

//----------
// function:
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_read_frame_end( struct rpi_camera* camera, int level ) {
	if ( !camera )
		return stak_error_throw( "No camera specified" );

	if ( rpi_camera_output_read_frame_end( &camera->outputs[level] ) )
		return stak_error_throw( "Could not read frame" );

	return stak_error_none( );
}


#if 1

//----------
// function:
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_control_set_saturation ( struct rpi_camera* camera, int32_t saturation ) {
	if ( !camera )
		return stak_error_throw( "No camera specified" );

	if ( -100 >= saturation || saturation >= 100 )
		return stak_error_throw( "Invalid saturation value" );

	MMAL_RATIONAL_T value = {saturation, 100};
	if ( mmal_port_parameter_set_rational( camera->component->control, MMAL_PARAMETER_SATURATION, value ) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting saturation" );

	// if we got here, no errors occurred. Yay!
	return stak_error_none( );
}

//----------
// function:
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_control_set_sharpness ( struct rpi_camera* camera, int32_t sharpness ) {
	if ( !camera )
		return stak_error_throw( "No camera specified" );

	if ( -100 >= sharpness || sharpness >= 100 )
		return stak_error_throw( "Invalid sharpness value" );

	MMAL_RATIONAL_T value = {sharpness, 100};
	if ( mmal_port_parameter_set_rational( camera->component->control, MMAL_PARAMETER_SHARPNESS, value ) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting sharpness" );

	// if we got here, no errors occurred. Yay!
	return stak_error_none( );
}

//----------
// function:
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_control_set_brightness ( struct rpi_camera* camera, int32_t brightness ) {
	if ( !camera )
		return stak_error_throw( "No camera specified" );

	if ( 0 >= brightness || brightness >= 100 )
		return stak_error_throw( "Invalid brightness value" );

	MMAL_RATIONAL_T value = {brightness, 100};
	if ( mmal_port_parameter_set_rational( camera->component->control, MMAL_PARAMETER_BRIGHTNESS, value ) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting brightness" );

	// if we got here, no errors occurred. Yay!
	return stak_error_none( );
}

//----------
// function:
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_control_set_iso ( struct rpi_camera* camera, int32_t iso ) {
	if ( !camera )
		return stak_error_throw( "No camera specified" );

	// TODO @zerotri: What is the range of the iso value?
	//if ( -100 >= iso || iso >= 100 )
	//	return stak_error_throw( "Invalid iso value" );

    if ( mmal_port_parameter_set_uint32( camera->component->control, MMAL_PARAMETER_ISO, iso ) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting iso" );

	// if we got here, no errors occurred. Yay!
	return stak_error_none( );
}

//----------
// function:
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_control_set_video_stabilisation ( struct rpi_camera* camera, int32_t vstabilisation ) {
	if ( !camera )
		return stak_error_throw( "No camera specified" );

	if ( vstabilisation != 0 && vstabilisation != 1 )
		return stak_error_throw( "Invalid stabilisation value" );

	if ( mmal_port_parameter_set_boolean(camera->component->control, MMAL_PARAMETER_VIDEO_STABILISATION, vstabilisation) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting stabilisation" );

	// if we got here, no errors occurred. Yay!
	return stak_error_none( );
}

//----------
// function:
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_control_set_exposure_compensation ( struct rpi_camera* camera, int32_t exp_comp ) {
	if ( !camera )
		return stak_error_throw( "No camera specified" );

	if ( mmal_port_parameter_set_int32(camera->component->control, MMAL_PARAMETER_EXPOSURE_COMP , exp_comp) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting exposure compensation" );

	return stak_error_none( );
}

//----------
// function:
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_control_set_metering_mode ( struct rpi_camera* camera, MMAL_PARAM_EXPOSUREMETERINGMODE_T mode ) {
	if ( !camera )
		return stak_error_throw( "No camera specified" );

	MMAL_PARAMETER_EXPOSUREMETERINGMODE_T meter_mode = {
		.hdr = {
			MMAL_PARAMETER_EXP_METERING_MODE,
			sizeof( meter_mode )
		},
		.value = mode
	};
	if ( mmal_port_parameter_set( camera->component->control, &meter_mode.hdr ) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting iso" );

	// if we got here, no errors occurred. Yay!
	return stak_error_none( );
}

//----------
// function:
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_control_set_awb_mode ( struct rpi_camera* camera, MMAL_PARAM_AWBMODE_T awb_mode ) {
	MMAL_PARAMETER_AWBMODE_T param = {
		.hdr = {
			MMAL_PARAMETER_AWB_MODE,
			sizeof(param)
		},
		.value = awb_mode
	};

	if (!camera)
		return stak_error_throw( "No camera specified" );

	if ( mmal_port_parameter_set( camera->component->control, &param.hdr ) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting awb mode" );

	// if we got here, no errors occurred. Yay!
	return stak_error_none( );
}

//----------
// function:
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_control_set_image_fx ( struct rpi_camera* camera, MMAL_PARAM_IMAGEFX_T image_fx ) {
	if (!camera)
		return stak_error_throw( "No camera specified" );
	MMAL_PARAMETER_IMAGEFX_T param = {
		.hdr = {
			MMAL_PARAMETER_IMAGE_EFFECT,
			sizeof(param)
		},
		.value = image_fx
	};

	if ( mmal_port_parameter_set( camera->component->control, &param.hdr ) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting image effect" );

	// if we got here, no errors occurred. Yay!
	return stak_error_none( );
}

//----------
// function:
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_control_set_colour_fx ( struct rpi_camera* camera, const MMAL_PARAM_COLOURFX_T *colour_fx ) {
	if (!camera)
		return stak_error_throw( "No camera specified" );

	MMAL_PARAMETER_COLOURFX_T colfx = {
		.hdr = {
			MMAL_PARAMETER_COLOUR_EFFECT,
			sizeof(colfx)
		},
		.enable = colour_fx->enable,
		.u = colour_fx->u,
		.v = colour_fx->v
	};

	if ( mmal_port_parameter_set( camera->component->control, &colfx.hdr ) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting colour effect" );

   // if we got here, no errors occurred. Yay!
	return stak_error_none( );

}

//----------
// function:
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_control_set_rotation ( struct rpi_camera* camera, int32_t rotation ) {
	int my_rotation = ((rotation % 360 ) / 90) * 90;

	if ( mmal_port_parameter_set_int32(camera->component->output[0], MMAL_PARAMETER_ROTATION, my_rotation) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting rotation" );

	if ( mmal_port_parameter_set_int32(camera->component->output[1], MMAL_PARAMETER_ROTATION, my_rotation) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting rotation" );

	if ( mmal_port_parameter_set_int32(camera->component->output[2], MMAL_PARAMETER_ROTATION, my_rotation) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting rotation" );

	// if we got here, no errors occurred. Yay!
	return stak_error_none( );
}

//----------
// function:
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_control_set_flips ( struct rpi_camera* camera, int32_t hflip, int32_t vflip ) {
	if ( !camera )
		return stak_error_throw( "No camera specified" );

	MMAL_PARAMETER_MIRROR_T mirror = {
		.hdr = {
			MMAL_PARAMETER_MIRROR,
			sizeof(MMAL_PARAMETER_MIRROR_T)
		},
		.value = ( (hflip) ? MMAL_PARAM_MIRROR_HORIZONTAL : 0 ) |
				 ( (vflip) ? MMAL_PARAM_MIRROR_HORIZONTAL : 0 ),
	};

   if ( mmal_port_parameter_set( camera->component->output[0], &mirror.hdr ) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting flip" );

	if ( mmal_port_parameter_set( camera->component->output[1], &mirror.hdr ) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting flip" );

   if ( mmal_port_parameter_set( camera->component->output[2], &mirror.hdr ) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting flip" );

	// if we got here, no errors occurred. Yay!
	return stak_error_none( );
}

//----------
// function:
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_control_set_roi ( struct rpi_camera* camera, PARAM_FLOAT_RECT_T rect ) {
	if ( !camera )
		return stak_error_throw( "No camera specified" );

	MMAL_PARAMETER_INPUT_CROP_T crop = {
		.hdr = {
			MMAL_PARAMETER_INPUT_CROP,
			sizeof(MMAL_PARAMETER_INPUT_CROP_T)
		},
		.rect = {
			.x = rect.x * 65536,
			.y = rect.y * 65536,
			.width = rect.w * 65536,
			.height = rect.h * 65536
		}
	};

   if ( mmal_port_parameter_set(camera->component->control, &crop.hdr) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting region of interest" );

	return stak_error_none( );
}

//----------
// function:
// parameters:
//   object:
//     camera - pointer to camera instance
//   outputs:
//   inputs:
//----------
int rpi_camera_control_set_shutter_speed ( struct rpi_camera* camera, int32_t speed_ms ) {
	if ( !camera )
		return stak_error_throw( "No camera specified" );

	if ( mmal_port_parameter_set_uint32(camera->component->control, MMAL_PARAMETER_SHUTTER_SPEED , speed_ms) != MMAL_SUCCESS )
		return stak_error_throw( "Error setting shutter speed" );

	return stak_error_none( );
}
#endif