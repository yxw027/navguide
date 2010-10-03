#ifndef _SIFT_H__
#define _SIFT_H__

/************************************************************************
Demo software: Invariant keypoint matching.
Author: David Lowe

defs.h:
This file contains the headers for a sample program to read images and
  keypoints, then perform simple keypoint matching.
*************************************************************************/

/* From the standard C libaray: */
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <stdlib.h>
#include "common/dbg.h"
#include <dirent.h>
#include <sys/stat.h>
#include <lcmtypes/navlcm_feature_list_t.h>
#include <common/lcm_util.h>

/*------------------------------ Macros  ---------------------------------*/
#ifndef ABS
#define ABS(x)    (((x) > 0) ? (x) : (-(x)))
#define MAX(x,y)  (((x) > (y)) ? (x) : (y))
#define MIN(x,y)  (((x) < (y)) ? (x) : (y))
#endif

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

/*---------------------------- Structures --------------------------------*/

/* Data structure for a float image.
*/
typedef struct SiftImageSt {
    int rows, cols;          /* Dimensions of image. */
    float **pixels;          /* 2D array of image pixels. */
  struct SiftImageSt *next;    /* Pointer to next image in sequence. */
} *SiftImage;


/* Data structure for a keypoint.  Lists of keypoints are linked
   by the "next" field.
*/
typedef struct KeypointSt {
    float row, col;             /* Subpixel location of keypoint. */
    float scale, ori;           /* Scale and orientation (range [-PI,PI]) */
    double *descrip;     /* Vector of descriptor values */
    double *ivec;
    struct KeypointSt *next;    /* Pointer to next keypoint in list. */
    int id;                     /* id for clustering */
    int frameid;                /* frame where the feature was observed */
    int nid;                    /* node id in the tree */
} *Keypoint;


void CopyKeypoint (Keypoint dst, Keypoint src);

/*-------------------------- Function prototypes -------------------------*/
/* These are prototypes for the external functions that are shared
   between files.
*/
extern const int MagFactor;
extern const float GaussTruncate;

/* From util.c */
void FatalError(const char *fmt, ...);
SiftImage CreateImage(int rows, int cols);
void FreeSiftImage( SiftImage img );
SiftImage ReadPGMFile(char *filename);
SiftImage ReadPGM(FILE *fp);
void WritePGM(FILE *fp, SiftImage image);
void DrawLine(SiftImage image, int r1, int c1, int r2, int c2);
Keypoint ReadKeyFile(char *filename);
Keypoint ReadKeys(FILE *fp);
void WriteKeyFile(Keypoint keys, char *filename);
void WriteKeys(Keypoint keys, FILE *fp);

/* Only interface needed to key.c. */
navlcm_feature_list_t* GetKeypoints(SiftImage image, double resize);

/* Following are from util.c */
void *MallocPool(int size, int pool);
void FreeStoragePool(int pool);
float **AllocMatrix(int rows, int cols, int pool);
SiftImage CreateImage(int rows, int cols, int pool);
SiftImage CopyImage(SiftImage image, int pool);
SiftImage DoubleSize(SiftImage image, int pool);
SiftImage HalfImageSize(SiftImage image, int pool);
void SubtractImage(SiftImage im1, SiftImage im2);
void GradOriImages(SiftImage im, SiftImage grad, SiftImage ori);
void GaussianBlur(SiftImage image, float sigma);
void SolveLeastSquares(float *solution, int rows, int cols, float **jacobian,
		       float *errvec, float **sqarray);
void SolveLinearSystem(float *solution, float **sq, int size);
float DotProd(float *v1, float *v2, int len);
void TransformLine(int rows, int cols, KeypointSt k, float row1, float col1, float row2,
		   float col2, int &r1, int &c1, int &r2, int &c2);
void SetSiftParameters( float PeakThreshInitValue, int DoubleImSizeValue, \
                        int ScalesValue, float InitSigmaValue );
double SiftDistSquared(Keypoint k1, Keypoint k2);
void
SiftAverage( Keypoint keys, int id, int nid, double *mean, int len ) ;
void
SiftFreeFeatureRec( Keypoint key ) ;
int
SiftCountFeatures ( Keypoint keys );

// extensions by Olivier Koch
//
void EraseKeypoints (KeypointSt** keys, int* n_keys, int n);
int ReadKeypoints (KeypointSt** keys, int* n_keys, const char *basename, int frameid);
void WriteKeypoints (KeypointSt** keys, int* n_keys, int n, const char *basename, int frameid);
int ReadKeypointsTry (const char *basename, int frameid);
int SqDistKeypoint (Keypoint k1, Keypoint k2);
Keypoint AllocKeypoints (int n);
Keypoint AllocAndCopyKeypoints (Keypoint src, int n);
void EraseKeypoints (Keypoint key, int n);
Keypoint ReadKeypoints (FILE *fp, int *n, int *sensor);
//int ReadSiftFeatures (FILE *fp, int index, Keypoint *keys, int *nkeys, int n);
void SkipComments(FILE *fp);

int WriteKeypoints (navlcm_feature_list_t *data, FILE *fp);
navlcm_feature_list_t *ReadKeypoints (FILE *fp);
navlcm_feature_list_t **ReadKeypoints (FILE *fp, int n);

#endif
