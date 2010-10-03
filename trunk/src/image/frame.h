#ifndef _FRAME_H__
#define _FRAME_H__

/* From standard C library */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>

#include "image/image.h"
#include "jpegcodec/jpegload.h"

using namespace std;

class frame_t {
 public:

    int id;             // frame ID

    vector<ImgColor*> img;   // color
    vector<ImgGray*> img_g;  // grayscale

    char _valid;

    //vector<Ipp32f*> img_eigen; // min eigen values
    /* methods */
    frame_t();
    ~frame_t() { clear(); _valid = 0;};

    void clear();
    void init (int n, int w, int h, int frameid);
    void convert_grayscale();
    void convert_grayscale (int sid);
    inline int n_img() { return (int)img.size(); }
    inline char valid() { return _valid; }
    inline void set_data (int sensor, unsigned char *data, int stride) 
    { img[sensor]->SetData (data, stride);
        convert_grayscale (sensor); }
    void write (int sid, const char *filename);
    int get_width (int sid);
    int get_height (int sid);
    int get_max_width ();
    int get_max_height ();

    inline int compute_histogram (int sid, const Ipp32s *levels, Ipp32s *histo, int n) {
        if (sid >= n_img()) return -1;
        IppiSize size = { get_width (sid), get_height (sid) };
        ippiHistogramRange_8u_C1R (img_g[sid]->IplImage(), get_width (sid),
                                   size, histo, levels, n);
        return 0;
    }
};

#endif
