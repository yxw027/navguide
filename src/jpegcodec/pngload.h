#ifndef _PNG_LOAD_H__
#define _PNG_LOAD_H__

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "common/dbg.h"

#include <png.h>
//#include "image/image.h"

#include "ippdefs.h"
#include "ippcore.h"
#include "ipps.h"
#include "ippi.h"
#include "ippcc.h"
#include "ippj.h"

//ImgColor* load_png(const char* fileName, int *width, int *height, int *channels );
void get_png_info(const char* fileName, int *width, int *height, int *channels );
//void write_png( ImgColor *img, const char *filename );
//void write_png( ImgGray *img, const char *filename );
void write_png (unsigned char *img, int width, int height, int nchannels, const char *filename);

#endif
