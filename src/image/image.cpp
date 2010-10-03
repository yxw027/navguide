#include "image.h"

/***********************************************
 * ImgColor: 8-bit, 3 channels per pixel
 *
 ***********************************************/

ImgColor::ImgColor( const ImgColor *img ) 
{
    Init( img->Width(), img->Height() );

    // copy pixel data
    memcpy( _iplImage, img->IplImage(), img->MemUsed() );
}

ImgColor::ImgColor ( int width, int height )
{
    Init( width, height );
}

void ImgColor::Init ( int width, int height )
{
    _width = width;
    _height = height;

    //dbg(DBG_INFO, "New Image: %d x %d (mem used: %d)\n", width, height, MemUsed());

    _iplImage = (Ipp8u*)ippMalloc( MemUsed() );
}

ImgColor::~ImgColor () 
{
    ippFree( _iplImage );
    _iplImage = NULL;
}

void ImgColor::Clear ( int r, int g, int b )
{
    Ipp8u *ptr = _iplImage;

    for (int col=0;col<Width();col++) {
        for (int row=0;row<Height();row++) {
            *ptr = r;
            ptr++;
            *ptr = g;
            ptr++;
            *ptr = b;
        }
    }
}

void ImgColor::Write( FILE *fp )
{
    assert( fp );
    
    fprintf( fp, "%d %d ", _width, _height );

    int c, r;

    for (c=0;c<_width;c++) {
        for (r=0;r<_height;r++) {

            int cr = GetPixel( c, r, 0 );
            int cg = GetPixel( c, r, 1 );
            int cb = GetPixel( c, r, 2 );

            fprintf(fp, "%d %d %d ", cr, cg, cb);
        }
    }
}

void ImgColor::Read( FILE *fp )
{
    assert( fp );

    assert( fscanf( fp, "%d%d", &_width, &_height ) == 2 );

     int c, r;

    for (c=0;c<_width;c++) {
        for (r=0;r<_height;r++) {

            int cr, cg, cb;

            assert( fscanf( fp, "%d%d%d", &cr, &cg, &cb) == 3 );

            SetPixel( c, r, (unsigned char)cr, (unsigned char)cg, (unsigned char)cb);
        }
    }
}

void ImgColor::Copy ( Ipp8u *data )
{
    memcpy( _iplImage, data, 3*_width*_height );
}

void
ImgColor::Grayscale ( ImgGray *imgray )
{
    assert( Width() == imgray->Width() &&
            Height() == imgray->Height());

    ippiRGBToGray_8u_C3C1R (_iplImage, 3*Width(), imgray->IplImagePtr(), Width(), Roi());

}

ImgGray *ImgColor::to_grayscale ()
{
    ImgGray *imgray = new ImgGray (Width(), Height());
    
    ippiRGBToGray_8u_C3C1R (_iplImage, 3*Width(), imgray->IplImagePtr(), 
                            Width(), Roi());
    
    return imgray;
}

int ImgColor::Bilinear ( float c, float r, unsigned char &rc, \
                          unsigned char &gc, unsigned char &bc )
{

    // return -1 if out of image bounds
    if ( c > _width-1 || c < 0 || r > _height-1 || r < 0 )
        return -1;

    // temporary, return closest image point
    //rc = GetPixel( int(c), int(r), 0);
    //gc = GetPixel( int(c), int(r), 1);
    //bc = GetPixel( int(c), int(r), 2);

    //return 0;

    int clow = int(floor( c ));
    int chigh = int(ceil( c ));
    int rlow = int(floor( r ));
    int rhigh = int(ceil( r ));

    float rf,gf,bf;
    double d1,d2,d3,d4;

    float crest = c - clow;
    float rrest = r - rlow;

    int cclose = crest > 0.5 ? chigh : clow;
    int rclose = rrest > 0.5 ? rhigh : rlow;

    // return -1 if out of image bounds
    if ( cclose > _width || cclose < 0 || rclose > _height || rclose < 0 )
        return -1;

    // last column
    if ( cclose == _width ) {
        if ( rclose == _height ) {
            rc = (unsigned char)GetPixel(_width-1, _height-1, 0);
            gc = (unsigned char)GetPixel(_width-1, _height-1, 1);
            bc = (unsigned char)GetPixel(_width-1, _height-1, 2);
            return 0;
        } 
        if ( rclose == 0 ) {
            rc = (unsigned char)GetPixel(_width-1, 0, 0);
            gc = (unsigned char)GetPixel(_width-1, 0, 1);
            bc = (unsigned char)GetPixel(_width-1, 0, 2);
            return 0;
        }
  
        d1 = sqrt((c-_width+1)*(c-_width+1)+(r-rclose)*(r-rclose));
        d2 = sqrt((c-_width+1)*(c-_width+1)+(r-rclose+1)*(r-rclose+1));
        double dd = d1+d2;

        rf = d1/dd*GetPixel(_width-1, rclose, 0) + d2/dd*GetPixel(_width-1, rclose-1, 0);
        gf = d1/dd*GetPixel(_width-1, rclose, 1) + d2/dd*GetPixel(_width-1, rclose-1, 1);
        bf = d1/dd*GetPixel(_width-1, rclose, 2) + d2/dd*GetPixel(_width-1, rclose-1, 2);

    }
    
    // first column
    else if ( cclose == 0 ) {
        if ( rclose == _height ) {
            rc = (unsigned char)GetPixel(0, _height-1, 0);
            gc = (unsigned char)GetPixel(0, _height-1, 1);
            bc = (unsigned char)GetPixel(0, _height-1, 2);
            return 0;
        } 
        if ( rclose == 0 ) {
            rc = (unsigned char)GetPixel(0, 0, 0);
            gc = (unsigned char)GetPixel(0, 0, 1);
            bc = (unsigned char)GetPixel(0, 0, 2);
            return 0;
        }
  
        d1 = sqrt((c)*(c)+(r-rclose)*(r-rclose));
        d2 = sqrt((c)*(c)+(r-rclose+1)*(r-rclose+1));
        double dd = d1+d2;

        rf = d1/dd*GetPixel(0, rclose, 0) + d2/dd*GetPixel(0, rclose-1, 0);
        gf = d1/dd*GetPixel(0, rclose, 1) + d2/dd*GetPixel(0, rclose-1, 1);
        bf = d1/dd*GetPixel(0, rclose, 2) + d2/dd*GetPixel(0, rclose-1, 2);

    }
    
    // first row
    else if ( rclose == 0 ) {
        d1 = sqrt((c-cclose)*(c-cclose)+r*r);
        d2 = sqrt((c-cclose+1)*(c-cclose+1)+r*r);
        double dd = d1 + d2;

        rf = d1/dd*GetPixel(cclose, 0, 0) + d2/dd*GetPixel(cclose-1, 0, 0);
        gf = d1/dd*GetPixel(cclose, 0, 1) + d2/dd*GetPixel(cclose-1, 0, 1);
        bf = d1/dd*GetPixel(cclose, 0, 2) + d2/dd*GetPixel(cclose-1, 0, 2);

    } 

    // last row
    else if ( rclose == _height ) {
        d1 = sqrt((c-cclose)*(c-cclose)+(r-_height+1)*(r-_height+1));
        d2 = sqrt((c-cclose+1)*(c-cclose+1)+(r-_height+1)*(r-_height+1));
        double dd = d1 + d2;

        rf = d1/dd*GetPixel(cclose, _height-1, 0) + d2/dd*GetPixel(cclose-1, _height-1, 0);
        gf = d1/dd*GetPixel(cclose, _height-1, 1) + d2/dd*GetPixel(cclose-1, _height-1, 1);
        bf = d1/dd*GetPixel(cclose, _height-1, 2) + d2/dd*GetPixel(cclose-1, _height-1, 2);

    } 
    
    else {
        
        d1 = sqrt((c-cclose)*(c-cclose)+(r-rclose)*(r-rclose));
        d2 = sqrt((c-cclose+1)*(c-cclose+1)+(r-rclose)*(r-rclose));
        d3 = sqrt((c-cclose)*(c-cclose)+(r-rclose+1)*(r-rclose+1));
        d4 = sqrt((c-cclose+1)*(c-cclose+1)+(r-rclose+1)*(r-rclose+1));
        double dd = d1+d2+d3+d4;

        // and finally the general case...
        rf = d1/dd*GetPixel(cclose, rclose, 0) + d2/dd*GetPixel(cclose-1, rclose, 0) + \
            d3/dd*GetPixel(cclose, rclose-1, 0) + d4/dd*GetPixel(cclose-1, rclose-1, 0);
        gf = d1/dd*GetPixel(cclose, rclose, 1) + d2/dd*GetPixel(cclose-1, rclose, 1) + \
            d3/dd*GetPixel(cclose, rclose-1, 1) + d4/dd*GetPixel(cclose-1, rclose-1, 1);
        bf = d1/dd*GetPixel(cclose, rclose, 2) + d2/dd*GetPixel(cclose-1, rclose, 2) + \
            d3/dd*GetPixel(cclose, rclose-1, 2) + d4/dd*GetPixel(cclose-1, rclose-1, 2);
    }

    rc = rf - floor(rf) > 0.5 ? (unsigned char)int(rf)+1 : (unsigned char)int(rf);
    gc = gf - floor(gf) > 0.5 ? (unsigned char)int(gf)+1 : (unsigned char)int(gf);
    bc = bf - floor(bf) > 0.5 ? (unsigned char)int(bf)+1 : (unsigned char)int(bf);
       
    return 0;
}

/* Filters an image using a Gaussian kernel */
void ImgColor::GaussianBlur( int size )
{
    IppiMaskSize mask = ( size == 3 ) ? ippMskSize3x3 : ippMskSize5x5;
    
    Ipp8u* tmp = (Ipp8u*)ippMalloc( MemUsed() );
    
    ippiFilterGauss_8u_C3R( _iplImage, Step(), tmp, Step(), Roi(), mask );
    
    memcpy( _iplImage, tmp, MemUsed() );
    
    ippFree( tmp );
    
}

/* Rotate an image */
void ImgColor::Rotate( float angle )
{
  unsigned char *tr = new unsigned char[_width*_height];
  unsigned char *tg = new unsigned char[_width*_height];
  unsigned char *tb = new unsigned char[_width*_height];
  
  int c,r;

  float cosa = cos(-angle);
  float sina = sin(-angle);

  int count = 0;

  for (c=-Width()/2;c<=Width()/2;c++) {
    for (r=-Height()/2;r<=Height()/2;r++) {
      
      if ( count >= _width*_height )
        break;

      if ( r*r + c*c >= Width()*Width()/4 ) {
        tr[count] = 0;
        tg[count] = 0;
        tb[count] = 0;
        count++;
        continue;
      }

      unsigned char rc, gc, bc;

      float ct = cosa*c - sina*r;
      float rt = sina*c + cosa*r;

      float cpos = MAX(0,MIN(Width()-1, ct + Width()/2));
      float rpos = MAX(0,MIN(Height()-1, rt + Height()/2));

      Bilinear( cpos, rpos, rc, gc, bc );

      tr[count] = rc;
      tg[count] = gc;
      tb[count] = bc;

      count++;
    }
  }

  count = 0;

  for (c=0;c<Width();c++) {
    for (r=0;r<Height();r++) {
      SetPixel( c, r, tr[count], tg[count], tb[count] );
      count++;
    }
  }

  delete []tr;
  delete []tg;
  delete []tb;

}

// resize an image to a specific width
//
void ImgColor::set_width (int width)
{
    if (width < 0 || width == Width()) return;
    
    int nwidth = width;
    int nheight = math_round (1.0 * Height() * width / Width());

    Ipp8u *src = _iplImage;
    IppiSize srcsize = {Width(), Height()};
    int srcstep = 3 * Width();
    IppiRect srcroi = {0, 0, Width(), Height()};
    
    Ipp8u *dst = (Ipp8u*)ippMalloc (nwidth * nheight * 3);
    int dststep = 3 * nwidth;
    IppiSize dstsize = {nwidth, nheight};
    double xfactor = 1.0 * nwidth / Width();
    double yfactor = 1.0 * nheight / Height();
    int interp = IPPI_INTER_LINEAR;

    ippiResize_8u_C3R (src, srcsize, srcstep, srcroi, dst, dststep,
                       dstsize, xfactor, yfactor, interp);

    _width = nwidth;
    _height = nheight;
    ippFree (_iplImage);
    _iplImage = dst;
}

void ImgColor::set_pixels (int col, int row, unsigned char *data, 
                           int width, int height)
{
    assert (col < Width() || row < Height());

    int offset = 3 * ( row * Width() + col);

    Ipp8u *src = (Ipp8u*)data;
    int srcstep = 3 * width;
    Ipp8u *dst = _iplImage + offset;
    int dststep = 3 * Width();
    IppiSize roisize = {width, height};
    
    ippiCopy_8u_C3R (src, srcstep, dst, dststep, roisize);
}

// comute a 1D histogram from an image
//
unsigned char *ImgColor::histogram ()
{
    int histsize = Width();

    ImgGray *gray = to_grayscale ();
 
    ImgGray *trans = gray->transpose ();
    
    unsigned char *hist = (unsigned char*)malloc(histsize*
                                                 sizeof(unsigned char));
    
    for (int i=0;i<histsize;i++) {
        unsigned char *ptr = (unsigned char*)_iplImage + trans->Width() * i;
        hist[i] = math_median (ptr, trans->Width());
    }
    
    delete gray;
    delete trans;

    return hist;
}
  
      
/***********************************************
 * ImgGray: 8-bit, 1 channel per pixel
 *
 **********************************************/


ImgGray::ImgGray( const ImgGray *img ) 
{
    Init( img->Width(), img->Height() );

    // copy pixel data
    memcpy( _iplImage, img->IplImage(), img->MemUsed() );
}

ImgGray::ImgGray ( int width, int height )
{
    Init( width, height );
}

void ImgGray::Init ( int width, int height )
{
    _width = width;
    _height = height;

    _iplImage = (Ipp8u*)ippMalloc( MemUsed() );
}

ImgGray::~ImgGray () 
{
    ippFree( _iplImage );
    _iplImage = NULL;
}

void ImgGray::Clear ( int level )
{
    Ipp8u *ptr = _iplImage;

    for (int col=0;col<Width();col++) {
        for (int row=0;row<Height();row++) {
            *ptr = level;
            ptr++;
        }
    }
}

ImgGray *ImgGray::transpose ()
{
    ImgGray *trans = new ImgGray (Height(), Width());

    unsigned char *ptr = _iplImage;

    for (int r=0;r<Height(); r++) {
        for (int c=0;c<Width();c++) {

            trans->SetPixel (r, c, *ptr);
            ptr++;
        }
    }

    return trans;
}

void ImgGray::set_width (int width)
{
    if (width < 0 || width == Width()) return;
    
    int nwidth = width;
    int nheight = math_round (1.0 * Height() * width / Width());

    Ipp8u *src = _iplImage;
    IppiSize srcsize = {Width(), Height()};
    int srcstep = Width();
    IppiRect srcroi = {0, 0, Width(), Height()};
    
    Ipp8u *dst = (Ipp8u*)ippMalloc (nwidth * nheight);
    int dststep = nwidth;
    IppiSize dstsize = {nwidth, nheight};
    double xfactor = 1.0 * nwidth / Width();
    double yfactor = 1.0 * nheight / Height();
    int interp = IPPI_INTER_LINEAR;

    ippiResize_8u_C1R (src, srcsize, srcstep, srcroi, dst, dststep,
                       dstsize, xfactor, yfactor, interp);

    _width = nwidth;
    _height = nheight;
    ippFree (_iplImage);
    _iplImage = dst;
}

void ImgGray::Write( FILE *fp )
{
    assert( fp );
    
    fprintf( fp, "%d %d ", _width, _height );

    int c, r;

    for (c=0;c<_width;c++) {
        for (r=0;r<_height;r++) {
            fprintf(fp, "%d ", (int)GetPixel( c, r));
        }
    }
}

void ImgGray::Read( FILE *fp )
{
    assert( fp );

    assert( fscanf( fp, "%d%d", &_width, &_height ) == 2 );

     int c, r;

    for (c=0;c<_width;c++) {
        for (r=0;r<_height;r++) {

            int val;

            assert( fscanf( fp, "%d", &val) == 1 );

            SetPixel( c, r, (unsigned char)val );
        }
    }
}

// comute a 1D histogram from an image
//
unsigned char *ImgGray::histogram ()
{
    int histsize = Width();

    ImgGray *trans = transpose ();
    
    unsigned char *hist = (unsigned char*)malloc(histsize*
                                                 sizeof(unsigned char));
    
    for (int i=0;i<histsize;i++) {
        unsigned char *ptr = (unsigned char*)_iplImage + trans->Width() * i;
        hist[i] = math_median (ptr, trans->Width());
    }
    
    delete trans;

    return hist;
}

// slides the image
//
void ImgGray::set_first_column (int col)
{
    assert (col < Width());
    
    Ipp8u *data = (Ipp8u*)ippMalloc(Width()*Height());
    
    for (int r=0;r<Height();r++) {
        int count=0;
        for (int c=col;c<Width();c++) {
            data[r*Width()+count] = GetPixel (c,r);
            count++;
        }
        for (int c=0;c<col;c++) {
            data[r*Width()+count] = GetPixel (c,r);
            count++;
        }
    }

    ippFree (_iplImage);
    _iplImage = data;
}
            
int ImgGray::Bilinear ( float c, float r, unsigned char &g )
{
    int clow = int(floor( c ));
    int chigh = int(ceil( c ));
    int rlow = int(floor( r ));
    int rhigh = int(ceil( r ));
    
    float f;
    float d1,d2,d3,d4;

    float crest = c - clow;
    float rrest = r - rlow;

    int cclose = crest > 0.5 ? chigh : clow;
    int rclose = rrest > 0.5 ? rhigh : rlow;

    // return -1 if out of image bounds
    if ( cclose > _width || cclose < 0 || rclose > _height || rclose < 0 )
        return -1;

    // last column
    if ( cclose == _width ) {
        if ( rclose == _height ) {
            g = (unsigned char)GetPixel(_width-1, _height-1);
            return 0;
        } 
        if ( rclose == 0 ) {
            g = (unsigned char)GetPixel(_width-1, 0);
            return 0;
        }
        
        d1 = sqrt((c-_width+1)*(c-_width+1)+(r-rclose)*(r-rclose));
        d2 = sqrt((c-_width+1)*(c-_width+1)+(r-rclose+1)*(r-rclose+1));
        f = d1/(d1+d2)*GetPixel(_width-1, rclose) + d2/(d1+d2)*GetPixel(_width-1, rclose-1);
        
    }
    
    // first column
    else if ( cclose == 0 ) {
        if ( rclose == _height ) {
            g = (unsigned char)GetPixel(0, _height-1);
            return 0;
        } 
        if ( rclose == 0 ) {
            g = (unsigned char)GetPixel(0, 0);
            return 0;
        }
  
        d1 = sqrt((c)*(c)+(r-rclose)*(r-rclose));
        d2 = sqrt((c)*(c)+(r-rclose+1)*(r-rclose+1));
        f = d1/(d1+d2)*GetPixel(0, rclose) + d2/(d1+d2)*GetPixel(0, rclose-1);
       
    }
    
    // first row
    else if ( rclose == 0 ) {
        d1 = sqrt((c-cclose)*(c-cclose)+r*r);
        d2 = sqrt((c-cclose+1)*(c-cclose+1)+r*r);
        f = d1/(d1+d2)*GetPixel(cclose, 0) + d2/(d1+d2)*GetPixel(cclose-1, 0);
        
    } 

    // last row
    else if ( rclose == _height ) {
        d1 = sqrt((c-cclose)*(c-cclose)+(r-_height+1)*(r-_height+1));
        d2 = sqrt((c-cclose+1)*(c-cclose+1)+(r-_height+1)*(r-_height+1));
        f = GetPixel(cclose, _height-1) + GetPixel(cclose-1, _height-1);
        
    } 
        
    else {
        // and finally the general case...
        d1 = sqrt((c-cclose)*(c-cclose)+(r-rclose)*(r-rclose));
        d2 = sqrt((c-cclose+1)*(c-cclose+1)+(r-rclose)*(r-rclose));
        d3 = sqrt((c-cclose)*(c-cclose)+(r-rclose+1)*(r-rclose+1));
        d4 = sqrt((c-cclose+1)*(c-cclose+1)+(r-rclose+1)*(r-rclose+1));
        double dd = d1+d2+d3+d4;

        f = d1/dd*GetPixel(cclose, rclose) + d2/dd*GetPixel(cclose-1, rclose) + \
            d3/dd*GetPixel(cclose, rclose-1) + d4/dd*GetPixel(cclose-1, rclose-1);
    }

    // now compute the average

    g = f - floor(f) > 0.5 ? (unsigned char)int(f)+1 : (unsigned char)int(f);       

    return 0;
}

/* Filters an image using a Gaussian kernel */
void ImgGray::GaussianBlur( int size )
{
    IppiMaskSize mask = ( size == 3 ) ? ippMskSize3x3 : ippMskSize5x5;
    
    Ipp8u* tmp = (Ipp8u*)ippMalloc( MemUsed() );
    
    ippiFilterGauss_8u_C1R( _iplImage, Step(), tmp, Step(), Roi(), mask );
    
    memcpy( _iplImage, tmp, MemUsed() );
    
    ippFree( tmp );
    
}

/* Raw and central moments for a grayscale image
 * 
 * http://en.wikipedia.org/wiki/Image_moments
 */
void ImgGray::RawMoment0( float &m )
{
    int c,r;
    
    m = 0.0;

    for (c=0;c<_width;c++) {
        for (r=0;r<_height;r++) {
            
            m += (int)_iplImage[c+_width*r];
        }
    }
}

void ImgGray::RawMoment1( float &mx, float &my )
{
    int c,r;
    
    mx = 0.0;
    my = 0.0;

    for (c=0;c<_width;c++) {
        for (r=0;r<_height;r++) {
            
            mx += c*(int)_iplImage[c+_width*r];
            my += r*(int)_iplImage[c+_width*r];
        }
    }
}

void ImgGray::CentralMoment (int i, int j, float &m )
{
    assert( i+j == 2);
    int c,r;
    m = 0.0;

    if ( i == 2 && j == 0 ) {

        for (c=0;c<_width;c++) {
            for (r=0;r<_height;r++) {
                
                m += c*c*(int)_iplImage[c+_width*r];
            }
        }  

    } else if ( i == 0 && j == 2 ) {
      
        for (c=0;c<_width;c++) {
            for (r=0;r<_height;r++) {
                
                m += r*r*(int)_iplImage[c+_width*r];
            }
        }  

    } else if ( i == 1 && j == 1 ) {

        
        for (c=0;c<_width;c++) {
            for (r=0;r<_height;r++) {
                
                m += r*c*(int)_iplImage[c+_width*r];
            }
        }  
    }
}

/* Compute the orientation for a grayscale image
 *   using central moments 
 */
float ImgGray::Orientation ()
{
    float M00, M10, M01, M20, M02, M11;

    // compute first order moments

    RawMoment0( M00 );
    
    RawMoment1( M10, M01 );
    
    float xbar = M10/M00;
    float ybar = M01/M00;

    // compute second order moments

    CentralMoment( 2, 0, M20 );
    CentralMoment( 0, 2, M02 );
    CentralMoment( 1, 1, M11 );

    float u20 = M20/M00 - xbar*xbar;
    float u02 = M02/M00 - ybar*ybar;
    float u11 = M11/M00 - xbar*ybar;

    if ( fabs(u20-u02) < 1E-4 ) {
        
        if ( u11 > 1E-4 )
            return M_PI/2;
        if ( u11 < -1E-4 )
            return -M_PI/2;
        
        return 0.0;
    }

    return .5 * atan2( 2.0 * u11 , (u20 - u02) );
}

// HSV to RGB conversion
void image_hsv_to_rgb (double *hsv, double *rgb)
{
    double h = clip_value (hsv[0], -PI, PI, 1E-6) + PI;
    int hi = MIN (5, MAX (0, (int)floor (h / (PI/3.0))));

    double v = hsv[2];
    double f = h / (PI/3.0)- hi;
    double p = v * (1.0 - hsv[1]);
    double q = v * (1.0 - f * hsv[1]);
    double t = v * ( 1.0 - (1.0-f)*hsv[1]);

    switch (hi) {
    case 0:
        rgb[0] = v; rgb[1] = t; rgb[2] = p;
        break;
    case 1:
        rgb[0] = q; rgb[1] = v; rgb[2] = p;
        break;
    case 2:
        rgb[0] = p; rgb[1] = v; rgb[2] = t;
        break;
    case 3:
        rgb[0] = p; rgb[1] = q; rgb[2] = v;
        break;
    case 4:
        rgb[0] = t; rgb[1] = p; rgb[2] = v;
        break;
    case 5:
        rgb[0] = v; rgb[1] = p; rgb[2] = q;
        break;
    default:
        fprintf (stderr, "error in hsv to rgb conversion. unexpected hi value: %d, h = %.4f\n", hi, h);
        assert (false);
    }
}

// RGB to HSV conversion
void image_rgb_to_hsv (unsigned char *rgb, double *hsv)
{
    double r = 1.0*rgb[0]/255;
    double g = 1.0*rgb[1]/255;
    double b = 1.0*rgb[2]/255;
    
    double max = fmax (r, fmax (g, b));
    double min = fmin (r, fmin (g, b));

    if (max-min < 1E-6) {
        hsv[0] = 0.0;
        hsv[1] = 0.0;
    } else {
        if (g > r && g > b) {
            hsv[0] = PI/3 * (b-r)/(max-min) + 2*PI/3;
        } else if (b > r && b > g) {
            hsv[0] = PI/3 * (r-g)/(max-min) + 4*PI/3;
        } else { // r > g && r > b
            if (g < b) {
                hsv[0] = PI/3 * (g-b)/(max-min) + 2*PI;
            } else {
                hsv[0] = PI/3 * (g-b)/(max-min);
            }
        }
        hsv[1] = 1.0 - min/max;
    }

    hsv[2] = max;
}

// convert a 3-channel RGB image to a single channel image using HSV formulation
//
// if Saturation > threshold, we use the hue; otherwise, we use the value (intensity).
// 
// the threshold is a function of the value: threshold = 1.0 - .8 * value
//
// input range: 0-255
// output range: 0-255
//
double *image_rgb_to_gray_hsv (unsigned char *src, unsigned char *dst, int size)
{
    double *tmp = (double*)malloc(3*size*sizeof(double));
    
    // convert to HSV
    dbg (DBG_INFO, "[image] converting to HSV...");

    image_rgb_to_hsv (src, tmp, size);

    // convert to 1-channel HSV
    dbg (DBG_INFO, "[image] converting to 1-channel HSV for size  = %d", size);

    for (int i=0;i<size;i++) {
        double thresh = 1.0 - .8 * tmp[3*i+2];
        if (tmp[3*i+1] < thresh) {
            dst[i] = MIN( 255, MAX( 0, math_round (255.0 * tmp[3*i+2]))); // use value
        } else {
            dst[i] = MIN (255, MAX (0, math_round ((clip_value (tmp[3*i], -PI, PI, 1E-6) / PI + 1.0 ) * 127.5)));
            //dst[i] = MIN (255, MAX (0, math_round ((tmp[3*i] / PI + 1.0 ) * 127.5)));
        }
    }

    return tmp;
}

void image_rgb_to_hsv (unsigned char *src, double *dst, int size)
{
    for (int i=0;i<size;i++) {
        image_rgb_to_hsv (src+3*i, dst+3*i);
    }
}

// compute the HSV signature of a blob given the HSV image
//
double image_hsv_dist (double *hsv_a, double *hsv_b)
{
    double a = hsv_a[2]-hsv_b[2];
    double b = hsv_a[1]*cos(hsv_a[0])-hsv_b[1]*cos(hsv_b[0]);
    double c = hsv_a[1]*sin(hsv_a[0])-hsv_b[1]*sin(hsv_b[0]);
    double dist =  sqrt ( (a*a + b*b + c*c) / 5.0);
    
    //    dbg (DBG_INFO, "[image] dist btw [%.3f, %.3f, %.3f] and [%.3f, %.3f, %.3f] = %.4f",
    //     hsv_a[0], hsv_a[1], hsv_a[2], hsv_b[0], hsv_b[1], hsv_b[2], dist);

    return dist;
}

// expand a blob
//
int**
image_expand_blob (unsigned char *markers, unsigned char *mask, int width, int height, int **index, int *n, int marker)
{
    if (!index)
        return NULL;

    int old_n = *n;

    for (int i=0;i<old_n;i++) {
        int c = index[0][i];
        int r = index[1][i];
        
        for (int cc=0;cc<=2;cc++) {
            for (int rr=0;rr<=2;rr++) {
                
                if (c+cc==0 || c+cc==width+1 || r+rr==0 || r+rr==height+1)
                    continue;
                
                if (mask[(r+rr-1)*width+c+cc-1] == 1)
                    continue;

                mask[(r+rr-1)*width+c+cc-1] = 1;

                if (markers[(r+rr-1)*width+c+cc-1] == marker) {
                    index[0] = (int*)realloc(index[0], (*n+1)*sizeof(int));
                    index[1] = (int*)realloc(index[1], (*n+1)*sizeof(int));
                    index[0][*n] = c+cc-1;
                    index[1][*n] = r+rr-1;
                    *n = *n+1;
                }
            }
        }
    }

    return index;  
}

        
void phone_send_image (lcm_t *lcm, botlcm_image_t *img, int force_height, int channel)
{
    Ipp8u *data = NULL;

    if (!img || img->width == 0 || img->height == 0) 
        return;

    // convert to color
    int greyscale = img->pixelformat == CAM_PIXEL_FORMAT_GRAY;

    if (greyscale) 
        data = grayscale_to_color (img->data, img->width, img->height);
    else
        data = img->data;

    // send to phone
    phone_send_image (lcm, data, img->width, img->height, force_height, channel);

    if (greyscale)
        free (data);
}

void phone_send_image (lcm_t *lcm, unsigned char *data, 
                       int width, int height, int force_height,
                       int channel)
{
    ImgColor *img = new ImgColor (width, height);

    unsigned char *ptr = data;

    for (int r=0;r<height;r++) {
        for (int c=0;c<width;c++) {
            img->SetPixel (c, r, *ptr, *(ptr+1), *(ptr+2));
            ptr+=3;
        }
    }

    phone_send_image (lcm, img, force_height, channel);

    delete img;

}

void phone_send_image (lcm_t *lcm, ImgColor *src, 
                       int force_height, int channel)
{
    if (!src) {
        fprintf (stderr, "trying to send null image to phone.");
        return;
    }

    int width = src->Width();
    int height = src->Height();

    int rwidth = width;
    int rheight = height;

    Ipp8u *rimg = NULL;
    Ipp8u *img = NULL;


    if (force_height > 0) {

        img = src->IplImage();
    
        IppiSize roi = {width, height};
        rwidth = (int)(1.0 * width * force_height / height);
        rheight = force_height;
        IppiRect rect = {0, 0, width, height};
        rimg = (Ipp8u*)ippMalloc(3*rwidth*rheight);
        IppiSize rroi = {rwidth, rheight};
        double ratio_x = 1.0 * rwidth / width;
        double ratio_y = 1.0 * rheight / height;

        ippiResize_8u_C3R (img, roi, 3*width, rect, rimg, 3*rwidth, 
                           rroi, ratio_x, ratio_y, IPPI_INTER_NN);

    } else {

        rimg = src->IplImage ();
    }
        
    botlcm_image_t msg;
    memset (&msg, 0, sizeof (msg));
    msg.width = rwidth;
    msg.height = rheight;
    msg.row_stride = 3*rwidth;
    msg.data = rimg;
    msg.size = 3*rwidth*rheight;
    msg.pixelformat = CAM_PIXEL_FORMAT_RGB;
    msg.utime = 0;
 
    char channel_str[20];
    sprintf (channel_str, "PHONE_THUMB%d", channel);

    dbg (DBG_INFO, "publishing image %d x %d to %s", rwidth, rheight, 
         channel_str);

    botlcm_image_t_publish (lcm, channel_str, &msg);

    if (force_height > 0) {
        ippFree (rimg);
    }
}

// converts a grayscale image to a color image
//
Ipp8u *grayscale_to_color (Ipp8u *src, int width, int height)
{
    Ipp8u *dst = (Ipp8u*)ippMalloc(3*width*height);

    for (int c=0;c<width;c++) {
        for (int r=0;r<height;r++) {
            int red =   src[r*width+c];
            int green = src[r*width+c];
            int blue =  src[r*width+c];
            dst[3*(r*width+c)+0] = red;
            dst[3*(r*width+c)+1] = green;
            dst[3*(r*width+c)+2] = blue;

        }
    }

    return dst;
}

// draw a point on an image
// assumes 3-channel image as input
//
void image_draw_point_C3R (Ipp8u *src, int width, int height,
                            int col, int row, int size, int red, int green, int blue)
{
    IppiSize roi = {size, size};

    int offset = 3 * ((row - MIN(row, size/2)) * width + (col - MIN(col, size/2)));
    Ipp8u *dst = src + offset;
    Ipp8u value[3];
    value[0] = red;
    value[1] = green;
    value[2] = blue;

    ippiSet_8u_C3R (value, dst, 3 * width, roi);
}

// draw a point on an image
// assumes 1-channel image as input
//
void image_draw_point_C1R (Ipp8u *src, int width, int height,
                            int col, int row, int size, int value)
{
    IppiSize roi = {size, size};

    int offset = ((row - MIN(row, size/2)) * width + (col - MIN(col, size/2)));
    Ipp8u *dst = src + offset;

    ippiSet_8u_C1R (value, dst, width, roi);
}
// extract a portion of an image
//
botlcm_image_t *fetch_image_patch (botlcm_image_t *img, int col, int row, int size)
{
    if (!img) return NULL;

    if (col < 0 || img->width <= col) return NULL;
    if (row < 0 || img->height <= row) return NULL;

    int greyscale = img->pixelformat == CAM_PIXEL_FORMAT_GRAY;
    int nchannels = greyscale ? 1 : 3;

    // create the patch
    unsigned char *dst = (unsigned char*)malloc(size*size*nchannels);
    for (int i=0;i<size*size*nchannels;i++)
        dst[i] = 0;

    // copy pixels
    for (int c=0;c<size;c++) {
        for (int r=0;r<size;r++) {
            int icol = col + c - size/2;
            int irow = row + r - size/2;
            if (0 <= icol && icol < img->width && 0 <= irow && irow < img->height) {
                if (nchannels == 1) {
                    dst[r*size+c] = img->data[irow*img->width+icol];
                } else {
                    dst[3*(r*size+c)+0] = img->data[3*(irow*img->width+icol)+0];
                    dst[3*(r*size+c)+1] = img->data[3*(irow*img->width+icol)+1];
                    dst[3*(r*size+c)+2] = img->data[3*(irow*img->width+icol)+2];
                }
            }
        }
    }

    // convert to a botlcm_image
    botlcm_image_t *oimg = botlcm_image_t_create (size, size, nchannels * size, 
            nchannels * size * size, dst, img->pixelformat, img->utime);

    // free memory
    free (dst);

    return oimg;
}

/* compute difference between two images
 */
botlcm_image_t *patch_difference (botlcm_image_t *p1, botlcm_image_t *p2)
{
    if (!p1 || !p2) return NULL;

    int greyscale = p1->pixelformat == CAM_PIXEL_FORMAT_GRAY;
    int nchannels = greyscale ? 1 : 3;

    // create new image from p1
    botlcm_image_t *out = botlcm_image_t_create (p1->width, p1->height, p1->width * nchannels,
            p1->size, p1->data, p1->pixelformat, p1->utime);

    // subtract p2
    for (int row=0;row<p1->height;row++) {
        for (int col=0;col<p1->width;col++) {
            if (nchannels == 1) {
                p1->data[row*p1->width+col] = p1->data[row*p1->width+col] - p2->data[row*p1->width+col];
            } else {
                out->data[3*(row*p1->width+col)+0] = out->data[3*(row*p1->width+col)+0] - p2->data[3*(row*p1->width+col)+0];
                out->data[3*(row*p1->width+col)+1] = out->data[3*(row*p1->width+col)+1] - p2->data[3*(row*p1->width+col)+1];
                out->data[3*(row*p1->width+col)+2] = out->data[3*(row*p1->width+col)+2] - p2->data[3*(row*p1->width+col)+2];
            }
        }
    }

    return out;
}


// rotate an image by a given angle in degrees (CCW)
//
botlcm_image_t *rotate_image (botlcm_image_t *img, double angle)
{
    if (!img) return NULL;

    int greyscale = img->pixelformat == CAM_PIXEL_FORMAT_GRAY;
    int nchannels = greyscale ? 1 : 3;

    // create an input ipp image
    Ipp8u *src = (Ipp8u*)ippMalloc(img->size);
    memcpy (src, img->data, img->size);

    // create an output ipp image
    Ipp8u *dst = (Ipp8u*)ippMalloc(img->size);
    IppiSize roi = {img->width, img->height};
    if (nchannels == 1) 
        ippiSet_8u_C1R (0, dst, img->row_stride, roi);
    else
        ippiSet_8u_C3R (0, dst, img->row_stride, roi);

    // rotate the input image
    IppiSize src_size = { img->width, img->height};
    int src_step = img->row_stride;
    IppiRect src_roi = { 0, 0, img->width, img->height};
    int dst_step = img->row_stride;
    IppiRect dst_roi = { 0, 0, img->width, img->height};
    if (nchannels == 1) 
        ippiRotateCenter_8u_C1R (src, src_size, src_step, src_roi, dst, dst_step, dst_roi,
                angle * 180.0/PI, img->width/2, img->height/2, IPPI_INTER_LINEAR | IPPI_SMOOTH_EDGE);
    else
        ippiRotateCenter_8u_C3R (src, src_size, src_step, src_roi, dst, dst_step, dst_roi,
                angle * 180.0/PI, img->width/2, img->height/2, IPPI_INTER_LINEAR | IPPI_SMOOTH_EDGE);

    // create the output image
    botlcm_image_t *oimg = botlcm_image_t_create (img->width, img->height, img->row_stride, img->size,
            dst, img->pixelformat, img->utime);

    // free memory
    ippFree (src);
    ippFree (dst);

    return oimg;
}

void image_save_tile_to_file (botlcm_image_t *img, int n, const char *filename)
{
    botlcm_image_t *im = NULL;

    assert (n==4);
    assert (img);
    im = &img[0];

    int greyscale = im->pixelformat == CAM_PIXEL_FORMAT_GRAY;
    int nchannels = greyscale ? 1 : 3;
    
    int width = 2 * im->width;
    int height = 2 * im->height;
    int size = n * im->size;
    unsigned char *data = (unsigned char*)malloc(size);

    // image 1 (top left)
    im = &img[1];
    for (int r=0;r<im->height;r++) {
        for (int c=0;c<im->width;c++) {
            int rr = r;
            int cc = c;
            for (int k=0;k<nchannels;k++)
                data[nchannels * (rr * width + cc) + k] = im->data[nchannels * (r * im->width + c) + k];
        }
    }

    // image 2 (top right)
    im = &img[2];
    for (int r=0;r<im->height;r++) {
        for (int c=0;c<im->width;c++) {
            int rr = r;
            int cc = c + im->width;
            for (int k=0;k<nchannels;k++)
                data[nchannels * (rr * width + cc) + k] = im->data[nchannels * (r * im->width + c) + k];
        }
    }

    // image 0 (bottom left)
    im = &img[0];
    for (int r=0;r<im->height;r++) {
        for (int c=0;c<im->width;c++) {
            int rr = r + im->height;
            int cc = c;
            for (int k=0;k<nchannels;k++)
                data[nchannels * (rr * width + cc) + k] = im->data[nchannels * (r * im->width + c) + k];
        }
    }

    // image 3 (bottom right)
    im = &img[3];
    for (int r=0;r<im->height;r++) {
        for (int c=0;c<im->width;c++) {
            int rr = r + im->height;
            int cc = c + im->width;
            for (int k=0;k<nchannels;k++)
                data[nchannels * (rr * width + cc) + k] = im->data[nchannels * (r * im->width + c) + k];
        }
    }

    if (greyscale)
        write_pgm (data, width, height, filename);
    else
        write_ppm (data, width, height, filename);

    free (data);
}

void image_stitch_image_to_file (GQueue *img, const char *filename)
{
    int nimg = g_queue_get_length (img);

    botlcm_image_t *im = (botlcm_image_t*)g_queue_peek_head (img);
    int imwidth = im->width;
    int imheight = im->height;

    int fullwidth = im->width * nimg;
    int fullheight = im->height;
    int nchannels = im->row_stride / im->width;
    int fullstride = im->row_stride * nimg;

    unsigned char *data = (unsigned char*)malloc(fullwidth*fullheight*nchannels);
   
    for (int p=0;p<nimg;p++) {
        im = (botlcm_image_t*)g_queue_peek_nth (img, p);
        assert (im);

        for (int i=0;i<im->height;i++) {
            for (int j=0;j<im->width;j++) {
                for (int k=0;k<nchannels;k++) {
                    data[nchannels*i*fullstride+nchannels*(p*im->width+j)+k] = im->data[nchannels*i*im->row_stride+nchannels*j+k];
                }
            }
        }
    }

    write_png (data, fullwidth, fullheight, nchannels, filename);

    dbg (DBG_INFO, "saved tiled image to file %s", filename);

    free (data);
}


