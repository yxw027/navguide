#ifndef _IMGLOAD_H__
#define _IMGLOAD_H__

#include "jpegload.h"
#include "pngload.h"

enum image_format_t { EXT_JPG, EXT_PNG, EXT_NOK };

/*ImgColor *LoadImage( const char *filename,  image_format_t type, \
                     int *width, int *height, int *channels );
                     */
void GetImageInfo( const char *filename,  image_format_t type, \
                     int *width, int *height, int *channels );

//void SaveImage( ImgColor *img, const char *filename, image_format_t type );
//void SaveImage( ImgGray *img, const char *filename, image_format_t type );

#endif
