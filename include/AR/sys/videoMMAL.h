/*
 *	videoMMAL.h
 *  ARToolKit5
 *
 *  Video capture module utilising the GStreamer pipeline for AR Toolkit
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

#ifndef AR_VIDEO_MMAL_H
#define AR_VIDEO_MMAL_H


#include <AR/ar.h>
#include <AR/video.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _AR2VideoParamMMALT AR2VideoParamMMALT;

int                    ar2VideoDispOptionMMAL     ( void );
AR2VideoParamMMALT    *ar2VideoOpenMMAL           ( const char *config_in );
int                    ar2VideoCloseMMAL          ( AR2VideoParamMMALT *vid );
int                    ar2VideoGetIdMMAL          ( AR2VideoParamMMALT *vid, ARUint32 *id0, ARUint32 *id1 );
int                    ar2VideoGetSizeMMAL        ( AR2VideoParamMMALT *vid, int *x,int *y );
AR_PIXEL_FORMAT        ar2VideoGetPixelFormatMMAL ( AR2VideoParamMMALT *vid );
AR2VideoBufferT       *ar2VideoGetImageMMAL       ( AR2VideoParamMMALT *vid );
int                    ar2VideoCapStartMMAL       ( AR2VideoParamMMALT *vid );
int                    ar2VideoCapStopMMAL        ( AR2VideoParamMMALT *vid );

int                    ar2VideoGetParamiMMAL      ( AR2VideoParamMMALT *vid, int paramName, int *value );
int                    ar2VideoSetParamiMMAL      ( AR2VideoParamMMALT *vid, int paramName, int  value );
int                    ar2VideoGetParamdMMAL      ( AR2VideoParamMMALT *vid, int paramName, double *value );
int                    ar2VideoSetParamdMMAL      ( AR2VideoParamMMALT *vid, int paramName, double  value );


#ifdef  __cplusplus
}
#endif
#endif // !AR_VIDEO_MMAL_H
