#ifndef _SIFT_KEY_H__
#define _SIFT_KEY_H__

/************************************************************************
  Copyright (c) 2004. David G. Lowe, University of British Columbia.
  This software is being made available for research purposes only
  (see file LICENSE for conditions).  This notice must be retained on
  all copies and modified versions of this software.
*************************************************************************/

/* From the standard C libaray: */
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

//#include "math/tool.h"

/*------------------------- Macros and constants  -------------------------*/

/* Following defines TRUE and FALSE if not previously defined. */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* Value of PI, rounded up, so orientations are always in range [0,PI]. */
#ifndef PI
#define PI 3.1415927
#endif

#ifndef ABS
#define ABS(x)    (((x) > 0) ? (x) : (-(x)))
#define MAX(x,y)  (((x) > (y)) ? (x) : (y))
#define MIN(x,y)  (((x) < (y)) ? (x) : (y))
#endif

/* Given the name of a structure, NEW allocates space for it in the
   given pool (see util.c) and returns a pointer to the structure.
*/
#define NEW(s,pool) ((struct s *) MallocPool(sizeof(struct s),pool))

/* Assign a unique number to each pool of storage needed for this application. 
*/
#define PERM_POOL  0     /* Permanent storage that is never released. */
#define IMAGE_POOL 1     /* Data used only for the current image. */
#define KEY_POOL   2     /* Data for set of keypoints. */

/* These constants specify the size of the index vectors that provide
   a descriptor for each keypoint.  The region around the keypoint is
   sampled at OriSize orientations with IndexSize by IndexSize bins.
   VecLength is the length of the resulting index vector.
*/
#define OriSize 8
#define IndexSize 4
#define VecLength (IndexSize * IndexSize * OriSize)

/*------------------------------- Externals -----------------------------*/



/*-------------------------- Function prototypes -------------------------*/
/* The are prototypes for the external functions that are shared
   between files.
*/
void SmoothHistogram(float *hist, int bins);
float InterpPeak(float a, float b, float c);


#endif
