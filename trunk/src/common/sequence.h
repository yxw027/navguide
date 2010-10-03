#ifndef _SEQUENCE_H__
#define _SEQUENCE_H__

#include <stdio.h>
#include "log.h"
#include "jpegcodec/imgload.h"
#include "camera/camera.h"

//typedef enum seq_camera_type_t { LADYBUG };
//typedef enum seq_image_format { JPG, PNG, PPM };

// sequence information
//
// date format is yyyymmdd

typedef struct _sequence_info { 

    camera_type_t camera; 
    image_format_t format; 
    int width;
    int height;
    int frate; 
    int n_frames; 
    int date; 
    char *dirname; 
    int n_sensors;

} sequence_info;

sequence_info read_sequence_info( const char *dname );
void clear_sequence_info( sequence_info &info ) ;

void get_frame_filename( int frameid, int sensorid, 
                         sequence_info seqinfo, char *filename );


#endif
