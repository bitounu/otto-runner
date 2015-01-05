#include "bcm_host.h"
#include "interface/vcos/vcos.h"
#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/mmal_component.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"
#include <stdint.h>

typedef struct
{
   double x;
   double y;
   double w;
   double h;
} PARAM_FLOAT_RECT_T;

typedef struct
{
   int enable;       /// Turn colourFX on or off
   int u,v;          /// U and V to use
} MMAL_PARAM_COLOURFX_T;

/// struct contain camera settings
typedef struct
{
	int sharpness;             /// -100 to 100
	int contrast;              /// -100 to 100
	int brightness;            ///  0 to 100
	int saturation;            ///  -100 to 100
	int iso;                   ///  TODO : what range?
	int video_stabilisation;    /// 0 or 1 (false or true)
	int exposure_compensation;  /// -10 to +10 ?
	MMAL_PARAM_EXPOSUREMODE_T exposure_mode;
	MMAL_PARAM_EXPOSUREMETERINGMODE_T exposure_meter_mode;
	MMAL_PARAM_AWBMODE_T awb_mode;
	MMAL_PARAM_IMAGEFX_T image_effect;
	MMAL_PARAMETER_IMAGEFX_PARAMETERS_T image_effects_parameters;
	MMAL_PARAM_COLOURFX_T colour_effects;
	int rotation;              /// 0-359
	int hflip;                 /// 0 or 1
	int vflip;                 /// 0 or 1
	PARAM_FLOAT_RECT_T  roi;   /// region of interest to use on the sensor. Normalised [0,1] values in the rect
	int shutter_speed;         /// 0 = auto, otherwise the shutter speed in ms
} RASPICAM_CAMERA_PARAMETERS;

struct rpi_camera_output {
	int						width;
	int						height;
	MMAL_COMPONENT_T*		resizer_component;
	MMAL_CONNECTION_T*		connection;
	MMAL_BUFFER_HEADER_T*	locked_buffer;
	MMAL_POOL_T*			buffer_pool;
	MMAL_QUEUE_T*			output_queue;
	MMAL_PORT_T*			buffer_port;
} rpi_camera_output;

struct rpi_camera_settings {
	int width;
	int height;
	int frame_rate;
	int num_levels;
	int do_argb_conversion;
};

struct rpi_camera {
	struct rpi_camera_settings settings;
	struct rpi_camera_output outputs[4];
	RASPICAM_CAMERA_PARAMETERS	camera_parameters;
	MMAL_COMPONENT_T*			component;    
	MMAL_COMPONENT_T*			splitter_component;
	MMAL_CONNECTION_T*			vid_to_split_connection;
};

int rpi_camera_create( struct rpi_camera* camera, struct rpi_camera_settings settings );
int rpi_camera_destroy( struct rpi_camera* camera );

int rpi_camera_read_frame( struct rpi_camera* camera, int level, void** buffer, int buffer_size );
int rpi_camera_read_frame_begin( struct rpi_camera* camera, int level, void** out_buffer, int* out_buffer_size );
int rpi_camera_read_frame_end( struct rpi_camera* camera, int level );

int rpi_camera_output_create( struct rpi_camera_output* output, int width, int height, MMAL_COMPONENT_T* input_component, int input_port_idx, int do_argb_conversion );
int rpi_camera_output_destroy( struct rpi_camera_output* output  );
int rpi_camera_output_read_frame( struct rpi_camera_output* output, void** buffer, int buffer_size, int *out_buffer_size );
int rpi_camera_output_read_frame_begin( struct rpi_camera_output* output, void** out_buffer, int* out_buffer_size );
int rpi_camera_output_read_frame_end( struct rpi_camera_output* output );
int rpi_camera_output_create_resize_component( struct rpi_camera_output* output, MMAL_PORT_T* video_output_port, int do_argb_conversion );
int rpi_camera_output_enable_port_callback_and_create_buffer_pool( struct rpi_camera_output* output, MMAL_PORT_T* port, MMAL_PORT_BH_CB_T cb, int buffer_count );

int rpi_camera_control_set_saturation ( struct rpi_camera* camera, int32_t saturation );
int rpi_camera_control_set_sharpness ( struct rpi_camera* camera, int32_t sharpness );
int rpi_camera_control_set_brightness ( struct rpi_camera* camera, int32_t brightness );
int rpi_camera_control_set_iso ( struct rpi_camera* camera, int32_t iso );
int rpi_camera_control_set_video_stabilisation ( struct rpi_camera* camera, int32_t vstabilisation );
int rpi_camera_control_set_exposure_compensation ( struct rpi_camera* camera, int32_t exp_comp );
int rpi_camera_control_set_metering_mode ( struct rpi_camera* camera, MMAL_PARAM_EXPOSUREMETERINGMODE_T mode );
int rpi_camera_control_set_awb_mode ( struct rpi_camera* camera, MMAL_PARAM_AWBMODE_T awb_mode );
int rpi_camera_control_set_image_fx ( struct rpi_camera* camera, MMAL_PARAM_IMAGEFX_T image_fx );
int rpi_camera_control_set_colour_fx ( struct rpi_camera* camera, const MMAL_PARAM_COLOURFX_T *colour_fx );
int rpi_camera_control_set_rotation ( struct rpi_camera* camera, int32_t rotation );
int rpi_camera_control_set_flips ( struct rpi_camera* camera, int32_t hflip, int32_t vflip );
int rpi_camera_control_set_roi ( struct rpi_camera* camera, PARAM_FLOAT_RECT_T rect );
int rpi_camera_control_set_shutter_speed ( struct rpi_camera* camera, int32_t speed_ms );
//int rpi_camera_callback()


