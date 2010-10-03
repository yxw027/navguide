#ifndef _PPM_LOAD_H__
#define _PPM_LOAD_H__

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <common/dbg.h>

void write_ppm (unsigned char *data, int width, int height, const char *fname);
void write_pgm (unsigned char *data, int width, int height, const char *fname);

#endif
