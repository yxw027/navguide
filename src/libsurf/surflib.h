/*********************************************************** 
*  --- OpenSURF ---                                        *
*  This library is distributed under the GNU GPL. Please   *
*  contact chris.evans@irisys.co.uk for more information.  *
*                                                          *
*  C. Evans, Research Into Robust Visual Features,         *
*  MSc University of Bristol, 2008.                        *
*                                                          *
************************************************************/

#ifndef SURFLIB_H
#define SURFLIB_H

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "integral.h"
#include "fasthessian.h"
#include "surf.h"
#include "ipoint.h"
#include "utils.h"
// from LCM
#include <common/lcm_util.h>

static IplImage * g_surf_img = NULL;
static IplImage * g_inte_img = NULL;

//! Library function builds vector of described interest points
inline void surfDetDes(IplImage *img,  /* image to find Ipoints in */
        std::vector<Ipoint> &ipts, /* reference to vector of Ipoints */
        bool upright = false, /* run in rotation invariant mode? */
        int octaves = OCTAVES, /* number of octaves to calculate */
        int intervals = INTERVALS, /* number of intervals per octave */
        int init_sample = INIT_SAMPLE, /* initial sampling step */
        float thres = THRES, /* blob response threshold */
        bool extended = false)
{
    // Create integral-image representation of the image
    IplImage *int_img = Integral(img);

    // Create Fast Hessian Object
    FastHessian fh(int_img, ipts, octaves, intervals, init_sample, thres);

    // Extract interest points and store in vector ipts
    fh.getIpoints(extended ? 128 : 64);

    // Create Surf Descriptor Object
    Surf des(int_img, ipts);

    // Extract the descriptors for the ipts
    des.getDescriptors(upright);

    // Deallocate the integral image
    cvReleaseImage(&int_img);
}


//! Library function builds vector of interest points
inline void surfDet(IplImage *img,  /* image to find Ipoints in */
        std::vector<Ipoint> &ipts, /* reference to vector of Ipoints */
        int octaves = OCTAVES, /* number of octaves to calculate */
        int intervals = INTERVALS, /* number of intervals per octave */
        int init_sample = INIT_SAMPLE, /* initial sampling step */
        float thres = THRES, /* blob response threshold */
        bool extended = false)
{
    // Create integral image representation of the image
    IplImage *int_img = Integral(img);

    // Create Fast Hessian Object
    FastHessian fh(int_img, ipts, octaves, intervals, init_sample, thres);

    // Extract interest points and store in vector ipts
    fh.getIpoints(extended ? 128 : 64);

    // Deallocate the integral image
    cvReleaseImage(&int_img);
}




//! Library function describes interest points in vector
inline void surfDes(IplImage *img,  /* image to find Ipoints in */
        std::vector<Ipoint> &ipts, /* reference to vector of Ipoints */
        bool upright = false) /* run in rotation invariant mode? */
{ 
    // Create integral image representation of the image
    IplImage *int_img = Integral(img);

    // Create Surf Descriptor Object
    Surf des(int_img, ipts);

    // Extract the descriptors for the ipts
    des.getDescriptors(upright);

    // Deallocate the integral image
    cvReleaseImage(&int_img);
}

inline navlcm_feature_list_t * surf_detect (unsigned char *data, int width, int height, int sensorid, int64_t timestamp, int octaves, int intervals, int init_sample, double thresh, bool extended)
{
    int DESC_SIZE = extended ? 128 : 64;

    IpVec ipts;
    navlcm_feature_list_t *features = navlcm_feature_list_t_create (width, height, sensorid, timestamp, DESC_SIZE);

    // create OpenCV image from the data
    if (!g_surf_img)
        g_surf_img = cvCreateImage ( cvSize (width, height), IPL_DEPTH_8U, 1);

    memcpy (g_surf_img->imageData, data, width * height);

    // detect and describe interest points in the image
    surfDetDes(g_surf_img, ipts, false, octaves, intervals, init_sample, thresh, extended);

    // convert to lcmtypes
    Ipoint *ipt;
    
    for(unsigned int i = 0; i < ipts.size(); i++) 
    {
        ipt = &ipts.at(i);

        navlcm_feature_t *ft = (navlcm_feature_t*)malloc(sizeof(navlcm_feature_t));
        ft->scale = ipt->scale;
        ft->col = ipt->x;
        ft->row = ipt->y;
        ft->ori = ipt->orientation;
        ft->laplacian = ipt->laplacian;
        ft->sensorid = sensorid;
        ft->index = i;
        ft->uid = i;
        ft->utime = timestamp;
        ft->size = DESC_SIZE;//64;
        ft->data = (float*)malloc(DESC_SIZE/*64*/*sizeof(float));
        for (int i=0;i<DESC_SIZE/*64*/;i++)
            ft->data[i] = ipt->descriptor[i];

        features = navlcm_feature_list_t_append (features, ft);

        navlcm_feature_t_destroy (ft);
    }

    ipts.clear ();

    return features;
}


#endif
