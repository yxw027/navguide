#ifndef FAST_H
#define FAST_H

#include <lcmtypes/navlcm_feature_list_t.h>
#include <common/lcm_util.h>

typedef unsigned char byte;
typedef struct { int x, y; byte desc[16]; } xy; 

xy*  fast_nonmax(const byte* im, int xsize, int ysize, xy* corners, int numcorners, int barrier, int* numnx);
xy*  fast_corner_detect_9(const byte* im, int xsize, int ysize, int barrier, int* numcorners);
xy*  fast_corner_detect_10(const byte* im, int xsize, int ysize, int barrier, int* numcorners);
xy*  fast_corner_detect_11(const byte* im, int xsize, int ysize, int barrier, int* numcorners);
xy*  fast_corner_detect_12(const byte* im, int xsize, int ysize, int barrier, int* numcorners);

navlcm_feature_list_t* compute_fast (byte *im, int width, int height, 
                                       int threshold, double resize);

#endif
