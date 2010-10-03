#ifndef _IMAGEPOOL_H__
#define _IMAGEPOOL_H__

/* From standard C library */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>

/* From IPP */
#include "ippdefs.h"
#include "ippcore.h"
#include "ipps.h"
#include "ippi.h"
#include "ippcc.h"
#include "ippj.h"

#include "common/dbg.h"
#include "common/log.h"
#include "image.h"

/* From the STL library */
#include <vector>
#include <algorithm>
using namespace std;

#define POOL_MAX_IMAGES 400

/* a class for a pool of images */

class ImagePool {

    vector< std::pair< int, ImgColor* > > _images; // an image and a frame ID

 public:

    ImagePool();
    ~ImagePool();

    void LoadImage( ImgColor *img, int frameid );
    ImgColor *GetImage( int frameid );
    void PopImage();
    void Clear();
};

    
#endif
