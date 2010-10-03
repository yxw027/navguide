/* 
 * This is the code for the rotation baseline algorithm. It takes two images as input
 * (presumably from a camera looking upward) and estimates the rotation between the two
 * images using SIFT feature matching. The camera does not need to be calibrated.
 */

#include "rotation.h"

#define ROT_THRESH_MIN .4
#define ROT_THRESH_MAX .99

// input points are in normalized coordinates
//
int rot_estimate_angle_unit (double x1, double y1, double x2, double y2, double threshold, double *angle)
{
    vec2d_t v1, v2;
    v1.x = x1; 
    v1.y = y1;
    v2.x = x2;
    v2.y = y2;

    double dot = v1.x * v2.x + v1.y * v2.y;
    double len1 = sqrt (pow(x1,2)+pow(y1,2));
    double len2 = sqrt (pow(x2,2)+pow(y2,2));
    double costheta = dot / (len1 * len2);
    if (costheta > 1) return 0;
    double theta = acos (costheta);

    if ((v1.x*v2.y - v1.y*v2.x) < 0) 
        theta = -theta;

    /*
    if (len1 < ROT_THRESH_MIN || len2 < ROT_THRESH_MIN)
        return -1;
    
    if (len1 > ROT_THRESH_MAX || len2 > ROT_THRESH_MAX)
        return -1;
    */

    //double a1 = atan2 (y1, x1);
    //double a2 = atan2 (y2, x2);
    //theta = a2-a1;
    if (angle) *angle = clip_value (theta, -PI, PI, 1E-6);

    return 0;
}

double rot_penalty (navlcm_feature_match_set_t *matches, double angle)
{
    double dist = 0.0;

    if (!matches || matches->num == 0) {
        dbg (DBG_ERROR, "no matches.");
        return .0;
    }

    // rotate features and compute SSD
    int count=0;
    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *m = matches->el + i;
        if (m->num==0) continue;
        navlcm_feature_t *src = &m->src;
        navlcm_feature_t *dst = m->dst;
        double srccol = cos(angle)*src->col - sin(angle)*src->row;
        double srcrow = sin(angle)*src->col + cos(angle)*src->row;
        double mag1 = sqrt(pow(src->col, 2)+pow(src->row,2));
        double mag2 = sqrt(pow(dst->col, 2)+pow(dst->row,2));
        if (mag1 < ROT_THRESH_MIN || mag2 < ROT_THRESH_MIN) continue;
        if (mag1 > ROT_THRESH_MAX || mag2 > ROT_THRESH_MAX) continue;
        dist += pow(dst->col - srccol, 2) + pow (dst->row - srcrow, 2);
        count++;
    }

    if (count > 0)
        dist /= count;

//    printf ("%.3f deg. --> %.3f\n", angle*180/PI, dist);

    return dist;
}

int rot_estimate_angle_from_features (navlcm_feature_list_t *f1, navlcm_feature_list_t *f2,
        double *angle_rad, double *stdev,
        navlcm_feature_match_set_t **mmatches)
{
    /*
    double src_cc[2], dst_cc[2];
    double *angles;
    int count=0;
    int nangles;
    */

    if (!f1 || !f2) return -1;

    // match features
    navlcm_feature_match_set_t *matches = find_feature_matches_multi (f1, f2, TRUE, TRUE, 5, 0.80, -1.0, -1.0, FALSE);

    if (!matches) {
        dbg (DBG_ERROR, "no matches found in rot_estimate_angle_from_features");
        return -1;
    } else {
        dbg (DBG_CLASS, "rot_estimate_angle_from_features: found %d matches.", matches->num);
    }


    //navlcm_feature_match_set_t *copy = navlcm_feature_match_set_t_copy (matches);

    /*
    // initialize centroids
    for (int i=0;i<2;i++) { 
        src_cc[i] = dst_cc[i] = 0.0;
    }

    // normalize & undisort the point coordinates, and compute centroids
    for (int i=0;i<(*matches)->num;i++) {
        navlcm_feature_match_t *m = (navlcm_feature_match_t*)(*matches)->el + i;
        if (m->num == 0) continue;
        navlcm_feature_t *src = &m->src;
        navlcm_feature_t *dst = m->dst;
        calib_normalize (&src->col, &src->row, calib_fc, calib_cc);
        calib_normalize (&dst->col, &dst->row, calib_fc, calib_cc);
//        calib_undistort_1st_order (&dst->col, &dst->row, calib_kc[0]);
        calib_undistort (&src->col, &src->row, calib_kc);
        calib_undistort (&dst->col, &dst->row, calib_kc);
        src_cc[0] += src->col;
        src_cc[1] += src->row;
        dst_cc[0] += dst->col;
        dst_cc[1] += dst->row;
        count++;
    }

    if (count > 0) {
        for (int i=0;i<2;i++) { 
            src_cc[i] /= count;
            dst_cc[i] /= count;
        }
    }
        */

    double *siftang=NULL;
    int nsiftang=0;
    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *m = (navlcm_feature_match_t*)matches->el + i;
        if (m->num == 0) continue;
        navlcm_feature_t *src = &m->src;
        navlcm_feature_t *dst = m->dst;
        double x2 = cos(src->ori);
        double y2 = sin(src->ori);
        double x1 = cos(dst->ori);
        double y1 = sin(dst->ori);
        double ang;
        if (rot_estimate_angle_unit (x1, y1, x2, y2, .6, &ang)==0)
            printf ("sift estimate: %.3f deg.\n", ang*180/PI);
        siftang = (double*)realloc(siftang, (nsiftang+1)*sizeof(double));
        siftang[nsiftang] = ang;
        nsiftang++;
    }

    double siftest = vect_mean_sincos (siftang, nsiftang);
    dbg (DBG_ERROR, "sift estimate: %.2f deg.", siftest*180/PI);

    double siftstd = vect_stdev_sincos (siftang, nsiftang, siftest);
    
    dbg (DBG_ERROR, "stdev: %.3f deg.", siftstd*180/PI);

    double *siftangf=NULL;
    int nsiftangf=0;
    for (int i=0;i<nsiftang;i++) {
        if (diff_angle_plus_minus_pi (siftest, siftang[i]) < .5*siftstd) {
            siftangf = (double*)realloc(siftangf, (nsiftangf+1)*sizeof(double));
            siftangf[nsiftangf]=siftang[i];
            nsiftangf++;
            printf ("%.4f deg. -- %.4f\n", siftang[i]*180/PI, diff_angle_plus_minus_pi (siftest, siftang[i])*180/PI);
        }
    }

    double siftestf = vect_mean_sincos (siftangf, nsiftangf);
    dbg (DBG_ERROR, "sift estimate: %.2f deg.", siftestf*180/PI);
    free (siftangf);
    free (siftang);

    *angle_rad = siftestf;

    *mmatches = matches;

    /*
    // subtract centroid to each feature
    for (int i=0;i<(*matches)->num;i++) {
        navlcm_feature_match_t *m = (navlcm_feature_match_t*)(*matches)->el + i;
        if (m->num == 0) continue;
        navlcm_feature_t *src = &m->src;
        navlcm_feature_t *dst = m->dst;
        src->col -= src_cc[0];
        src->row -= src_cc[1];
        dst->col -= dst_cc[0];
        dst->row -= dst_cc[1];
    }

    // proceed to voting. each match votes for a rotation
    angles = NULL;
    nangles = 0;

    for (int i=0;i<(*matches)->num;i++) {
        navlcm_feature_match_t *m = (navlcm_feature_match_t*)(*matches)->el + i;
        if (m->num == 0) continue;
        navlcm_feature_t *src = &m->src;
        navlcm_feature_t *dst = m->dst;
        double angle;
        if (rot_estimate_angle_unit (src->col, src->row, dst->col, dst->row, .1, &angle) == 0) {
            angles = (double*)realloc(angles, (nangles+1)*sizeof(double));
            angles[nangles] = angle;
            nangles++;
        }
    }
    */
/*
    // compute least square average
    ssin = 0.0, scos = 0.0;
    iter = g_queue_peek_head_link (angles);
    while (iter) {
        double *angle = (double*)iter->data;
        ssin += sin (*angle);
        scos += cos (*angle);
        iter = iter->next;
    }
    angle_rad = atan2 (ssin, scos);
    */ 

    /*
    assert (angle_rad && stdev);

    *angle_rad = 0.0;
    double penalty = 0.0;

    // compute penalty around solution
    for (int i=0;i<360;i++) {
        double ap = clip_value(1.0*(i)*PI/180, -PI, PI, 1E-6);
        double s = rot_penalty (*matches, ap);
        if (i==0 || s < penalty) {
            penalty = s;
            *angle_rad = ap;
        }
    }

    // compute standard deviation
    *stdev = vect_stdev_sincos (angles, nangles, *angle_rad);
    */
    /*
    dbg (DBG_CLASS, "mean, stdev: %.3f, %.3f deg. (%d votes)", *angle_rad*180/PI, *stdev*180/PI, nangles);

    dbg (DBG_CLASS, "sincos avg.: %.3f deg.", vect_mean_sincos (angles, nangles)*180/PI);
    */
    /*
    // remove outliers from matches
    navlcm_feature_match_set_t *setunrect = navlcm_feature_match_set_t_create ();
    navlcm_feature_match_set_t *setrect = navlcm_feature_match_set_t_create ();
    free (angles); angles = NULL;
    nangles = 0;
    for (int i=0;i<(*matches)->num;i++) {
        navlcm_feature_match_t *m = (navlcm_feature_match_t*)((*matches)->el + i);
        navlcm_feature_match_t *mc = (navlcm_feature_match_t*)((copy)->el + i);
        if (m->num == 0) continue;
        navlcm_feature_t *src = &m->src;
        navlcm_feature_t *dst = m->dst;
        double angle;
        if (rot_estimate_angle_unit (src->col, src->row, dst->col, dst->row, .1, &angle) == 0) {
            if (diff_angle_plus_minus_pi (angle, *angle_rad)  < (*stdev)) {
                navlcm_feature_match_set_t_insert (setunrect, mc);
                navlcm_feature_match_set_t_insert (setrect, m);
                angles = (double*)realloc(angles, (nangles+1)*sizeof(double));
                angles[nangles]=angle;
                nangles++;
            }
        }
    }

    navlcm_feature_match_set_t_destroy (*matches);
    *matches = setunrect;
    // re-compute penalty around solution
    double angle0=*angle_rad;
    for (int i=0;i<150;i++) {
        double ap = clip_value(angle0+20.0*(i-50)/50*PI/180, -PI, PI, 1E-6);
        double s = rot_penalty (setrect, ap);
        if (s < penalty) {
            penalty = s;
            *angle_rad = ap;
        }
    }
    // compute standard deviation
    *stdev = vect_stdev_sincos (angles, nangles, *angle_rad);
   
    *angle_rad = siftestf;
    dbg (DBG_CLASS, "mean, stdev: %.3f, %.3f deg. (%d votes, after filtering)", *angle_rad*180/PI, *stdev*180/PI, nangles);
    

    // free
//    navlcm_feature_match_set_t_destroy (setrect);
    navlcm_feature_match_set_t_destroy (copy);
    free (angles);
*/
    return 0;
}

void draw_arc (IplImage *img, CvPoint pt1, CvPoint pt2, CvPoint cc, CvScalar color, int thickness, int steps)
{
    double radstart = sqrt (pow(pt1.x-cc.x,2)+pow(pt1.y-cc.y,2));
    double radend = sqrt (pow(pt2.x-cc.x,2)+pow(pt2.y-cc.y,2));

    CvPoint pt0=pt1;

    for (int i=0;i<steps;i++) {
        double r = 1.0-1.0*i/steps;
        CvPoint pt = cvPoint (r*pt1.x+(1-r)*pt2.x, r*pt1.y+(1-r)*pt2.y);
        double rad = r*radstart+(1-r)*radend;
        double len = sqrt(pow(pt.x-cc.x,2)+pow(pt.y-cc.y,2));
        double ux = (pt.x-cc.x)/len;
        double uy = (pt.y-cc.y)/len;
        pt.x = cc.x + rad * ux;
        pt.y = cc.y + rad * uy;
        cvLine (img, pt0, pt, color, thickness);
        pt0 = pt;
    }
}

int rot_estimate_angle_from_images (botlcm_image_t *img1, botlcm_image_t *img2,
        double *angle_rad, double *stdev)
{
    if (!img1 || !img2) return -1;
    GTimer *timer = g_timer_new ();

    // create a sift image for each input image
    SiftImage sift_img1 = NULL, sift_img2 = NULL;
    sift_img1 = CreateImage (img1->height, img1->width);
    sift_img2 = CreateImage (img2->height, img2->width);

    // scale from [0,255] to [0.0,1.0]
    //
    IppiSize roi1 = { img1->width, img1->height };
    IppiSize roi2 = { img2->width, img2->height };

    float *img1f = (float*)malloc (img1->width*img1->height*sizeof(float));
    ippiScale_8u32f_C1R (img1->data, img1->width, img1f,
            img1->width * sizeof(float), roi1 , 0.0, 1.0);
    float *img2f = (float*)malloc (img2->width*img2->height*sizeof(float));
    ippiScale_8u32f_C1R (img2->data, img2->width, img2f,
            img2->width * sizeof(float), roi2 , 0.0, 2.0);

    memcpy (sift_img1->pixels[0], img1f, img1->width*img1->height*sizeof(float));
    memcpy (sift_img2->pixels[0], img2f, img2->width*img2->height*sizeof(float));

    // setup the SIFT machinery
    SetSiftParameters (0.02, 0, 5, 1.6);

    // compute the SIFT features
    navlcm_feature_list_t *f1 = GetKeypoints (sift_img1, 1.0);
    navlcm_feature_list_t *f2 = GetKeypoints (sift_img2, 1.0);

    navlcm_feature_match_set_t *matches = NULL;
    int ok = rot_estimate_angle_from_features (f1, f2, angle_rad, stdev, &matches);

    // save a sift image
#if 0
    IplImage *img1_ref =   cvCreateImage ( cvSize (img1->width, img1->height), IPL_DEPTH_8U, 1);
    memcpy (img1_ref->imageData, img1->image, img1->width * img1->height);
    IplImage *img2_ref =   cvCreateImage ( cvSize (img2->width, img2->height), IPL_DEPTH_8U, 1);
    memcpy (img2_ref->imageData, img2->image, img2->width * img2->height);

    IplImage *img1_rgb = cvCreateImage ( cvSize (img1->width, img1->height), IPL_DEPTH_8U, 3);
    cvCvtColor (img1_ref, img1_rgb, CV_GRAY2BGR);
    IplImage *img2_rgb = cvCreateImage ( cvSize (img2->width, img2->height), IPL_DEPTH_8U, 3);
    cvCvtColor (img2_ref, img2_rgb, CV_GRAY2BGR);
  
    cvSaveImage ("img1.png", img1_rgb);
    cvSaveImage ("img2.png", img2_rgb);

    // draw sift features as yellow dots and feature matches as blue dots
    //
     for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *match = (navlcm_feature_match_t*)matches->el+i;
        if (match->num==0) continue;
        navlcm_feature_t *f1 = &match->src;
        navlcm_feature_t *f2 =  match->dst;
        if (fabs (f1->col) < 1E-6 && fabs (f1->row) < 1E-6) continue;
        draw_arc (img1_rgb, cvPoint(f1->col, f1->row), cvPoint (f2->col, f2->row), cvPoint (calib_cc[0], calib_cc[1]), cvScalar (255, 0, 0), 3, 100);

        cvRectangle (img1_rgb, cvPoint(f1->col-3, f1->row-3), cvPoint (f1->col+3, f1->row+3), cvScalar (0, 255, 255), 4);
        cvRectangle (img2_rgb, cvPoint(f2->col-3, f2->row-3), cvPoint (f2->col+3, f2->row+3), cvScalar (0, 255, 255), 4);
    }

     cvSaveImage ("img1-sift.png", img1_rgb);
     cvSaveImage ("img2-sift.png", img2_rgb);

    // rotate img1 by computed angle
    char cmd[256];
    sprintf (cmd, "convert img1.png -rotate %.3f img1-rot.png", *angle_rad * 180.0/PI);
    system (cmd);
    cvReleaseImage (&img1_ref);
    cvReleaseImage (&img1_rgb);
    cvReleaseImage (&img2_ref);
    cvReleaseImage (&img2_rgb);
#endif
    if (matches)
        navlcm_feature_match_set_t_destroy (matches);

    // free memory
    navlcm_feature_list_t_destroy (f1);
    navlcm_feature_list_t_destroy (f2);
    FreeSiftImage (sift_img1);
    FreeSiftImage (sift_img2);
    free (img1f);
    free (img2f);

    //double secs = g_timer_elapsed (timer, NULL);

    g_timer_destroy (timer);

    return ok;
}

int rot_estimate_angle_from_images (const char *f1, const char *f2)
{
    IplImage *img1 = cvLoadImage (f1, 0);
    IplImage *img2 = cvLoadImage (f2, 0);

    if (!img1 || !img2) {
        fprintf (stderr, "failed to load images.");
        return -1;
    }

   botlcm_image_t *i1 = botlcm_image_t_create (img1->width, img1->height, img1->width, img1->width*img1->height, (const unsigned char*)img1->imageData, CAM_PIXEL_FORMAT_GRAY, 0);
   botlcm_image_t *i2 = botlcm_image_t_create (img2->width, img2->height, img2->width, img2->width*img2->height, (const unsigned char*)img2->imageData, CAM_PIXEL_FORMAT_GRAY, 0);

   double angle_rad, stdev;

   rot_estimate_angle_from_images (i1, i2, &angle_rad, &stdev);

   cvReleaseImage (&img1);
   cvReleaseImage (&img2);
   botlcm_image_t_destroy (i1);
   botlcm_image_t_destroy (i2);

   return 0;

}

