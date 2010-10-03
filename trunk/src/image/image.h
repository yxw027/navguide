#ifndef _IMAGE_H__
#define _IMAGE_H__

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

/* from common */
#include <common/dbg.h>
#include <common/mathutil.h>
#include <common/lcm_util.h>

/* from lcm */
#include <bot/bot_core.h>

/* from camunits */
#include <camunits/cam.h>

/* from codecs */
#include <jpegcodec/ppmload.h>
#include <jpegcodec/pngload.h>

#ifndef ABS
#define ABS(x)    (((x) > 0) ? (x) : (-(x)))
#define MAX(x,y)  (((x) > (y)) ? (x) : (y))
#define MIN(x,y)  (((x) < (y)) ? (x) : (y))
#endif

#define GAUSS_CUTOFF 5 /* to choose the size for the gaussian window */
                       /* if the value is too small then sum of the 
			  derivatives of the gaussian are not equal
			  to zero */

#define JET_VEC_LENGTH 10

class ImgGray {

    int _width;
    int  _height;
    Ipp8u *_iplImage;

    ImgGray(ImgGray &img); // hide default copy constructor
    void Init( int width, int height );
    
 public:
    
    ImgGray (int width, int height);
    ImgGray (const ImgGray *img);
    ~ImgGray ();

    // access pointer to data
    inline Ipp8u *IplImage() const { return _iplImage; };
    inline Ipp8u *IplImagePtr() { return _iplImage; };
    inline int Step() const { return 3 * _width; };    
    inline IppiSize Roi() const { IppiSize roi = {_width,  _height}; return roi; };

    inline int GetPixel( int c, int r ) const { return (int)_iplImage[c+_width*r]; };
    inline void SetPixel( int c, int r, unsigned char g ) { _iplImage[c+_width*r] = g; };

    // get image info
    inline int Width() const { return _width; };
    inline int Height() const { return _height; };
    inline int MemUsed() const { return _width * _height; };

    // clear image to a specific grayscale level
    void Clear( int level );
    void Write( FILE* );
    void Read( FILE* );

    // bilinear interpolation
    int Bilinear ( float c, float r, unsigned char &g );

    // raw moments
    void RawMoment0 ( float &);
    void RawMoment1 (float &, float &);
    
    // central moments
    void CentralMoment( int, int, float & );
    float Orientation();
    void GaussianBlur( int size );

    // derivatives
    float zero(int c, int r, float scale);
    float First_x(int c, int r, float scale);
    float First_y(int c, int r, float scale);
    float Second_xx(int c, int r,float scale);
    float Second_xy(int c, int r,float scale);
    float Second_yy(int c, int r,float scale);
    float Third_xxx(int c, int r,float scale);
    float Third_xxy(int c, int r,float scale);
    float Third_xyy(int c, int r,float scale);
    float Third_yyy(int c, int r,float scale);
    int Jet_calc(int x, int y, int n, float scale, float *jet);

    // transformations
    ImgGray *transpose ();
    void set_width (int);
    unsigned char *histogram ();
    void set_first_column (int col);

    inline void SetData (unsigned char *data, int stride) 
    { ippiCopy_8u_C1R (data, stride, _iplImage, stride, Roi()); }

};



class ImgColor {
    
    int _width;
    int _height;
    Ipp8u *_iplImage;

    ImgColor(ImgColor &img); // hide default copy constructor
    void Init( int width, int height );
    
 public:
    
    ImgColor (int width, int height);
    ImgColor (const ImgColor *img);
    ~ImgColor ();

    // access pointer to data
    inline Ipp8u *IplImage() const { return _iplImage; };
    inline Ipp8u *IplImagePtr() { return _iplImage; };
    
    // get image info
    inline int Width() const { return _width; };
    inline int Height() const { return _height; };
    inline int MemUsed() const { return 3 * _width * _height; };

    inline int Step() const { return 3 * _width; };
    
    inline IppiSize Roi() const { IppiSize roi = {_width,  _height}; return roi; };
    
    inline int GetPixel( int c, int r, int n ) const { return (int)_iplImage[3*(c+_width*r)+n]; };
    inline void GetPixel( int c, int r, unsigned char &cr, unsigned char &cg, unsigned char &cb) {
      cr = _iplImage[3*(c+_width*r)]; cg = _iplImage[3*(c+_width*r)+1]; 
      cb = _iplImage[3*(c+_width*r)+2]; };
    inline void SetPixel( int c, int r, unsigned char cr, unsigned char cg, unsigned char cb ) 
        { _iplImage[3*(c+_width*r)] = cr;
        _iplImage[3*(c+_width*r)+1] = cg; _iplImage[3*(c+_width*r)+2] = cb;};
    

    // clear image to the specified color
    void Clear(int r, int g, int b);
    void Write( FILE* );
    void Read( FILE* );
    void Copy (Ipp8u *data );

    // convert to grayscale
    void Grayscale( ImgGray *imgray);
    ImgGray *to_grayscale ();
    unsigned char *histogram ();

    // bilinear interpolation
    int Bilinear ( float c, float r, unsigned char &rc, \
                    unsigned char &gc, unsigned char &bc );

    void GaussianBlur( int size );
    void Rotate( float angle );

    inline void SetData (unsigned char *data, int stride) 
    { ippiCopy_8u_C3R (data, stride, _iplImage, stride, Roi()); }

    void set_pixels (int col, int row, unsigned char *data, 
                     int width, int height);
    void set_width (int width);

};

void image_rgb_to_hsv (unsigned char *src, double *dst, int size);
double *image_rgb_to_gray_hsv (unsigned char *src, unsigned char *dst, int size);
double image_hsv_dist (double *hsv_a, double *hsv_b);
void image_hsv_to_rgb (double *hsv, double *rgb);

void phone_send_image (lcm_t *lcm, botlcm_image_t *img, int force_height, int channel);
void phone_send_image (lcm_t *lcm, unsigned char *data, 
                       int width, int height, int force_height,
                       int channel);
void phone_send_image (lcm_t *lcm, ImgColor *src, 
                       int force_height, int channel);
void image_draw_point_C3R (Ipp8u *src, int width, int height,
                        int col, int row, int size, int red, int green, int blue);
void image_draw_point_C1R (Ipp8u *src, int width, int height,
                        int col, int row, int size, int value);
Ipp8u *grayscale_to_color (Ipp8u *src, int width, int height);

botlcm_image_t *fetch_image_patch (botlcm_image_t *img, int col, int row, int size);
botlcm_image_t *patch_difference (botlcm_image_t *p1, botlcm_image_t *p2);
botlcm_image_t *rotate_image (botlcm_image_t *img, double angle);
void image_save_tile_to_file (botlcm_image_t *img, int n, const char *filename);
void image_stitch_image_to_file (GQueue *img, const char *filename);

#endif
