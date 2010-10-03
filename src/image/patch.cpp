#include "patch.h"

Patch2D::Patch2D()
{
    _img = NULL;
    _img_g = NULL;

    _c = _r = 0;
    _pos = NULL;
    _ncc = 0.0;
}

Patch2D::Patch2D ( const Patch2D *p )
{
    int w = p->_img->Width();
    int h = p->_img->Height();

    _img = new ImgColor( p->_img );

    _img_g = new ImgGray( p->_img_g );

    _pos = (int*)malloc(2*w*h*sizeof(int));
    memcpy( _pos, p->_pos, 2*w*h*sizeof(int));

    _c = p->_c;
    _r = p->_r;
    _id = p->_id;
    _ncc = p->_ncc;
}

Patch2D::Patch2D( int id, int c, int r, int w, int h )
{
    // patch size must be odd
    assert ( w % 2 == 1);
    assert ( h % 2 == 1);

    _img = new ImgColor( w, h );

    _img_g = new ImgGray( w, h );
    
    _pos = (int*)malloc(2*w*h*sizeof(int));

    _c = c;
    _r = r;

    _id = id;

    _ncc = 0.0;
    
    // reset the values

    int count = 0;

    for (int c=0;c<w;c++) {
        for (int r=0;r<h;r++) {
            SetPixel(c,r,0,0,0);
            _pos[2*count] = 0;
            _pos[2*count+1] = 0;
            count++;
        }
    }
}

Patch2D::Patch2D( int id, int w, int h )
{
    // patch size must be odd
    assert ( w % 2 == 1);
    assert ( h % 2 == 1);

    _img = new ImgColor( w, h );
    
    _img_g = new ImgGray( w, h );

    _pos = (int*)malloc(2*w*h*sizeof(int));

    _id = id;

    _ncc = 0.0;

    _c = _r = 0;

    // reset the values

    int count = 0;

    for (int c=0;c<w;c++) {
        for (int r=0;r<h;r++) {
            SetPixel(c,r,0,0,0);
            _pos[2*count] = 0;
            _pos[2*count+1] = 0;
            count++;
        }
    }

}

void Patch2D::SetPixel( int c, int r, unsigned char rc, unsigned char gc, unsigned char bc )
{
    assert ( c >= 0 && c < _img->Width() );
    assert ( r >= 0 && r < _img->Height() );

    _img->SetPixel( c, r, rc, gc, bc );

    // set the value for the grayscale image
    float gv = ( (float)rc + (float)gc + (float)bc ) / 3.0;

    _img_g->SetPixel( c, r, (unsigned char)Round(gv) );
}

void Patch2D::GetPixel( int c, int r, unsigned char &rc, unsigned char &gc, unsigned char &bc )
{
    assert ( c >= 0 && c < _img->Width() );
    assert ( r >= 0 && r < _img->Height() );

    rc = _img->GetPixel( c, r, 0);
    gc = _img->GetPixel( c, r, 1);
    bc = _img->GetPixel( c, r, 2);
}

Patch2D::~Patch2D()
{
    // dbg(DBG_INFO, "deleting patch\n");
    delete( _img );
    _img = NULL;

    delete( _img_g );
    _img_g = NULL;

    free(_pos);
    _pos = NULL;

}

/* Update the grayscale image in the patch */
void Patch2D::UpdateGrayscale()
{
    assert( _img_g );
    assert( _img );

    for (int c=0;c<_img->Width();c++) {
        for (int r=0;r<_img->Height();r++) {
            
            unsigned char rc = _img->GetPixel( c, r, 0);
            unsigned char gc = _img->GetPixel( c, r, 1);
            unsigned char bc = _img->GetPixel( c, r, 2);

            float gv = ( (float)rc + (float)gc + (float)bc ) / 3.0;

            _img_g->SetPixel( c, r, (unsigned char)Round(gv) );
        }
    }
}

/* Filters an image using a Gaussian kernel */
void Patch2D::GaussianBlur( int size )
{
    _img->GaussianBlur( size );

    UpdateGrayscale();
}

/* Draw a 2D patch with a given zoom factor 
 * origin is at top left corner */
void
Patch2D::Draw( int x0, int y0, int w, int h, int zoom, int current_id )
{
    int x,y;
    
    for (int i=0;i<_img->Height();i++) {

        y = y0 + zoom * i;

        for (int j=0;j<_img->Width();j++) {

            x = x0 + zoom * j;

            unsigned char cr, cg, cb;
            
            GetPixel( j, i, cr, cg, cb );

            glColor3f((float)cr/255, float(cg)/255, float(cb)/255);
            glBegin(GL_POINTS);
            
            for (int p=0;p<zoom;p++) {
                for (int q=0;q<zoom;q++) {
                    glVertex2f(x+p,y+q);
                }
            }

            glEnd();
        }
    }
    

    // also draw a white bounding box around the patch
    if ( current_id == _id )
        DrawBox( Vec2f(x0-1,y0-1), Vec2f(x0+zoom*_img->Width()+1,y0+zoom*_img->Height()+1), RED, 2);
    else
        DrawBox( Vec2f(x0-1,y0-1), Vec2f(x0+zoom*_img->Width()+1,y0+zoom*_img->Height()+1), YELLOW, 2);

    // draw the frame ID on top of the patch
    displayMsg( x0, y0 - 5, w, h, WHITE, "%d", _id);

    // draw the NCC value
    displayMsg( x0, y0 + +zoom*_img->Height()+15, w, h, WHITE, "NCC: %.4f", _ncc);
}

void Patch2D::SetPixelPos( int count, int c, int r )
{
    _pos[2*count] = c;
    _pos[2*count+1] = r;
}

void Patch2D::GetPixelPos( int count, int &c, int &r )
{
    c = _pos[2*count];
    r = _pos[2*count+1];
}

/* Normalized Cross-Correlation */
void Patch2D::ComputeNCC( Patch2D *p )
{
    int w = Width();
    int h = Height();

    assert( p->Width() == w );
    assert( p->Height() == h );

    // compute the average vectors
    Vec3f avg1 = Average();
    Vec3f avg2 = p->Average();

    // compute denominator
    float d1 = 0.0, d2 = 0.0;
    int c,r;
    for (c=0;c<Width();c++) {
        for (r=0;r<Height();r++) {
            unsigned char rc1, gc1, bc1;
            GetPixel(c, r, rc1, gc1, bc1);
            
            unsigned char rc2, gc2, bc2;
            p-> GetPixel(c, r, rc2, gc2, bc2);

            Vec3f v1 = Vec3f(rc1,gc1,bc1)/255;
            Vec3f v2 = Vec3f(rc2,gc2,bc2)/255;

            d1 += dot(v1-avg1,v1-avg1);
            d2 += dot(v2-avg2,v2-avg2);

        }
    }

    float d = sqrt(d1*d2);

    // compute nominator
    float n = 0.0;

    for (c=0;c<Width();c++) {
        for (r=0;r<Height();r++) {
            unsigned char rc1, gc1, bc1;
            GetPixel(c, r, rc1, gc1, bc1);
            
            unsigned char rc2, gc2, bc2;
            p-> GetPixel(c, r, rc2, gc2, bc2);

            Vec3f v1 = Vec3f(rc1,gc1,bc1)/255;
            Vec3f v2 = Vec3f(rc2,gc2,bc2)/255;
            
            n += dot(v1-avg1, v2-avg2);
       }
    }

    // compute NCC
    _ncc = n / d;
}

Vec3f Patch2D::Average()
{
    Vec3f avg;

    int s = Surface();

    int c,r;
    for (r=0;r<Height();r++) {
        for (c=0;c<Width();c++) {
            unsigned char rc, gc, bc;
            GetPixel(c, r, rc, gc, bc);
            avg[0] += (float)rc/255;
            avg[1] += (float)gc/255;
            avg[2] += (float)bc/255;
        }
    }

    avg /= s;

    return avg;
}

/* Compute the gradient orientation and magnitude */
void Patch2D::ComputeGradient( float *ori, float *mag )
{
    int c,r;
    for (c=0;c<Width();c++) {
        for (r=0;r<Height();r++) {
            
            // process pixels in a central disk only
            if ( c*c + r*r >= Width()*Width() ) {
                mag[r*Width()+c] = 0.0;
                ori[r*Width()+c] = 0.0;                
                continue;
            }

            // compute the gradient
            float xgrad = (float)_img_g->GetPixel(c+1,r) - (float)_img_g->GetPixel( c-1,r );
            float ygrad = (float)_img_g->GetPixel(c,r+1) - (float)_img_g->GetPixel( c,r-1 );
            
            // compute magnitude and orientation
            mag[r*Width()+c] = sqrt(xgrad * xgrad + ygrad * ygrad);
            ori[r*Width()+c] = atan2 (ygrad, xgrad);
        }
    }
}

/* Compute the orientation histogram for the patch 
 * in a similar fashion as Lowe in SIFT */

void Patch2D::AssignOrientations()
{
    assert( Width() == Height() );

    // compute image gradient and orientation at each pixel
    int c,r;
    float *ori = (float*)malloc(Width()*Height()*sizeof(float));
    float *mag = (float*)malloc(Width()*Height()*sizeof(float));

    ComputeGradient( ori, mag );

    // sum the gradient weighted orientation in a histogram
    float hist[PATCH_ORI_BINS];
    for (c=0;c<PATCH_ORI_BINS;c++)
        hist[c] = 0.0;

    float row = (float)Width()/2.0 + .5;
    float col = (float)Height()/2.0 + .5;
    float sigma = 1.0;

    for (r = 0; r < Height(); r++) {
        for (c = 0; c < Width(); c++) {
            
            if ( c*c + r*r >= Width()*Width() )
                continue;

            float distsq = (r-row)*(r-row) + (c-col)*(c-col);
            float gval = mag[r*Width()+c];
            float weight = exp(- distsq / (2.0 * sigma * sigma));

            float angle = ori[r*Width()+c];
            int bin = (int) (PATCH_ORI_BINS * (angle + M_PI + 0.001) / (2.0 * M_PI));
            assert(bin >= 0 && bin <= PATCH_ORI_BINS);
            bin = MIN(bin, PATCH_ORI_BINS - 1);
            hist[bin] += weight * gval;
        }
    }

    // smooth the histogram six times
    int i;
    for (i = 0; i < 6; i++)
     SmoothHistogram(hist, PATCH_ORI_BINS);

    // Find maximum value in histogram
    float maxval = 0.0;
    for (i = 0; i < PATCH_ORI_BINS; i++)
        if (hist[i] > maxval)
         maxval = hist[i];

    // Look for each local peak in histogram.  If value is within
    //  80% of maximum value, then save the orientation value
    int prev,next;
    for (i = 0; i < PATCH_ORI_BINS; i++) {
        prev = (i == 0 ?  - 1 : i - 1);
        next = (i == PATCH_ORI_BINS - 1 ? 0 : i + 1);
        if (hist[i] > hist[prev]  &&  hist[i] > hist[next]  &&
            hist[i] >= .8 * maxval) {
            
            //Use parabolic fit to interpolate peak location from 3 samples.
            // Set angle in range -PI to PI.
            
            float interp = InterpPeak(hist[prev], hist[i], hist[next]);
            float angle = 2.0 * M_PI * (i + 0.5 + interp) / PATCH_ORI_BINS - M_PI;
            assert(angle >= -M_PI  &&  angle <= M_PI);
            
            // store the angle
            //_ori.push_back( angle );
            dbg(DBG_INFO, "Orientation angle: %f\n", angle/MPIOVER180);
        }
    }

    // free memory
    free( ori );
    free( mag );
}

/* Compute the orientation of a patch using the central moments
 *
 */
float Patch2D::Orientation()
{
    return _img_g->Orientation();
}

/* Compute the Sift descriptor for a patch given an orientation (in radians)
 */

void Patch2D::ComputeSiftDescriptor( float angle, unsigned char *vec )
{
    // save image for debug
    //char name[256];
    //sprintf(name,"patch-%f.png", angle);
    //SaveImage( _img_g, name, EXT_PNG);

    // compute image gradient and orientation at each pixel
    float *ori = (float*)malloc(Width()*Height()*sizeof(float));
    float *mag = (float*)malloc(Width()*Height()*sizeof(float));
    
    ComputeGradient( ori, mag );
    
    // compute sample vector
    MakeKeypointSample( vec, angle, mag, ori, Width() );

    // free memory
    free( ori );
    free( mag );
}

/* Write patch to a file */
void Patch2D::Write( FILE *fp )
{
    assert( fp );

    fprintf( fp, "%d %d ", _img->Width(), _img->Height() );

    fprintf( fp, "%d %d %d ", _id, _c, _r );
    
    fprintf( fp, "%f ", _ncc );

    int c, r;
    int count = 0;

    for (c=0;c<_img->Width();c++) {
        for (r=0;r<_img->Height();r++) {

            fprintf( fp, "%d %d ", _pos[count], _pos[count+1] );

            count += 2;
        }
    }
    
    _img->Write( fp );
    
    _img_g->Write( fp );
}

/* Read a patch from a file */
void Patch2D::Read( FILE *fp) 
{
    assert( fp );

    int w, h;

    assert( fscanf( fp, "%d%d", &w, &h) == 2 );

    assert( fscanf( fp, "%d%d%d", &_id, &_c, &_r ) == 3 );

    assert( fscanf( fp, "%f", &_ncc) == 1 );

    int c,r, count = 0;

    if ( !_img )
        _img = new ImgColor( w, h );
    
    if ( !_img_g )
        _img_g = new ImgGray( w, h );

    if ( !_pos ) {
        _pos = (int*)malloc(2*w*h*sizeof(int));
        memset( _pos, 0, 2*w*h*sizeof(int));
    }

    assert( w == _img->Width() );
    assert( w == _img_g->Width() );

    assert( h == _img->Height() );
    assert( h == _img_g->Height() );

    for (c=0;c<w;c++) {
        for (r=0;r<h;r++) {

            int a, b;

            assert( fscanf( fp,  "%d%d", &a, &b ) == 2 );

            _pos[count] = a;
            _pos[count+1] = b;
            
            count += 2;
        }
    }

    _img->Read( fp );

    _img_g->Read( fp );
}

/* Rotate a patch */
void Patch2D::Rotate( float angle )
{
  assert( Width() == Height()) ;

  // rotate the RGB image
  _img->Rotate( angle );

  // update the grayscale image
  UpdateGrayscale();
      
}
                    
/* Compute jet */
void Patch2D::Compute_Jet( int n, float scale, float *vect )
{
  _img_g->Jet_calc( Width()/2, Height()/2, n, scale, vect );
}



/*************** EXTERN METHODS **********************/

void AveragePatches( vector<Patch2D*> patches, Patch2D *patch )
{
    int c,r;
    int n = (int)patches.size();
    int i;

    for (c=0;c<patch->Width();c++) {
        for (r=0;r<patch->Height();r++) {
            
            float arc = 0.0, agc = 0.0, abc = 0.0;

            int count = 0;

            for (i=0;i<n;i++) {
                if ( !patches[i] )
                    continue;

                unsigned char rc, gc, bc;
                patches[i]->GetPixel( c, r, rc, gc, bc );

                arc += (float)rc;
                agc += (float)gc;
                abc += (float)bc;

                count++;
            }

            unsigned char frc = (unsigned char)MIN(255,MAX(0,Round( arc / count)));
            unsigned char fgc = (unsigned char)MIN(255,MAX(0,Round( agc / count)));
            unsigned char fbc = (unsigned char)MIN(255,MAX(0,Round( abc / count)));

            patch->SetPixel( c, r, frc, fgc, fbc);
        }
    }
}

