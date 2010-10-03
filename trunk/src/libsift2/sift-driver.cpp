// file:        sift-driver.cpp
// author:      Andrea Vedaldi
// description: SIFT command line utility implementation

// AUTORIGTHS

#include<sift.hpp>

#include<string>
#include<iostream>
#include<iomanip>
#include<fstream>
#include<sstream>
#include<algorithm>

extern "C" {
#include<getopt.h>
#if defined (VL_MAC)
#include<libgen.h>
#else
#include<string.h>
#endif
#include<assert.h>
}

#ifndef ABS
#define ABS(x)    (((x) > 0) ? (x) : (-(x)))
#define MAX(x,y)  (((x) > (y)) ? (x) : (y))
#define MIN(x,y)  (((x) < (y)) ? (x) : (y))
#endif

#include <common/dbg.h>
#include <common/codes.h>
#include <common/lcm_util.h>

using namespace std ;

size_t const not_found = numeric_limits<size_t>::max() - 1 ;

/** @brief Case insensitive character comparison
 **
 ** This predicate returns @c true if @a a and @a b are equal up to
 ** case.
 **
 ** @return predicate value.
 **/
inline
bool ciIsEqual(char a, char b)
{
  return 
    tolower((char unsigned)a) == 
    tolower((char unsigned)b) ;
}

/** @brief Case insensitive extension removal
 **
 ** The function returns @a name with the suffix $a ext removed.  The
 ** suffix is matched case-insensitve.
 **
 ** @return @a name without @a ext.
 **/
string
removeExtension(string name, string ext)
{
  string::iterator pos = 
    find_end(name.begin(),name.end(),ext.begin(),ext.end(),ciIsEqual) ;

  // make sure the occurence is at the end
  if(pos+ext.size() == name.end()) {
    return name.substr(0, pos-name.begin()) ;
  } else {
    return name ;
  }
}


/** @brief Insert descriptor into stream
 **
 ** The function writes a descriptor in ASCII/binary format
 ** and in integer/floating point format into the stream.
 **
 ** @param os output stream.
 ** @param descr_pt descriptor (floating point)
 ** @param binary write binary descriptor?
 ** @param fp write floating point data?
 **/
std::ostream&
insertDescriptor(std::ostream& os,
                 VL::float_t const * descr_pt,
                 bool binary,
                 bool fp )
{
#define RAW_CONST_PT(x) reinterpret_cast<char const*>(x)
#define RAW_PT(x)       reinterpret_cast<char*>(x)

  if( fp ) {

    /* convert to 32 bits floats (single precision) */
    VL::float32_t fdescr_pt [128] ;
    for(int i = 0 ; i < 128 ; ++i)
      fdescr_pt[i] = VL::float32_t( descr_pt[i]) ;

    if( binary ) {
      /* 
         Test for endianess. Recall: big_endian = the most significant
         byte at lower memory address.
      */
      short int const word = 0x0001 ;
      bool little_endian = RAW_CONST_PT(&word)[0] ;
      
      /* 
         We save in big-endian (network) order. So if this machine is
         little endiand do the appropriate conversion.
      */
      if( little_endian ) {
        for(int i = 0 ; i < 128 ; ++i) {
          VL::float32_t tmp = fdescr_pt[ i ] ;        
          char* pt  = RAW_PT(fdescr_pt + i) ;
          char* spt = RAW_PT(&tmp) ;
          pt[0] = spt[3] ;
          pt[1] = spt[2] ;
          pt[2] = spt[1] ;
          pt[3] = spt[0] ;
        }
      }            
      os.write( RAW_PT(fdescr_pt), 128 * sizeof(VL::float32_t) ) ;

    } else {

      for(int i = 0 ; i < 128 ; ++i) 
        os << ' ' 
           << fdescr_pt[i] ;
    }

  } else {

    VL::uint8_t idescr_pt [128] ;

    for(int i = 0 ; i < 128 ; ++i)
      idescr_pt[i] = uint8_t(float_t(512) * descr_pt[i]) ;
    
    if( binary ) {

      os.write( RAW_PT(idescr_pt), 128) ;	

    } else { 
      
      for(int i = 0 ; i < 128 ; ++i) 
        os << ' ' 
           << uint32_t( idescr_pt[i] ) ;
    }
  }
  return os ;
}

/* keypoint list */
typedef vector<pair<VL::Sift::Keypoint,VL::float_t> > Keypoints ;

/* predicate used to order keypoints by increasing scale */
bool cmpKeypoints (Keypoints::value_type const&a,
		   Keypoints::value_type const&b) {
  return a.first.sigma < b.first.sigma ;
}

// -------------------------------------------------------------------
//                                                             library
// -------------------------------------------------------------------
navlcm_feature_list_t*
get_keypoints_2 (float *data, int width, int height,
                 int levels, double sigma_init, 
                 double peak_thresh, int doubleimsize, double resize)
{
  int    first          = doubleimsize ? -1 : 0 ; // set this to -1 if you want to upsample the image
  int    octaves        = -1 ;
  //  int    levels         = 3 ;
  float  threshold      = peak_thresh / levels / 2;
  float  edgeThreshold  = 10.0f;
  float  magnif         = 3.0 ;
  int    noorient       = 0 ;
  int    haveKeypoints  = 0 ;
  int    unnormalized   = 0 ;
  string outputFilenamePrefix ;
  string outputFilename ;
  string descriptorsFilename ;
  string keypointsFilename ;

  // -----------------------------------------------------------------
  //                                            Loop over input images
  // -----------------------------------------------------------------      
  VL::PgmBuffer buffer ;
  
  // set buffer data
  buffer.width  = width ;
  buffer.height = height ;
  buffer.data   = data ;

  // ---------------------------------------------------------------
  //                                            Gaussian scale space
  // ---------------------------------------------------------------    
  int         O      = octaves ;    
  int const   S      = levels ;
  int const   omin   = first ;
  float const sigman = .5 ;
  float const sigma0 = sigma_init * powf(2.0f, 1.0f / S) ;
      
  // optionally autoselect the number number of octaves
  // we downsample up to 8x8 patches
  printf ("[sift2] # octaves: %d  #levels: %d\n", O, S);
  if(O < 1) {
      O = std::max
	  (int
	   (std::floor
	    (log2
	     (std::min(buffer.width,buffer.height))) - omin -3), 1) ;
  }

  printf ("[sift2] # octaves: %d  #levels: %d\n", O, S);

  // initialize scalespace
  VL::Sift sift(buffer.data, buffer.width, buffer.height, 
                sigman, sigma0,
                O, S,
                omin, -1, S+1) ;

  //dbg (DBG_INFO, "siftpp: Gaussian scale space completed");

      
  // -------------------------------------------------------------
  //                                             Run SIFT detector
  // -------------------------------------------------------------    
  if( ! haveKeypoints ) {

      //      dbg (DBG_INFO,   "siftpp: running detector  "
      //"threshold             : "
      //    "edge-threshold        : "
      //    );
      
      sift.detectKeypoints(threshold, edgeThreshold) ;

      cout
          << "siftpp: detector completed with "
          << sift.keypointsEnd() - sift.keypointsBegin()
          << " keypoints"
          << endl ;

  }

  // -------------------------------------------------------------
  //                  Run SIFT orientation detector and descriptor
  // -------------------------------------------------------------    

  /* set descriptor options */
  sift.setNormalizeDescriptor( ! unnormalized ) ;
  sift.setMagnification( magnif ) ;

  /*
     if( ! noorient &   nodescr) dbg (DBG_INFO, "[sift2] computing keypoint orientations") ;
     if(   noorient & ! nodescr) dbg (DBG_INFO, "[sift2] computing keypoint descriptors" );
     if( ! noorient & ! nodescr) dbg (DBG_INFO, "[sift2] computing orientations and descriptors" );
     if(   noorient &   nodescr) dbg (DBG_INFO, "[sift2] finalizing") ; 
     */

  // -------------------------------------------------------------
  //            Run detector, compute orientations and descriptors
  // -------------------------------------------------------------

  navlcm_feature_list_t *set = 
      navlcm_feature_list_t_create (width, height, 0, 0, 128);

  for( VL::Sift::KeypointsConstIter iter = sift.keypointsBegin() ;
          iter != sift.keypointsEnd() ; ++iter ) {

      // detect orientations
      VL::float_t angles [4] ;
      int nangles ;
      if( ! noorient ) {
          nangles = sift.computeKeypointOrientations(angles, *iter) ;
      } else {
          nangles = 1;
          angles[0] = VL::float_t(0) ;
      }

      if (nangles > 0) {
          for (int a=0;a<nangles;a++) {
              navlcm_feature_t *ft = navlcm_feature_t_create();
              ft->col = iter->x;
              ft->row = iter->y;
              ft->scale = iter->sigma;
              ft->size = 128;
              ft->laplacian = 1;
              ft->data = (float*)malloc(ft->size*sizeof(float));
              VL::float_t desc[128];
              sift.computeKeypointDescriptor(desc, *iter, angles[a]) ;
              // Convert float vector to integer. Assume largest value in normalized
              // vector is likely to be less than 0.5.
              for (int i=0;i<128;i++) {
                  ft->data[i] = desc[i];//MIN (255, MAX (0, (int)(desc[i]*512.0)));
              }
              ft->ori = angles[a];
              navlcm_feature_list_t_append (set, ft);
              navlcm_feature_t_destroy (ft);
          }
      }
  } // next keypoint

  return set;
}
