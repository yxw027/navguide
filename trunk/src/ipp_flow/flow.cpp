#include "flow.h"

static float
bessi0 (float x)
{
    float ax, ans;
    double y;

    if ((ax=fabs(x)) < 3.75) {
        y = x/3.75;
        y *= y;
        ans = 1.0 + y * (3.5156229 + y * (3.0899424 + y * (1.2067492
                        + y * (0.2659732 + y * (0.360768e-1
                                + y * 0.45813e-2)))));
    }
    else {
        y = 3.75/ax;
        ans = (exp(ax)/sqrt(ax)) * (0.39894228 + y * (0.1328592e-1
                    + y * (0.225319e-2 + y * (-0.157565e-2 + y * (0.916281e-2
                    + y * (-0.2057706e-1 + y * (0.2635537e-1 + y *(-0.1647633e-1
                    + y * 0.392377e-2))))))));
    }
    return ans;
}

#define ACC 40.0
#define BIGNO   1.0e10
#define BIGNI   1.0e-10

void
besseli (float * I, int n, float x)
{
    int j, k;
    float bi, bim, bip, tox;

    I[0] = bessi0(x);
    if (n == 0)
        return;

    if (x == 0.0) {
        for (j = 1; j <= n; j++)
            I[j] = 0.0;
        return;
    }

    tox = 2.0/fabs(x);
    bip = 0.0;
    bi = 1.0;
    for (j = 2*(n + (int) sqrt(ACC*n)); j > 0; j--) {
        bim = bip + j*tox*bi;
        bip = bi;
        bi = bim;
        if (fabs (bi) > BIGNO) {
            for (k = j+1; k <= n; k++)
                I[k] *= BIGNI;
            bi *= BIGNI;
            bip *= BIGNI;
        }
        if (j <= n)
            I[j] = bip;
    }
    for (j = 1; j <= n; j++) {
        I[j] *= (x < 0.0 && (n & 1) ? -1 : 1) * I[0] / bi;
    }
}

void
IppInitEmtpyOpticalFlow ( UnitFlow *uc )
{
    IppiSize roi = { 0, 0};

    uc->width = 0;
    uc->height = 0;
    uc->roi = roi;
    uc->winSize = 0;
    uc->maxlevel = 0;

    uc->prev_pyr = NULL;
    uc->next_pyr = NULL;

    uc->error_thresh = 0;

    uc->numpts = 0;

    uc->prevpts = NULL;
    uc->nextpts = NULL;
    uc->ptstatus = NULL;
    uc->errors = NULL;
}

/* Init structures for optical flow computation 
 * winSize = 10
 * maxLevel = 3
 * numpoints = 280
 */

void
IppInitOpticalFlow ( UnitFlow *uc, int width, int height, IppiSize roi, int winSize, \
                     int maxlevel, int n, float error_thresh)
{
    IppFreeOpticalFlow ( uc );

    uc->width = width;
    uc->height = height;
    uc->roi = roi;
    uc->winSize = winSize;
    uc->maxlevel = maxlevel;
    uc->error_thresh = error_thresh;
 
    uc->numpts = n; //x * ny;

    uc->prevpts = (IppiPoint_32f*)malloc (uc->numpts * sizeof (IppiPoint_32f));
    uc->nextpts = (IppiPoint_32f*)malloc (uc->numpts * sizeof (IppiPoint_32f));
    uc->ptstatus = (Ipp8s*)malloc (uc->numpts * sizeof (Ipp8s));
    uc->errors = (Ipp32f*)malloc (uc->numpts * sizeof (Ipp32f));

    //uc->eigens = ippiMalloc_32f_C1 (width, height, &uc->eigen_stride);

    return;

    /*
    int c, i, j;

    for (i=0;i<uc->numpts;i++) {
        uc->ptstatus[i] = 0;
        uc->errors[i] = 0;
    }

    c = 0;
    for (i = 0; i < nx; i++) {
        for (j = 0; j <ny; j++) {
            uc->prevpts[c].x = (i + 1) * width / (nx + 1);
            uc->prevpts[c].y = (j + 1) * height / (ny + 1);
            c++;
        }
    }

    memcpy (uc->nextpts, uc->prevpts,
            uc->numpts*sizeof (IppiPoint_32f));
    */
}

void
free_gaussian_pyramid (IppiPyramid * pyr)
{
    if ( !pyr )
        return;

    int i;
    for (i = 1; i <= pyr->level; i++) {
        if ( pyr->pImage[i] ) {
            //printf("freeing image %d / %d\n", i, pyr->level);
            ippiFree (pyr->pImage[i]);
        }
    }
    ippiPyramidFree (pyr);
}

void
IppFreeOpticalFlow ( UnitFlow *uc )
{
    if ( uc == NULL ) {
        return;
    }

    dbg(DBG_INFO,"clearing IPP unit flow\n");

    if (uc->prev_pyr)
        free_gaussian_pyramid (uc->prev_pyr);
    if (uc->next_pyr)
        free_gaussian_pyramid (uc->next_pyr);

    uc->prev_pyr = NULL;
    uc->next_pyr = NULL;

    free (uc->prevpts);
    free (uc->nextpts);
    free (uc->ptstatus);
    free (uc->errors);

    uc->prevpts = NULL;
    uc->nextpts = NULL;
    uc->ptstatus = NULL;
    uc->errors = NULL;

    //ippiFree (uc->eigens);

}


int16_t *
get_discrete_gaussian_kernel (float sigma, int kernel_size)
{
    float kernel[kernel_size];
    int16_t * kernel16;
    int i;
    int koffset;

    koffset = kernel_size / 2;
    //fprintf (stderr, "kernel size %d, sigma %f\n", kernel_size, sigma);
    besseli (kernel + koffset, koffset, sigma*sigma);
    for (i = 0; i <= koffset; i++) {
        kernel[koffset+i] *= exp (-sigma*sigma);
        kernel[koffset-i] = kernel[koffset+i];
        //printf (" %f", kernel[koffset+i]);
    }
    //printf ("\n");
    
    kernel16 = (int16_t *)malloc (kernel_size * sizeof(int16_t));
    for (i = 0; i < kernel_size; i++) {
        kernel16[i] = int(kernel[i] * 0x7fff);
    }

    return kernel16;
}

// rate = 2
// kernel size = 9
// level = 1000
//
IppiPyramid *
generate_gaussian_pyramid (ImgGray *src,
        int level, float rate, int kernel_size, int save)
{
    int width = src->Width();
    int height = src->Height();
    int step = width;

    IppiPyramid * pyr;
    int16_t * kernel;
    IppiSize roi = { width, height};
    IppiPyramidDownState_8u_C1R ** ppState;
    int i;

    ippiPyramidInitAlloc (&pyr, level, roi, rate);

    kernel = get_discrete_gaussian_kernel (sqrt(rate*rate-1), kernel_size);
    ppState = (IppiPyramidDownState_8u_C1R **)&(pyr->pState);
    ippiPyramidLayerDownInitAlloc_8u_C1R (ppState, roi, rate, kernel,
            kernel_size, IPPI_INTER_LINEAR);
    
    pyr->pImage[0] = src->IplImage();
    pyr->pStep[0] = step;

    for (i = 1; i <= pyr->level; i++) {
        pyr->pImage[i] = ippiMalloc_8u_C1 (pyr->pRoi[i].width,
                pyr->pRoi[i].height, pyr->pStep + i);
        //printf("gaussian image level %d: %d x %d (step = %d)\n", i, pyr->pRoi[i].width,
        //     pyr->pRoi[i].height, pyr->pStep[i]);
        
       ippiPyramidLayerDown_8u_C1R (pyr->pImage[i-1], pyr->pStep[i-1],
                pyr->pRoi[i-1], pyr->pImage[i], pyr->pStep[i], pyr->pRoi[i],
                *ppState);
        if ( save) {
            char filename[256];
            sprintf(filename,"gaussian_%d.pgm", i);
            FILE *fp = fopen( filename, "w");
            printf("writing gaussian image to %s\n", filename);
            pgm_write( fp, pyr->pImage[i], pyr->pRoi[i].width,
                       pyr->pRoi[i].height, pyr->pStep[i]);
        
            fclose(fp);
        }
     }

    ippiPyramidLayerDownFree_8u_C1R (*ppState);
    free (kernel);

    return pyr;
}





/* Calculates optical flow for the set of feature points
 * using the pyramidal Lucas-Kanade algorithm.
 *
 * pPyr1  Pointer to the ROI in the first image pyramid structure.
 * pPyr2  Pointer to the ROI in the second image pyramid structure.
 * pPrev  Pointer to the array of initial coordinates of the feature points.
 * pNext  Pointer to the array of new coordinates of feature point; as input it
 *          contains hints for new coordinates.
 * pStatus       Pointer to the array of result indicators.
 * pError       Pointer to the array of differences between neighborhoods of old and
 *         new point positions.
 * numFeat         Number of feature points.
 * winSize       Size of the square search window.
 * maxLev      Pyramid level to start the operation.
 * maxIter      Maximum number of algorithm iterations for each pyramid level (e.g 5)
 * threshold         Threshold value. (e.g 0.01)
 * pState       Pointer to the pyramidal optical flow structure.
 *
 */
void 
IppComputeOpticalFlow( UnitFlow *uc, \
                int maxIter, Ipp32f threshold )
{
    //     use Gaussian and Laplacian pyramids to compute optical flow
    // see IPP doc pp--14-35 for optical flow doc

    IppiOptFlowPyrLK_8u_C1R * pState;
    IppiSize roi = {uc->width,  uc->height};

    memcpy (uc->nextpts, uc->prevpts,
                    uc->numpts*sizeof (IppiPoint_32f));


    // allocate memory for optical flow computation
    ippiOpticalFlowPyrLKInitAlloc_8u_C1R (&pState, roi, uc->winSize,
                                          ippAlgHintAccurate);

    ippiOpticalFlowPyrLK_8u_C1R (uc->prev_pyr, uc->next_pyr,
                                 uc->prevpts, uc->nextpts, uc->ptstatus, uc->errors,
                                 uc->numpts, uc->winSize, uc->maxlevel, maxIter, 0.01, pState);

    ippiOpticalFlowPyrLKFree_8u_C1R (pState);
    
}
