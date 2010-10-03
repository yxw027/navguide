// file: ezavi_tester.c
// desc: simple program to demonstrate the use of ezavi

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#include "ezavi.h"

///* 5 seconds stream duration */
#define STREAM_DURATION   5.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */

static void fill_bgr_image(uint8_t *img, int frame_index, int width, int height)
{
    int x, y;
    for(y=0;y<height;y++) {
        for(x=0;x<width;x++) {
            img[y*width*3+x*3+0] = (int)((x / (float)width) * 255);
            img[y*width*3+x*3+1] = (int)((y / (float)height) * 255);
            img[y*width*3+x*3+2] = (frame_index*2) % 255;
        }
    }

    for( y=height/3; y<2*height/3; y++ ) {
        for( x=0; x<width/3; x++ ) {
            img[y*width*3+x*3+0] = 255;
            img[y*width*3+x*3+1] = 0;
            img[y*width*3+x*3+2] = 0;
        }
        for( x=width/3; x<2*width/3; x++ ) {
            img[y*width*3+x*3+0] = 0;
            img[y*width*3+x*3+1] = 255;
            img[y*width*3+x*3+2] = 0;
        }
        for( x=2*width/3; x<width; x++ ) {
            img[y*width*3+x*3+0] = 0;
            img[y*width*3+x*3+1] = 0;
            img[y*width*3+x*3+2] = 255;
        }
    }
}

/**************************************************************/
/* media file output */

int main(int argc, char **argv)
{
    int status;
    char *filename;
    char *codec_name;

    if (argc != 2) {
        printf("usage: %s <file_prefix>\n"
                "\n"
               "\n", argv[0]);
        exit(1);
    }
    filename = argv[1];
    codec_name = argv[2];

    // ===
    ezavi_t *xv;

    ezavi_params_t opts;
    memset(&opts, 0, sizeof(opts));

    opts.file_prefix = filename;
    opts.codec = "raw";
    opts.width = 352;
    opts.height = 288;
    opts.src_stride = 352*3;
    opts.frame_rate = 25;
    opts.split_point = 0;

    xv = ezavi_new( &opts );
    if( NULL == xv ) {
        fprintf(stderr, "couldn't initialize EZAVI\n");
        return 1;
    }

    uint8_t *img;
    img = (uint8_t*)malloc(opts.width*opts.height*3);
    fill_bgr_image(img, 0, opts.width, opts.height);

    double video_pts;
    int frame_count;
    for(;;) {
        video_pts = ezavi_get_video_pts(xv);
        frame_count = ezavi_get_frame_count( xv );

        if (video_pts >= STREAM_DURATION) break;

        fill_bgr_image(img, frame_count, opts.width, opts.height);
        status = ezavi_write_video(xv, img);

        if( status < 0 ) {
            fprintf(stderr, "error writing video frame!\n");
            break;
        }
    }

    ezavi_finish( xv );
    ezavi_destroy( xv );
    free( img );

    return 0;
}
