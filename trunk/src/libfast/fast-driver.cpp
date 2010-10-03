#include "fast.h"

navlcm_feature_list_t* compute_fast (byte *im, int width, int height, 
                                       int threshold, double resize)
{
    // compute the fast features
    int num = 0;
    xy *set = fast_corner_detect_12 (im, width, height, threshold, &num);
    
    // filter non max
    int num_max = 0;
    xy *set2 = fast_nonmax (im, width, height, set, num, threshold, &num_max);

    // generate feature set
    navlcm_feature_list_t *list = navlcm_feature_list_t_create (width, height, 0, 0, 49);

    for (int i=0;i<num_max;i++) {
        double fcol = 1.0 * set2[i].x;
        double frow = 1.0 * set2[i].y;
        int icol = math_round(fcol);
        int irow = math_round(frow);
        if (icol < 3 || width - 1 - 3 < icol) continue;
        if (irow < 3 || height - 1 - 3 < irow) continue;
        navlcm_feature_t *ft = navlcm_feature_t_create();
        ft->scale = 1.0;
        ft->ori = 0.0;
        ft->col = fcol;
        ft->row = frow;
        ft->size = 49;
        ft->laplacian = 1;
        ft->data = (float*)malloc(ft->size*sizeof(float));
        int count=0;
        for (int col=0;col<7;col++) {
            for (int row=0;row<7;row++) {
                int r = irow + row - 3;
                int c = icol + col - 3;
                ft->data[count] = 1.0*im[r * width + c]/255.0; 
                count++;
            }
        }
        
        list = navlcm_feature_list_t_append (list, ft);
        navlcm_feature_t_destroy (ft);
    }

    // free 
    free (set);
    free (set2);

    return list;
}

