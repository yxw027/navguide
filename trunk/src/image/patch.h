#ifndef _PATCH_H__
#define _PATCH_H__

/* From standard C library */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <math.h>

/* From IPP */
#include "ippdefs.h"
#include "ippcore.h"
#include "ipps.h"
#include "ippi.h"
#include "ippcc.h"
#include "ippj.h"

/* From the GL library */
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include "common/dbg.h"
#include "image/image.h"
#include "common/gltool.h"

#include "math/geom.h"
#include "math/tool.h"
#include "jpegcodec/imgload.h"

#include "sift/key.h"

static const int PATCH_SIZE[4] = {9,15,21,31}; // patch sizes ** MUST BE ODD NUMBERS!

#define MAX_PATCH_RES_LEVEL 15 // number of resolution levels for surface patches
#define PATCH_ORI_BINS 36 // number of bins for orientation histograms

typedef struct { unsigned char vec[SIFT_VEC_LENGTH]; float angle; } SiftDescriptor;

/* a class for a 2D patch */

class Patch2D {

    ImgColor *_img;   // a small color patch
    ImgGray *_img_g;   // color patch converted to grayscale

    int _c,_r; // position on the image
    int _id; 
    int *_pos; // pixel positions on the image as a series of c,r
    float _ncc;  // normalized cross-correlation

 public:

    // constructors
    Patch2D();
    Patch2D (const Patch2D *);
    Patch2D(int id, int c, int r, int w, int h);
    Patch2D(int id, int w, int h);
    ~Patch2D();

    // accessors
    inline int Width() const { return _img->Width(); };
    inline int Height() const { return _img->Height(); };
    inline int c() const { return _c; };
    inline int r() const { return _r; };
    inline void c(int v) { _c = v;};
    inline void r(int v) { _r = v;};
    inline float NCC() const { return _ncc; }
    inline int id() const { return _id; };
    inline void Id(int id) { _id = id; };
    inline int Surface() const { return _img->Width()*_img->Height(); };
    
    // methods
    void SetPixel( int c, int r, unsigned char rc, unsigned char gc, unsigned char bc );
    void GetPixel( int c, int r, unsigned char &rc, unsigned char &gc, unsigned char &bc );
    void SetPixelPos( int count, int c, int r );
    void GetPixelPos( int count, int &c, int &r );
    void UpdateGrayscale();
    void Draw( int x0, int y0, int w, int h, int zoom, int current_id );
    void GaussianBlur(int size);
    Vec3f Average();
    void ComputeNCC( Patch2D *);
    void ComputeGradient( float *ori, float *mag );
    void ComputeSiftDescriptor( float angle, unsigned char *vec );
    void AssignOrientations();
    float Orientation();
    void Compute_Jet( int n, float scale, float *vect );

    void Write( FILE *fp );
    void Read ( FILE *fp );

    void Rotate ( float angle );

};

// global methods
void AveragePatches( vector<Patch2D*> patches, Patch2D *patch );

#endif
