/*
 *	videoMMAL.c
 *  ARToolKit5
 *
 *  Video capture module utilising the MMAL for AR Toolkit
 *
 *  This file is part of ARToolKit.
 *
 *  ARToolKit is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ARToolKit is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with ARToolKit.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As a special exception, the copyright holders of this library give you
 *  permission to link this library with independent modules to produce an
 *  executable, regardless of the license terms of these independent modules, and to
 *  copy and distribute the resulting executable under terms of your choice,
 *  provided that you also meet, for each linked independent module, the terms and
 *  conditions of the license of that module. An independent module is a module
 *  which is neither derived from nor based on this library. If you modify this
 *  library, you may extend this exception to your version of the library, but you
 *  are not obligated to do so. If you do not wish to do so, delete this exception
 *  statement from your version.
 *
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2008-2015 ARToolworks, Inc.
 *  Copyright 2003-2008 Hartmut Seichter <http://www.technotecture.com>
 *
 *  Author(s): the78est herry
 *
 */
f
#include <AR/video.h>

/* using memcpy */
#include <string.h>

#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"


#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2

//#define GSTREAMER_TEST_LAUNCH_CFG "videotestsrc ! video/x-raw-rgb,bpp=24 ! identity name=artoolkit sync=true ! fakesink"

struct _AR2VideoParamMMALT 
{	
	/* size and pixel format of the image */
	int	width;
	int height;
    AR_PIXEL_FORMAT pixelFormat;

	/* the actual video buffer */
    ARUint8*			videoBuffer;
    AR2VideoBufferT 	arVideoBuffer;
    
    MMAL_COMPONENT_T* 	p_camera;
    MMAL_PORT_T*		p_preview_port;
    MMAL_QUEUE_T*		p_video_queue;
    MMAL_POOL_T* 		p_video_pool;            
};

static AR2VideoParamMMALT* g_p_vid_cur = NULL;

static void gf_video_output_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	MMAL_STATUS_T ret;
	MMAL_BUFFER_HEADER_T* new_buffer;
	MMAL_BUFFER_HEADER_T* existing_buffer;
	
	//printf("video_output_callback\n\n");
	//printf("buffer info: alloc_size = %d, length = %d\n", buffer->alloc_size, buffer->length);

	//to handle the user not reading frames, remove and return any pre-existing ones
	if(mmal_queue_length(g_p_vid_cur->p_video_queue) >= 2)
	{
		existing_buffer = mmal_queue_get(g_p_vid_cur->p_video_queue);
		if(existing_buffer)
		{
			mmal_buffer_header_release(existing_buffer);
			if (port->is_enabled)
			{
				new_buffer = mmal_queue_get(g_p_vid_cur->p_video_pool->queue);
				if (new_buffer)
				{
					ret = mmal_port_send_buffer(port, new_buffer);
				}
				
				if (!new_buffer || ret != MMAL_SUCCESS)
				{
					printf("Unable to return a buffer to the video port\n\n");
				}
			}
		}
	}

	//add the buffer to the output queue
	mmal_queue_put(g_p_vid_cur->p_video_queue, buffer);

	//printf("Video buffer callback, output queue len=%d\n\n", mmal_queue_length(OutputQueue));
}

static void gf_camera_control_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer)
{
	printf("camera_control_callback\n\n");
	return;
}

static void gf_cinit_video_param(AR2VideoParamMMALT* p_vid)
{
	p_vid->p_camera 		= NULL;
	p_vid->p_preview_port 	= NULL;
	p_vid->p_video_queue 	= NULL;
	p_vid->p_video_pool		= NULL;
	return;
}

static int gf_start_camera(AR2VideoParamMMALT* p_vid)
{
	MMAL_STATUS_T ret;
	int num ;
	int c;
	MMAL_BUFFER_HEADER_T* buffer;
	
	ret = mmal_port_enable(p_vid->p_preview_port, gf_video_output_callback);
	if (ret != MMAL_SUCCESS)
	{
		return -1;
	}

	ret = mmal_component_enable(p_vid->p_camera);
	if (ret != MMAL_SUCCESS)
	{
		return -1;
	}

	num = mmal_queue_length(p_vid->p_video_pool->queue);
	printf("video_pool:queue num =%d\n", num);
	for (c = 0; c < num; ++c)
	{
		buffer = mmal_queue_get(p_vid->p_video_pool->queue);
		if (!buffer)
		{
			printf("Unable to get a required buffer %d from pool queue\n\n", c);
		}
		else if (mmal_port_send_buffer(p_vid->p_preview_port, buffer) != MMAL_SUCCESS)
		{
			printf("Unable to send a buffer to port (%d)\n\n", c);
		}
	}
	return 0;
}

static int gf_stop_camera(AR2VideoParamMMALT* p_vid)
{
	MMAL_STATUS_T ret;
	ret = mmal_component_disable(p_vid->p_camera);
	if (ret != MMAL_SUCCESS)
	{
		return -1;
	}

	ret = mmal_port_disable(p_vid->p_preview_port);
	if (ret != MMAL_SUCCESS)
	{
		return -1;
	}
	
	return 0;
}

static int gf_read_camera_frame(AR2VideoParamMMALT* p_vid)
{
	MMAL_BUFFER_HEADER_T* buf;
	MMAL_STATUS_T status;
	MMAL_BUFFER_HEADER_T *new_buffer;	
	
	buf = mmal_queue_get(p_vid->p_video_queue)
	if(!buf)
	{
		return 0;
	}
		
	memcpy(my_buffer, buf->data, buf->length);
	mmal_buffer_header_release(buf);
	
	if(g_p_preview_port->is_enabled)
	
		new_buffer = mmal_queue_get(g_p_video_pool->queue);
		if (new_buffer)
		{
			status = mmal_port_send_buffer(g_p_preview_port, new_buffer);
		}
	
		if (!new_buffer || status != MMAL_SUCCESS)
		{
			printf("Unable to return a buffer to the video port\n\n");
		}
	}
	
	return true;

}

static void gf_destory_camera(AR2VideoParamMMALT* p_vid)
{
	if (p_vid->p_video_queue)
	{
		mmal_queue_destroy(p_vid->p_video_queue);
		p_vid->p_video_queue = NULL;
	}

	if (p_vid->p_video_pool)
	{
		mmal_port_pool_destroy(p_vid->p_preview_port, p_vid->p_video_pool);
		p_vid->p_video_pool = NULL;
	}

	if (p_vid->p_camera)
	{
		mmal_component_destroy(p_vid->p_camera);
		p_vid->p_camera = NULL;
	}
	
	g_p_vid_cur = NULL;
	return;
}

static int gf_create_camera(AR2VideoParamMMALT* p_vid, int width, int height, int frame_rate)
{
	MMAL_STATUS_T ret;
	MMAL_PARAMETER_CAMERA_CONFIG_T cam_config;
	MMAL_ES_FORMAT_T* p_format;
	
	ret = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &p_vid->p_camera);
	if (ret != MMAL_SUCCESS)
	{
		return -1;
	}

	do
	{
		if (p_vid->p_camera->output_num == 0)
		{
			break;
		}

		printf("num =%d\n", p_vid->p_camera->output_num);

		p_vid->p_preview_port 	= p_vid->p_camera->output[MMAL_CAMERA_PREVIEW_PORT];

		ret = mmal_port_enable(p_vid->p_camera->control, gf_camera_control_callback);
		if (ret != MMAL_SUCCESS)
		{
			break;
		}

		cam_config.hdr.id = MMAL_PARAMETER_CAMERA_CONFIG;
		cam_config.hdr.size = sizeof(cam_config);
		cam_config.max_stills_w = width;
		cam_config.max_stills_h = height;
		cam_config.stills_yuv422 = 0;
		cam_config.one_shot_stills = 0;
		cam_config.max_preview_video_w = width;
		cam_config.max_preview_video_h = height;
		cam_config.num_preview_video_frames = 3;
		cam_config.stills_capture_circular_buffer_height = 0;
		cam_config.fast_preview_resume = 0;
		cam_config.use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC;

		ret = mmal_port_parameter_set(p_vid->p_camera->control, &cam_config.hdr);
		if (ret != MMAL_SUCCESS)
		{
			break;
		}

		p_format = p_vid->p_preview_port->format;
		p_format->encoding = MMAL_ENCODING_RGB24;
		p_format->encoding_variant = MMAL_ENCODING_VARIANT_H264_DEFAULT;
		p_format->es->video.width = width;
		p_format->es->video.height = height;
		p_format->es->video.crop.x = 0;
		p_format->es->video.crop.y = 0;
		p_format->es->video.crop.width = width;
		p_format->es->video.crop.height = height;
		p_format->es->video.frame_rate.num = frame_rate;
		p_format->es->video.frame_rate.den = 1;
		
		ret = mmal_port_format_commit(p_vid->p_preview_port);
		if (ret != MMAL_SUCCESS)
		{
			break;
		}

		ret = mmal_port_parameter_set_boolean(p_vid->p_preview_port, MMAL_PARAMETER_ZERO_COPY, MMAL_TRUE);
		if (ret != MMAL_SUCCESS)
		{
			gf_destory_camera();
			return false;
		}

		p_vid->p_preview_port->buffer_num = 8;
		p_vid->p_preview_port->buffer_size = p_vid->p_preview_port->buffer_size_recommended;
		printf("buffer_size =%d\n", p_vid->p_preview_port->buffer_size);

		p_vid->p_video_pool = mmal_port_pool_create(p_vid->p_preview_port, p_vid->p_preview_port->buffer_num, p_vid->p_preview_port->buffer_size);
		if (!p_vid->p_video_pool)
		{
			break;
		}

		p_vid->p_video_queue = mmal_queue_create();
		if (!p_vid->p_video_queue)
		{
			break;
		}
		
		p_vid->width 		= width;
		p_vid->height 		= height;
		vid->pixelFormat 	= AR_INPUT_MMAL_PIXEL_FORMAT
		
		g_p_vid_cur = p_vid;
		return 0;		
		
	} while(0);
	//Failed
	
	gf_destory_camera(p_vid);
	return -1;
}

int ar2VideoDispOptionMMAL(void)
{
    return 0;
}

AR2VideoParamMMALT* ar2VideoOpenMMAL( const char *config_in ) 
{
    const char *config 		= NULL;
    AR2VideoParamMMALT*	vid = NULL;
    int i;
    int ret;

    /* setting up defaults - we fall back to the TV test signal simulator */
    /*
    if (!config_in) config = GSTREAMER_TEST_LAUNCH_CFG;
    else if (!config_in[0]) config = GSTREAMER_TEST_LAUNCH_CFG;
    else config = config_in;
	*/

    /* init ART structure */
    arMalloc(vid, AR2VideoParamMMALT, 1);
    
    /* initialise buffer */
    vid->videoBuffer = NULL;

    /* report the current version and features */
    //g_print ("libARvideo: %s\n", gst_version_string());

    gf_init_video_param(vid);
    ret = gf_create_camera(vid, 640, 480, 30);
    if (ret < 0)
    {
    	free(vid);
    	return NULL;
    }
    
	arMalloc(vid->videoBuffer, ARUint8, (vid->width * vid->height * arVideoUtilGetPixelSize(vid->pixelFormat)));

    /* return the video handle */
    return (vid);
}

int ar2VideoCloseMMAL(AR2VideoParamMMALT* p_vid) 
{
    if (!p_vid) 
    {
    	return (-1);
    }
    
	/* stop the pipeline */
	gf_stop_camera(p_vid);
	
	/* free the pipeline handle */
	gf_destory_camera(p_vid);
	return 0;
}

int ar2VideoGetIdMMAL (AR2VideoParamMMALT* p_vid, ARUint32 *id0, ARUint32 *id1)
{
    if (!p_vid) 
    {	
    	return (-1);
    }
	
    *id0 = 0;
    *id1 = 0;
	
    return (-1);
}

int ar2VideoGetSizeMMAL(AR2VideoParamMMALT* p_vid, int* x, int* y) 
{
    if (!p_vid) 
    {
    	return (-1);
    }
    
    *x = vid->width; // width of your static image
    *y = vid->height; // height of your static image
    return (0);
}

AR_PIXEL_FORMAT ar2VideoGetPixelFormatMMAL(AR2VideoParamMMALT* p_vid)
{
    if (!p_vid)
    {
    	return (AR_PIXEL_FORMAT_INVALID);
    }
    
    return (vid->pixelFormat);
}

AR2VideoBufferT* ar2VideoGetImageMMAL(AR2VideoParamMMALT* p_vid)
{
	int ret;
    if (!p_vid) 
    {
    	return (NULL);
    }
    
    gf_read_camera_frame(p_vid);
		
	/* just return the bare video buffer */
    (vid->arVideoBuffer).buff = vid->videoBuffer;
    (vid->arVideoBuffer).fillFlag = 1;
    
    return (&(vid->arVideoBuffer));
}

int ar2VideoCapStartMMAL(AR2VideoParamMMALT* p_vid) 
{
    if (!p_vid) 
    {
    	return (-1);
    }

    return gf_start_camera(p_vid);
}

int ar2VideoCapStopMMAL(AR2VideoParamMMALT* p_vid)
{
    if (!p_vid) 
    {
    	return (-1);
    }
    
    return gf_stop_camera(p_vid);
}

int ar2VideoGetParamiMMAL( AR2VideoParamMMALT *vid, int paramName, int *value )
{
    return -1;
}
int ar2VideoSetParamiMMAL( AR2VideoParamMMALT *vid, int paramName, int  value )
{
    return -1;
}
int ar2VideoGetParamdMMAL( AR2VideoParamMMALT *vid, int paramName, double *value )
{
    return -1;
}
int ar2VideoSetParamdMMAL( AR2VideoParamMMALT *vid, int paramName, double  value )
{
    return -1;
}
