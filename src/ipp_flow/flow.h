#ifndef _IPP_FLOW_H__
#define _IPP_FLOW_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ipp.h>
#include <ippcv.h>
#include "common/ppm.h"

#include "image/image.h"

typedef struct _UnitFlow {
    
    int width;
    int height;
    IppiSize roi;

    IppiPyramid * prev_pyr;
    IppiPyramid * next_pyr;
    //FrameBuffer * prev_buf;
    //GLUtil * gl_util;
    //int gl_refcount;
    IppiPoint_32f * prevpts;
    IppiPoint_32f * nextpts;
    Ipp8s* ptstatus;
    Ipp32f * errors;
    int numpts;

    //float * eigens;
    //int eigen_stride;


    int winSize;
    int maxlevel;
    float error_thresh;

} UnitFlow;

void
free_gaussian_pyramid (IppiPyramid * pyr);
IppiPyramid *
generate_gaussian_pyramid (ImgGray *src,
        int level, float rate, int kernel_size, int save);
void
IppInitEmtpyOpticalFlow ( UnitFlow *uc );
void
IppInitOpticalFlow ( UnitFlow *uc, int width, int height, IppiSize roi, int winSize, \
                     int maxlevel, int n, float error_thresh);
void
IppFreeOpticalFlow ( UnitFlow *uc );
void 
IppComputeOpticalFlow( UnitFlow *uc, \
                       int maxIter, Ipp32f threshold );

#endif
