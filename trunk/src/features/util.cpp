/*
 * This module computes various types of image features (sift, fast, mser, etc.) and relies
 * on the corresponding libraries (libsift2, libfast, libmser, etc.)
 *
 */

#include "util.h"

navlcm_feature_list_t * frame_compute_surf (unsigned char *data, int width, int height, int sensorid, int64_t utime, navlcm_features_param_t *param, int extended)
{
    GTimer *timer = g_timer_new ();

    dbg (DBG_FEATURES, "surf params: octaves: %d intervals: %d init sample: %d threshold: %f extended: %d", param->surf_octaves, param->surf_intervals, param->surf_init_sample, param->surf_thresh, extended);

    navlcm_feature_list_t *features = surf_detect (data, width, height, sensorid, utime, param->surf_octaves, param->surf_intervals, param->surf_init_sample, param->surf_thresh, extended);

    features->feature_type = extended ? NAVLCM_FEATURES_PARAM_T_SURF128 : NAVLCM_FEATURES_PARAM_T_SURF64;

    if (features) 
        dbg (DBG_FEATURES, "%d surf features computed in %.3f secs.", features->num, g_timer_elapsed (timer, NULL));

    g_timer_destroy (timer);

    return features;
}

int frame_compute_mser (unsigned char *data, int width, int height, int sensorid, int64_t utime, lcm_t *lcm)
{  
    /*
    GTimer *timer = g_timer_new ();

    dbg (DBG_FEATURES, "[sift] downsizing image...");

    // downsize the image
    float ratio = .5;
    int swidth = math_round(ratio*width);
    int sheight = math_round (ratio*height);
    Ipp8u *sdata = (Ipp8u*)ippMalloc (3*swidth*sheight);
    IppiRect rect = {0, 0, width, height};
    double xfactor = 1.0*swidth/width;
    double yfactor = 1.0*sheight/height;
    IppiSize srcroi = {width, height};
    IppiSize dstroi = {swidth, sheight};
    ippiResize_8u_C3R (data, srcroi, 3*width, rect, 
            sdata, 3*swidth, dstroi, xfactor, yfactor, IPPI_INTER_NN);


    // convert to 1-channel HSV
    Ipp8u* src = (Ipp8u*)ippMalloc(swidth*sheight);
    dbg (DBG_FEATURES, "[sift] converting to 1-channel HSV...");
    double* hsv_sdata = image_rgb_to_gray_hsv (sdata, src, swidth*sheight);

    // compute MSER
    dbg (DBG_FEATURES, "[sift] computing MSER...");
    int delta = 5;
    navlcm_blob_list_t out = mser_compute (src, swidth, sheight, delta);

    dbg (DBG_FEATURES, "[sift] found %d mser.", out.num);

    out.sensorid = sensorid;

    // compute HSV signature for each blob
    // and scale back
    //
    dbg (DBG_FEATURES, "[sift] processing blobs...");
    for (int i=0;i<out.num;i++) {

        out.el[i].sensorid = sensorid;

        image_compute_blob_hsv (out.el + i, hsv_sdata, swidth);

        for (int j=0;j<out.el[i].num;j++) {
            out.el[i].cols[j] = MIN( width-1, MAX (0, math_round (out.el[i].cols[j] / ratio)));
            out.el[i].rows[j] = MIN( height-1, MAX (0, math_round (out.el[i].rows[j] / ratio)));
        }
        out.el[i].col = MIN( width-1, MAX (0, math_round (out.el[i].col / ratio)));
        out.el[i].row = MIN( height-1, MAX (0, math_round (out.el[i].row / ratio)));
        out.el[i].lenx /= ratio;
        out.el[i].leny /= ratio;
    }

    // publish
    if (lcm)
        navlcm_blob_list_t_publish (lcm, "BLOB_LIST", &out);

    // free
    for (int i=0;i<out.num;i++) {
        free (out.el[i].cols);
        free (out.el[i].rows);
    }

    if (out.el)
        free (out.el);

    ippFree (src);
    ippFree (sdata);
    free (hsv_sdata);

    // timer
    gulong usecs;
    dbg (DBG_FEATURES, "[sift] mser computation time: %.2f sec.", g_timer_elapsed (timer, &usecs));
    g_timer_destroy (timer);
*/

    return 0;
}

// Implementation by Rosten
//
// http://mi.eng.cam.ac.uk/~er258/work/fast.html
// 
navlcm_feature_list_t* frame_compute_fast (unsigned char *data, int width, int height, int sensorid, int64_t utime, navlcm_features_param_t *param)
{
    GTimer *timer = g_timer_new ();

    int threshold = param->fast_thresh;

    navlcm_feature_list_t *out = compute_fast (data, width, height, threshold, param->scale_factor);

    out->utime = utime;
    out->sensorid = sensorid;
    out->width = width;
    out->height = height;
    out->feature_type = NAVLCM_FEATURES_PARAM_T_FAST;

    for (int i=0;i<out->num;i++) {
        out->el[i].sensorid = sensorid;
        out->el[i].index = i;
        out->el[i].laplacian = 0;
    }

    // print out timer
    gulong usecs;
    dbg (DBG_FEATURES, "[fast] computed %d features in %.4f sec. [sensor %d]"
            "on %d x %d image", out->num, g_timer_elapsed (timer, &usecs),
            sensorid, width, height);
    g_timer_destroy (timer);

    return out;
}

// Good features to track, using OpenCV
//
navlcm_feature_list_t* frame_compute_gftt (unsigned char *data, int width, int height, int sensorid, int64_t utime, navlcm_features_param_t *param)
{
    GTimer *timer = g_timer_new ();

    // create an opencv image
    IplImage *img = cvCreateImage ( cvSize (width, height), IPL_DEPTH_8U, 1);
    IplImage *eig = cvCreateImage ( cvSize (width, height), IPL_DEPTH_32F, 1);
    IplImage *tmp = cvCreateImage ( cvSize (width, height), IPL_DEPTH_32F, 1);

    // set the pixels
    memcpy (img->imageData, data, width * height);

    // compute good features to track 
    double quality_level = param->gftt_quality;
    double min_distance = param->gftt_min_dist;
    int block_size = param->gftt_block_size;
    int use_harris = param->gftt_use_harris;
    double harris_param = param->gftt_harris_param; // used only if use_harris is 1
    int max_count=300;
    int patch_size=7;
    assert (patch_size % 2 == 1);

    int count=max_count;
    CvPoint2D32f* corners = (CvPoint2D32f*)malloc(max_count*sizeof(CvPoint2D32f));

    cvGoodFeaturesToTrack (img, eig, tmp, &corners[0], &count, quality_level, min_distance,
            NULL, block_size, use_harris, harris_param);


    // subpixel accuracy
    cvFindCornerSubPix (img, &corners[0], count, cvSize(10,10), cvSize(-1,-1),
            cvTermCriteria (CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 20, 0.03));

    // convert corners to features
    navlcm_feature_list_t *features = navlcm_feature_list_t_create (width, height, 
            sensorid, utime, FEATURES_DIST_MODE_NCC);

    features->utime = utime;
    features->sensorid = sensorid;
    features->width = width;
    features->height = height;
    features->feature_type = NAVLCM_FEATURES_PARAM_T_GFTT;

    for (int i=0;i<count;i++) {
        navlcm_feature_t *ft = navlcm_feature_t_create ();
        CvPoint2D32f *corner = corners + i;
        ft->col = corner->x / param->scale_factor;
        ft->row = corner->y / param->scale_factor;
        ft->scale = 1.0*patch_size/2.0;
        ft->size = patch_size * patch_size;
        ft->laplacian = 0;
        ft->data = (float*)malloc(ft->size*sizeof(float));
        int pixcount=0;
        int icol = math_round(ft->col);
        int irow = math_round(ft->row);
        if (icol < patch_size/2 || width - 1 - patch_size/2 < icol) continue;
        if (irow < patch_size/2 || height - 1 - patch_size/2 < irow) continue;

        for (int col=0;col<patch_size;col++) {
            for (int row=0;row<patch_size;row++) {
                int r = irow + row - patch_size/2;
                int c = icol + col - patch_size/2;
                ft->data[pixcount] = data[r * width + c]; 
                pixcount++;
            }
        }

        features = navlcm_feature_list_t_append (features, ft);
        navlcm_feature_t_destroy (ft);
    }

    // free image
    cvReleaseImage (&img);
    cvReleaseImage (&eig);
    cvReleaseImage (&tmp);
    free (corners);

    gulong usecs;
    double secs = g_timer_elapsed (timer, &usecs);
    g_timer_destroy (timer);
    dbg (DBG_FEATURES, "found %d corners in %.3f secs.", count, secs); 

    // return features
    return features;
}

// Implementation by Andrea Vedaldi
//
// http://vision.ucla.edu/~vedaldi/code/siftpp/siftpp.html
//
navlcm_feature_list_t* frame_compute_sift_2 (float *data, int width, int height, int sensorid, int64_t utime, navlcm_features_param_t *param)
{
    GTimer *timer = g_timer_new ();

    // compute SIFT features
    navlcm_feature_list_t *out = get_keypoints_2 (data, width, height,
            param->sift_nscales, 
            param->sift_sigma,
            param->sift_peakthresh, 
            param->sift_doubleimsize,
            param->scale_factor);

    out->utime = utime;
    out->sensorid = sensorid;
    out->width = width;
    out->height = height;
    out->feature_type = NAVLCM_FEATURES_PARAM_T_SIFT2;

    for (int i=0;i<out->num;i++) {
        out->el[i].sensorid = sensorid;
        out->el[i].index = i;
        out->el[i].laplacian = 0;
    }

    // print out timer
    gulong usecs;
    dbg (DBG_FEATURES, "[sift] computed %d features in %.4f sec. [sensor %d]"
            "on %d x %d image mode: %s", out->num, g_timer_elapsed (timer, &usecs),
            sensorid, width, height, "sift++");
    g_timer_destroy (timer);

    return out;
}

// Implementation by D. Lowe
//
// Demo release
//
static SiftImage g_sift_img = NULL;
static int g_sift_width = -1;
static int g_sift_height = -1;

navlcm_feature_list_t* frame_compute_sift ( float *data, int width, int height, int sensorid, int64_t utime, navlcm_features_param_t *param)
{  
    GTimer *timer = g_timer_new ();

    // create a sift image
    if (g_sift_width != width || g_sift_height != height) {
        if (g_sift_img)
            FreeSiftImage (g_sift_img);
        g_sift_img = CreateImage (height, width);
        g_sift_width = width;
        g_sift_height = height;
    }

    memcpy (g_sift_img->pixels[0], data, width*height*sizeof(float));

    // setup the SIFT machinery
    SetSiftParameters( param->sift_peakthresh, param->sift_doubleimsize, param->sift_nscales, param->sift_sigma);

    dbg (DBG_FEATURES, "sift params: thresh: %.3f doubleimsize: %d nscales: %d sigma: %.3f resize: %.3f", param->sift_peakthresh, param->sift_doubleimsize, param->sift_nscales, param->sift_sigma, param->scale_factor);

    // compute the SIFT features
    navlcm_feature_list_t *out = GetKeypoints (g_sift_img, param->scale_factor);

    out->utime = utime;
    out->sensorid = sensorid;
    out->width = width;
    out->height = height;
    out->feature_type = NAVLCM_FEATURES_PARAM_T_SIFT;

    for (int i=0;i<out->num;i++) {
        out->el[i].sensorid = sensorid;
        out->el[i].index = i;
        out->el[i].laplacian = 0;
    }

    // print out timer
    gulong usecs;
    dbg (DBG_FEATURES, "[sift] computed %d features in %.4f sec [sensor %d]. "
            "on %d x %d image, mode: %s", out->num, g_timer_elapsed (timer, &usecs),
            sensorid, width, height, "lowe");
    g_timer_destroy (timer);

    return out;
}

void compare_with_lowe_features (navlcm_feature_list_t *features, float *src, int width, int height, int sensorid, int64_t utime, navlcm_features_param_t *param)
{
    // compute features by Lowe
    navlcm_feature_list_t *lowes = 
        frame_compute_sift (src, width, height, sensorid, utime, param);

    // compare the two sets
    dbg (DBG_INFO, "***************************************************");
    dbg (DBG_INFO, "Lowe's set: %d features      Sift++: %d features.",
            lowes->num, features->num);

    // for each feature, find the closest feature and compare descriptors
    for (int i=0;i<features->num;i++) {
        navlcm_feature_t *f1 = features->el + i;

        int best_j=-1; double best_dist=0.0;

        for (int j=0;j<lowes->num;j++) {

            navlcm_feature_t *f2 = lowes->el + j;

            double dist = vect_sqdist_float (f1->data, f2->data, f1->size);

            if (best_j == -1 || dist < best_dist) {
                best_dist = dist;
                best_j = j;
            }
        }

        assert (best_j!=-1);

        navlcm_feature_t *f3 = lowes->el + best_j;

        double gdist = (f3->col - f1->col ) * (f3->col - f1->col) + 
            (f3->row - f1->row) * (f3->row - f1->row);
        gdist = sqrt (gdist);
        double fdist = vect_sqdist_float (f3->data, f1->data, f1->size);
        fdist = sqrt(fdist/f1->size);

        dbg (DBG_INFO, "*********** feature %d ***************", i);
        dbg (DBG_INFO, "geom dist: %.3f      feature dist: %.3f", gdist, fdist);
        for (int i=0;i<20;i++)
            printf ("[%.3f] ", f1->data[i]);
        printf ("\n\n");
        for (int i=0;i<20;i++)
            printf ("[%.3f] ", f3->data[i]);
        printf ("\n");

    }

    // destroy
    navlcm_feature_list_t_destroy (lowes);
}

/* two features are similar if they have the same position (col, row) and the same scale
*/
gboolean features_similar (navlcm_feature_t *f1, navlcm_feature_t *f2)
{
    assert (f1 && f2);
    if (fabs(f1->col - f2->col) > 1E-6) return FALSE;
    if (fabs(f1->row - f2->row) > 1E-6) return FALSE;
    if (fabs(f1->scale - f2->scale) > 1E-6) return FALSE;
    return TRUE;
}

/* two features are consensoridered as duplicates if their position (col, row) is the same
 * and if their scale is the same; this function is here to prevent duplicates having
 * different orientation angles.
 */
navlcm_feature_list_t *remove_duplicate_features (navlcm_feature_list_t *features)
{
    if (!features) return NULL;

    navlcm_feature_list_t *fs = navlcm_feature_list_t_create (features->width, features->height, 
            features->sensorid, features->utime, features->desc_size);

    for (int i=0;i<features->num;i++) {
        // fortunately, duplicates are consecutive so we don't have to search through the entire list
        if (i > 0 && features_similar (features->el + i, features->el + i-1))
            continue;
        fs = navlcm_feature_list_t_append (fs, features->el + i);
    }

    navlcm_feature_list_t_destroy (features);

    return fs;
}

navlcm_feature_list_t * features_driver (botlcm_image_t *img, int sensorid, navlcm_features_param_t *param, gboolean save_to_file)
{
    if (!img) return NULL;

    GTimer *timer = g_timer_new ();

    navlcm_feature_list_t *features = NULL;
    gboolean resize = param->scale_factor < .99;
    int width = img->width;
    int height = img->height;
    int nchannels = img->pixelformat == CAM_PIXEL_FORMAT_GRAY ? 1 : 3;

    assert (width > 0 && height > 0);

    int swidth = (int)(width * param->scale_factor);
    int sheight = (int)(height * param->scale_factor);

    assert (swidth > 0 && sheight > 0);

    IppiSize base_roi = {width,  height};
    IppiRect src_roi = { 0, 0, width, height};
    IppiSize dst_roi = { swidth, sheight };
    double ratio_x = (double)swidth / width;
    double ratio_y = (double)sheight / height;

    // decompress from JPEG if needed
    //
    unsigned char *tmp = NULL;
    gboolean decompress = img->size < nchannels * img->width * img->height;
    if (decompress) {
        int tmpwidth, tmpheight, tmpchannels;
        tmp = jpeg_decompress (img->data, img->size, 
                &tmpwidth, &tmpheight, &tmpchannels);
        assert (tmpwidth == width && tmpheight == height);
        assert (tmpchannels == nchannels);
    } else {
        tmp = img->data;
    }

    assert (tmp);


    // convert to grayscale
    //
    Ipp8u *src = NULL;
    gboolean greyscale = nchannels == 1;

    if (!greyscale) {
        src = (Ipp8u*)ippMalloc (width*height);
        ippiRGBToGray_8u_C3C1R (tmp, 3*width, src, width, base_roi);
    } else {
        src = tmp;
    }

    // resize
    Ipp8u *src2 = NULL;
    if (resize) {
        src2 = (Ipp8u*)ippMalloc (swidth*sheight);
        ippiResize_8u_C1R ( src, base_roi, width, src_roi, src2,
                swidth, dst_roi, ratio_x, ratio_y, IPPI_INTER_NN);
    } else {
        src2 = src;
    }

    /* sift features */
    if (param->feature_type == NAVLCM_FEATURES_PARAM_T_SIFT) {

        // scale the image from [0,255] to [0.0,1.0]
        // (that's a requirement of the SIFT library)
        //
        float *src3 = (float*)malloc (swidth*sheight*sizeof(float));

        ippiScale_8u32f_C1R (src2, swidth, src3,
                swidth * sizeof(float), dst_roi , 0.0, 1.0);

        features = frame_compute_sift ( src3, swidth, sheight, sensorid, img->utime, param );

        free (src3);
    }

    if (param->feature_type == NAVLCM_FEATURES_PARAM_T_SIFT2) {

        // scale the image from [0,255] to [0.0,1.0]
        // (that's a requirement of the SIFT library)
        //
        float *src3 = (float*)malloc (swidth*sheight*sizeof(float));

        ippiScale_8u32f_C1R (src2, swidth, src3,
                swidth * sizeof(float), dst_roi , 0.0, 1.0);

        features = frame_compute_sift_2 ( src3, swidth, sheight, sensorid, img->utime, param );

        free (src3);
    }

    /* fast features */
    if (param->feature_type == NAVLCM_FEATURES_PARAM_T_FAST) {
        features = frame_compute_fast (src2, swidth, sheight, sensorid, img->utime, param );
    }

    /* gftt features */
    if (param->feature_type == NAVLCM_FEATURES_PARAM_T_GFTT) {
        features = frame_compute_gftt (src2, swidth, sheight, sensorid, img->utime, param);
    }

    /* surf 64 features */
    if (param->feature_type == NAVLCM_FEATURES_PARAM_T_SURF64) {
        features = frame_compute_surf (src2, swidth, sheight, sensorid, img->utime, param, 0);
    }

    /* surf 128 features */
    if (param->feature_type == NAVLCM_FEATURES_PARAM_T_SURF128) {
        features = frame_compute_surf (src2, swidth, sheight, sensorid, img->utime, param, 1);
    }

    if (!features)
        dbg (DBG_ERROR, "Error: unrecognized feature type %d", param->feature_type);

    // set feature parameters
    if (features) {
        for (int i=0;i<features->num;i++) {
            features->el[i].utime = img->utime;
            features->el[i].index = i;
            features->el[i].sensorid = sensorid;
            features->el[i].uid = 0;
        }
    }

    dbg (DBG_FEATURES, "computation rate: %.2f Hz", 1.0/g_timer_elapsed (timer, NULL));
    g_timer_destroy (timer);

    // remove duplicate features
    // features = remove_duplicate_features (features);

    // save to pgm if needed
    if (features && save_to_file) {
        char fname_color[256];
        char fname_input[256];
        sprintf (fname_input, "image-%d-%ld.pgm", sensorid, img->utime);
        sprintf (fname_color, "image-%d-%ld-color.ppm", sensorid, img->utime);
        // write features on image
        Ipp8u *colorimg = grayscale_to_color (tmp, width, height);
        for (int i=0;i<features->num;i++) {
            int col = (int)(features->el[i].col);
            int row = (int)(features->el[i].row);
            image_draw_point_C3R (colorimg, width, height, col, row, 3, 255, 0, 0);
        }
        write_pgm (tmp, width, height, fname_input);
        write_ppm (colorimg, width, height, fname_color);
        ippFree (colorimg);
    }

    // free
    if (decompress) free (tmp);
    if (!greyscale) ippFree (src);
    if (resize)     ippFree (src2);

    return features;
}

navlcm_feature_list_t *features_driver (botlcm_image_t **img, int nimg, navlcm_features_param_t *param)
{
    if (!img) return NULL;

    navlcm_feature_list_t *features = navlcm_feature_list_t_create ();

    for (int i=0;i<nimg;i++) {

        navlcm_feature_list_t * f = features_driver (img[i], i, param, FALSE);

        if (!f) continue;

        features->width = f->width;
        features->height = f->height;
        features->desc_size = f->desc_size;
        features->feature_type = f->feature_type;

        features = navlcm_feature_list_t_append (features, f);
    }

    // re-index features
    for (int i=0;i<features->num;i++)
        ((navlcm_feature_t*)(features->el+i))->index = i;

    return features;
}

