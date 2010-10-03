#ifndef _JPEG_LOAD_H__
#define _JPEG_LOAD_H__

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "common/dbg.h"

//#include "image/image.h"

//#include "ippdefs.h"
//#include "ippcore.h"
//#include "ipps.h"
//#include "ippi.h"
//#include "ippcc.h"
//#include "ippj.h"
#include "encoder.h"
#include "decoder.h"
#include "metadata.h"

/*ImgColor*
load_jpeg(const char* fileName, int *width, int *height, int *channels );
*/
void
get_jpeg_info(const char* fileName, int *width, int *height, int *channels );

/*void write_jpeg( ImgColor *src, const char *filename, int width, 
                 int height, int nchannels );
                 */
void write_jpeg( unsigned char *src, const char *filename, int width, 
                 int height, int nchannels );
unsigned char *jpeg_decompress (unsigned char *src, int size, int *width, 
                                int *height, int *nchannels);

#endif
