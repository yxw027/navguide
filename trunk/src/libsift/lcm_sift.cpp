#include "lcm_sift.h"

int
lcm_compute_sift ( unsigned char *data, int width, int height,
                   int nchannels, int sid, int64_t timestamp,
                   double resize, double crop, int doubleimsize,
                   int nscales, double peakthresh, double sigma,
                   SiftImage sift_img, lcm_t *lcm)
{    

    // will receive the grayscale image
    Ipp8u *src = NULL;

    // the original image ROI
    IppiSize base_roi = {width,  height};

    // convert image to grayscale if needed
    if (nchannels == 1) 
        src = data;
    else if (nchannels == 3) {
        src = (Ipp8u*)ippMalloc (width*height);
        ippiRGBToGray_8u_C3C1R (data, 3*width, src, width, base_roi);
    } else {
        dbg (DBG_FEATURES, "Invalid # channels: %d", nchannels);
        return -1;
    }

    // compute features                
    // create a sift image
    //double resize = self->param->resize;//gtk_get_scale_value("hscale_sift_resize");
    //int crop_percent = 0;//self->sift_crop_percent;//(int)gtk_get_scale_value("hscale_sift_crop");

    dbg (DBG_FEATURES, "****************************************");
    dbg (DBG_FEATURES, "resizing factor: %.1f", resize);
    dbg (DBG_FEATURES, "cropping factor: %d %%", (int)crop*100);
    dbg (DBG_FEATURES, "double image size: %d", doubleimsize);
    dbg (DBG_FEATURES, "# scales: %d", nscales);
    dbg (DBG_FEATURES, "peak thresh: %.3f", peakthresh);
    dbg (DBG_FEATURES, "sigma: %.3f", sigma);

    int sift_width = width;
    int sift_height = height;
    int modified = 0;

    if (fabs(resize-1.0) > 1E-6 || fabs (crop) > 1E-6) {
        sift_width = (int)(width * resize);
        sift_height = (int)(height * resize * (1.0 - 2 * crop));
        modified = 1;
    }

    if (sift_img != NULL && (sift_img->rows != sift_height ||
         sift_img->cols != sift_width)) {
        dbg (DBG_FEATURES, "[%d] new sift image", sid);
        dbg (DBG_FEATURES, "[%d != %d] or [%d != %d]", 
             sift_img->rows, sift_height,
             sift_img->cols, sift_width);
        FreeSiftImage (sift_img);
        sift_img = NULL;
    }

    if (sift_img == NULL) {
        dbg (DBG_FEATURES, "[%d] creating new sift image %d x %d", sid, 
             sift_width, sift_height);
        sift_img = CreateImage (sift_height, sift_width);
    }

    // scale sift image by a factor of 255
    int dh = (int)(crop * height);
    IppiRect src_roi = { 0, dh, width, height - 2 * dh};
    IppiSize dst_roi = { sift_width, sift_height };
    double ratio_x = (double)sift_width / width;
    double ratio_y = (double)sift_height / height;

    if (modified) {
        Ipp8u* ipp_roi_img = (Ipp8u*)ippMalloc(sift_width*sift_height);
        
        // first resize the roi of interest in the source image
        ippiResize_8u_C1R ( src + dh * width, base_roi, 
                            width, src_roi, ipp_roi_img,
                            sift_width, dst_roi, ratio_x, ratio_y, IPPI_INTER_NN);
        
        // second scale the image by 255 and write it into the SIFT image
        ippiScale_8u32f_C1R (ipp_roi_img, sift_width, 
                             sift_img->pixels[0], 
                             sift_width * sizeof(float), dst_roi , 0.0, 1.0);

        ippFree (ipp_roi_img);

    } else {
        // only scale image by 255
        ippiScale_8u32f_C1R (src, width, 
                             sift_img->pixels[0], 
                             width * sizeof(float), dst_roi , 0.0, 1.0);
    }

    ippFree (src);
        
    // setup the SIFT machinery
    SetSiftParameters( peakthresh, doubleimsize, nscales, sigma);
            
    // compute the SIFT features
    Keypoint keypoint = GetKeypoints (sift_img);

    int n_keys = 0;
        
    Keypoint k = keypoint;
        
    // count features
    while ( k ) {
        n_keys++;
        k = k->next;
    }
    
    dbg (DBG_FEATURES, "[%d] # features: %d (image size: %d x %d)", sid, n_keys,
        sift_width, sift_height);

    // send features over LCM
    
    dbg (DBG_FEATURES, "****************************************");
    dbg (DBG_FEATURES, "width: %d", width);
    dbg (DBG_FEATURES, "height: %d", height);
         
    navlcm_sift_t sift_data;
    sift_data.num = n_keys;
    sift_data.sensor_id = sid;
    sift_data.timestamp = timestamp;
    sift_data.scale = (double*)malloc(n_keys*sizeof(double));
    sift_data.orientation = (double*)malloc(n_keys*sizeof(double));
    sift_data.col = (double*)malloc(n_keys*sizeof(double));
    sift_data.row = (double*)malloc(n_keys*sizeof(double));
    sift_data.data = (double**)malloc(n_keys*sizeof(double*));
    sift_data.width = width;
    sift_data.height = height;
    k = keypoint;
    for (int i=0;i<n_keys;i++) {
        sift_data.data[i] = (double*)malloc(128*sizeof(double));
        sift_data.scale[i] = k->scale / resize;
        sift_data.orientation[i] = k->ori;
        sift_data.col[i] = k->col / resize;
        sift_data.row[i] = k->row / resize;
        memcpy( sift_data.data[i], k->ivec, 128*sizeof(double));
        k = k->next;
    }

    navlcm_sift_t_publish (lcm, "SIFT", &sift_data);

    free (sift_data.scale);
    free (sift_data.orientation);
    free (sift_data.col);
    free (sift_data.row);
    for (int i=0;i<n_keys;i++) {
        free (sift_data.data[i]);
        sift_data.data[i] = NULL;
    }
    free (sift_data.data);

    // free sift features
    FreeStoragePool( KEY_POOL );
    
    return 0;
}

