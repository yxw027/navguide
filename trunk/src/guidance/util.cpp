/* Classifier utilities
 */

#include "util.h"

/* Feature matching analysis.
 */
void class_run_matching_analysis (GQueue **images, int nsensors, GQueue *features, int64_t utime1, int64_t utime2, lcm_t *lcm)
{
    int count=0;

    dbg (DBG_INFO, "running match analysis between utime %ld - %ld", utime1, utime2);

    // search features
    navlcm_feature_list_t *f1 = find_feature_list_by_utime (features, utime1);
    navlcm_feature_list_t *f2 = find_feature_list_by_utime (features, utime2);

    /* read features from file*/
    /*
    FILE *fp1 = fopen ("fp1.raw", "rb");
    FILE *fp2 = fopen ("fp2.raw", "rb");
    navlcm_feature_list_t *f1 = (navlcm_feature_list_t*)malloc(sizeof(navlcm_feature_list_t));
    navlcm_feature_list_t *f2 = (navlcm_feature_list_t*)malloc(sizeof(navlcm_feature_list_t));
    navlcm_feature_list_t_read (f1, fp1);
    navlcm_feature_list_t_read (f2, fp2);
    fclose (fp1); fclose (fp2);
    */

    if (!f1 || !f2) {
        dbg (DBG_ERROR, "failed to find features for utime %ld or %ld", utime1, utime2);
        return;
    }

    dbg (DBG_INFO, "matching features...");

    // match features
    navlcm_feature_match_set_t *matches = (navlcm_feature_match_set_t*)malloc(sizeof (navlcm_feature_match_set_t));
    int matching_mode = f1->feature_type == NAVLCM_FEATURES_PARAM_T_FAST ? MATCHING_NCC : MATCHING_DOTPROD;
    find_feature_matches_fast (f1, f2, TRUE, TRUE, TRUE, TRUE, .8, -1, matching_mode, matches);

//    navlcm_feature_match_set_t *matches = find_feature_matches_multi (f1, f2, TRUE, TRUE, 5.0, .6, -1.0, -1.0, TRUE);

    if (!matches->num) {
        dbg (DBG_ERROR, "failed to find matches.");
        return;
    }

    dbg (DBG_INFO, "found %d matches.", matches->num);

    // publish matches for display
    navlcm_feature_match_set_t_publish (lcm, "FEATURE_MATCH", matches);

    return;

    // for each match, draw matches on an image and save the image
    double inside=.0, outside=.0;
    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *match = matches->el + i;
        if (match->num == 0) continue;
        if (match->src.sensorid == match->dst[0].sensorid)
            inside+=1.0;
        else
            outside+=1.0;
    }
    printf ("outside: %.3f %%, inside: %.3f %%\n", 100.0 * outside / (outside+inside), 100.0 * inside / (outside+inside));

    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *match = matches->el + i;
        botlcm_image_t *im1 = find_image_by_utime (images[match->src.sensorid], match->src.utime);
        botlcm_image_t *im2 = find_image_by_utime (images[match->dst[0].sensorid], match->dst[0].utime);
        if (!im1) {
            dbg (DBG_ERROR, "failed to find image for utime %ld", match->src.utime);
        }
        if (!im2) {
            dbg (DBG_ERROR, "failed to find image for utime %ld", match->dst[0].utime);
        }

        assert (im1 && im2);
        // stitch images
        Ipp8u *dimg = (Ipp8u*)ippMalloc(2*im1->width*im1->height);
        for (int col=0;col<im1->width;col++) {
            for (int row=0;row<im1->height;row++) {
                dimg[row*2*im1->width+col] = im1->data[row*im1->width+col];
                dimg[row*2*im1->width+col+im1->width] = im2->data[row*im1->width+col];
            }
        }
        Ipp8u *dimg_rgb = grayscale_to_color (dimg, 2*im1->width, im1->height);
        // draw feature points
        image_draw_point_C3R (dimg_rgb, 2*im1->width, im1->height, match->src.col, match->src.row, 4, 255, 0, 0);
        image_draw_point_C3R (dimg_rgb, 2*im1->width, im1->height, match->dst[0].col+im1->width, match->dst[0].row, 4, 255, 0, 0);
        // find next available image
        char filename[256];
        sprintf (filename, "%ld-%ld-%03d.ppm", im1->utime, im2->utime, i);
        dbg (DBG_INFO, "saving image to %s", filename);
        write_ppm (dimg_rgb, 2*im1->width, im1->height, filename);

        ippFree (dimg_rgb);
        ippFree (dimg);
    }
}

/* publish list of maps
 */
void class_publish_map_list (lcm_t *lcm)
{
    DIR *dir = opendir (".");
    if (!dir) {
        dbg (DBG_INFO, "warning: failed to open dir .");
        return;
    }

    struct dirent *dt;
    
    navlcm_dictionary_t dict;
    dict.num = 0;
    dict.keys = NULL;
    dict.vals = NULL;

    while ((dt = readdir (dir))!=NULL) {
        if (strncmp (g_strreverse (dt->d_name), "setag.",4)!=0) continue;
        g_strreverse (dt->d_name);

        dict.keys = (char**)realloc (dict.keys, (dict.num+1)*sizeof(char*));
        dict.vals = (char**)realloc (dict.vals, (dict.num+1)*sizeof(char*));
        dict.keys[dict.num] = (char*)malloc(10);
        dict.vals[dict.num] = g_strdup (dt->d_name);
        sprintf (dict.keys[dict.num], "%d", dict.num);
        dict.num++;
    }

    closedir (dir);

    navlcm_dictionary_t_publish (lcm, "MAP_LIST", &dict);

    for (int i=0;i<dict.num;i++) {
        free (dict.keys[i]);
        free (dict.vals[i]);
    }
    free (dict.keys);
    free (dict.vals);
}

void save_to_image (GQueue *img, navlcm_feature_list_t *features,  const char *filename)
{
    int nimg = g_queue_get_length (img);
    
    botlcm_image_t *img0 = (botlcm_image_t*)g_queue_peek_head (img);

    int width = img0->width;
    int fullwidth = img0->width * nimg;
    int fullheight = img0->height;

    printf ("CHECK creating image %d x %d\n", fullwidth, fullheight);

    IplImage *ip = cvCreateImage (cvSize (fullwidth, fullheight), 8, 3);

    for (int i=0;i<nimg;i++) {
        botlcm_image_t *im = (botlcm_image_t*)g_queue_peek_nth (img, i);
        IplImage *p = cvCreateImage (cvSize (im->width, im->height), 8, 1);
        IplImage *pcolor = cvCreateImage (cvSize (im->width, im->height), 8, 3);
        memcpy (p->imageData, im->data, im->width * im->height);
        cvCvtColor (p, pcolor, CV_GRAY2RGB);

        cvSetImageROI (ip, cvRect (i*im->width, 0, im->width, im->height));
        cvCopy (pcolor, ip);
        cvReleaseImage (&p);
        cvReleaseImage (&pcolor);
    }

    cvSetImageROI (ip, cvRect (0, 0, fullwidth, fullheight));

    // draw feature points
    for (int i=0;i<features->num;i++) {
        navlcm_feature_t *f = (navlcm_feature_t*)features->el + i;
        cvRectangle (ip, cvPoint (f->col  + f->sensorid * width - 2, f->row - 2), cvPoint (f->col + f->sensorid * width+ 2, f->row + 2),
                cvScalar(255,255,0));
    }

    cvSaveImage (filename, ip);
    cvReleaseImage (&ip);
}

           
void read_mission_file (GQueue *mission, const char *filename)
{
    FILE *fp  = fopen (filename, "r");
    if (!fp)
        return;

    printf ("reading mission:\n");

    while (!feof (fp)) {
        int id;
        if (fscanf (fp, "%d", &id) != 1) 
            break;
        g_queue_push_tail (mission, g_intdup (id));
        printf ("%d\n", id);
    }

    fclose (fp);
}

/* compute duration, distance traveled, etc.
 */
void util_compute_statistics (GQueue *poses)
{
    if (!poses || g_queue_get_length (poses) < 2)
        return;

    int64_t start_utime = 0;
    int64_t end_utime = 0;
    botlcm_pose_t *last_pose_1 = NULL;
    botlcm_pose_t *last_pose_2 = NULL;
    double distance_traveled = .0;
    double max_speed = .0;
    
    FILE *fp = fopen ("statistics.txt", "w");

    for (GList *iter=g_queue_peek_tail_link (poses);iter;iter=iter->prev) {
        
        botlcm_pose_t *p = (botlcm_pose_t*)iter->data;
        
        if (start_utime == 0)
            start_utime = p->utime;
        end_utime = p->utime;

        if (!last_pose_1)
            last_pose_1 = p;
        if (!last_pose_2)
            last_pose_2 = p;

        // detect jump in the log
        double d1 = sqrt (powf (p->pos[1] - last_pose_1->pos[1],2) + powf (p->pos[0] - last_pose_1->pos[0],2));
        if (d1 > 5.0) {
            last_pose_1 = p;
            last_pose_2 = p;
        }

        if (p->utime - last_pose_1->utime > 1000000) {
            d1 = sqrt (powf (p->pos[1] - last_pose_1->pos[1],2) + powf (p->pos[0] - last_pose_1->pos[0],2));
            max_speed = fmax (max_speed, d1 * 1000000/ (p->utime - last_pose_1->utime)); 

            last_pose_1 = p;
        }

        // update distance traveled

        if (p->utime - last_pose_2->utime > 5000000) {

            double d2 = sqrt (powf (p->pos[1] - last_pose_2->pos[1],2) + powf (p->pos[0] - last_pose_2->pos[0],2));
            distance_traveled += d2;
            last_pose_2 = p;
        }

    fprintf (fp, "%ld %.3f %.3f\n", (p->utime - start_utime)/1000000, distance_traveled, max_speed);
    }

    fclose (fp);

    int duration = (end_utime - start_utime)/1000000;

    printf ("duration: %d seconds\n", duration);
    printf ("distance traveled: %.1f m\n", distance_traveled);
    printf ("max speed: %.2f m/s\n", max_speed);
    printf ("average speed: %.2f m/s\n", distance_traveled/duration);
}

GQueue *util_read_poses (const char *filename)
{
    FILE *fp = fopen (filename, "rb");
    if (!fp) {
        fprintf (stderr, "failed to open file %s\n", filename);
        return NULL;
    }

    GQueue *data = g_queue_new ();

    while (!feof (fp)) {
        botlcm_pose_t *p = (botlcm_pose_t*)calloc(1, sizeof(botlcm_pose_t));
        botlcm_pose_t_read (p, fp);

        // data may be corrupted (a sign a corruption is a zero utime). thank you carmen.
        if (p->utime > 0) 
            g_queue_push_tail (data, p);
        else
            botlcm_pose_t_destroy (p);
    }

    dbg (DBG_CLASS, "read %d poses from file %s", g_queue_get_length (data), filename);

    fclose (fp);

    return data;

}

GQueue *util_read_ground_truth (const char *filename)
{
    FILE *fp = fopen (filename, "rb");
    if (!fp) {
        fprintf (stderr, "failed to open ground-truth file %s\n", filename);
        return NULL;
    }

    GQueue *data = g_queue_new ();

    while (!feof (fp)) {
        int64_t utime;
        int nodeid;

        fread (&utime, sizeof(int64_t), 1, fp);
        fread (&nodeid, sizeof(int), 1, fp);

        ground_truth_t *t = (ground_truth_t*)malloc(sizeof(ground_truth_t));
        t->utime = utime;
        t->nodeid = nodeid;

        g_queue_push_tail (data, t);
    }

    fclose (fp);

    printf ("Read %d ground-truth items.\n", g_queue_get_length (data));

    return data;
}

int util_fix_pose (botlcm_pose_t **pose, GQueue *poses)
{
    if (!*pose || g_queue_is_empty (poses))
        return -1;

    botlcm_pose_t *pstart = (botlcm_pose_t*)g_queue_peek_tail (poses);
    botlcm_pose_t *pend = (botlcm_pose_t*)g_queue_peek_head (poses);

    int64_t min_utime = pstart->utime;
    int64_t max_utime = pend->utime;

    if ((*pose)->utime < min_utime || max_utime < (*pose)->utime) {
        printf ("out of bound: %ld\t%ld\t%ld\n", (*pose)->utime, min_utime, max_utime);
        return -1;
    }

    botlcm_pose_t *p = find_pose_by_utime (poses, (*pose)->utime);

    if (p) {
        free (*pose);
        *pose = botlcm_pose_t_copy (p);
        return 0;
    }

    return -1;
}

/* input image is assumed to be grayscale
 * output image is assumed to be RGB (the tablet can only draw RGB images through pyGTK)
 */
void util_publish_image (lcm_t *lcm, botlcm_image_t *img, const char *channel, int width, int height)
{
    if (!img) return;

    if (img->width != width || img->height != height) {
        IppiSize srcSize = { img->width, img->height };
        IppiRect srcRoi = { 0, 0, img->width, img->height };
        IppiSize dstRoiSize = { width, height };

        double sf_x = (double) width / img->width;
        double sf_y = (double) height / img->height;

        int interp_method = IPPI_INTER_LINEAR;

        unsigned char *outdata = (unsigned char*)calloc (width*height, sizeof(unsigned char));
        int outrow_stride = width;
        ippiResize_8u_C1R (img->data, srcSize, img->row_stride,
                srcRoi, outdata, outrow_stride, dstRoiSize,
                sf_x, sf_y, interp_method);

        unsigned char *outdata_rgb = (unsigned char*)calloc(3*width*height, sizeof(unsigned char));
        for (int i=0;i<width*height;i++) {
            for (int k=0;k<3;k++) {
                outdata_rgb[3*i] = outdata[i];
                outdata_rgb[3*i+1] = outdata[i];
                outdata_rgb[3*i+2] = outdata[i];
            }
        }
        botlcm_image_t outimg;
        outimg.width = width;
        outimg.height = height;
        outimg.size = 3*width*height;
        outimg.data = outdata_rgb;
        outimg.row_stride = 3*outrow_stride;
        outimg.utime = img->utime;
        outimg.pixelformat = CAM_PIXEL_FORMAT_RGB;
        outimg.nmetadata = 0;
        outimg.metadata = NULL;

        botlcm_image_t_publish (lcm, channel, &outimg);
        free (outdata);
        free (outdata_rgb);
    } else {
        botlcm_image_t_publish (lcm, channel, img);
    }

}

void util_publish_map_list (lcm_t *lcm, const char *prefix, const char *suffix, const char *channel) 
{
    char *suffix_rev = strdup (suffix);
    suffix_rev = g_strreverse (suffix_rev);

    GQueue *names = g_queue_new ();

    DIR *dir = opendir (".");
    if (dir) {
        struct dirent *dt = NULL;
        while ((dt = readdir (dir)) != NULL) {
            char *name = strdup (dt->d_name);
            if (strncmp (name, prefix, strlen (prefix)) != 0) continue; // prefix check
            name = g_strreverse (name);
            if (strncmp (name, suffix_rev, strlen (suffix_rev)) != 0) continue; // suffix check
            name = g_strreverse (name);

            g_queue_push_tail (names, name);

        }
    }

    closedir (dir);
  
    g_queue_sort (names, g_strcmp, NULL);
    
    navlcm_dictionary_t *dict = (navlcm_dictionary_t*)calloc(1,sizeof(navlcm_dictionary_t));
    dict->num = 0;
    dict->keys = NULL;
    dict->vals = NULL;

    for (GList *iter = g_queue_peek_head_link (names);iter;iter=iter->next) {
        char *name = (char*)iter->data;
        dict->keys = (char**)realloc(dict->keys, (dict->num+1)*sizeof(char*));
        dict->keys[dict->num] = name;
        dict->vals = (char**)realloc(dict->vals, (dict->num+1)*sizeof(char*));
        dict->vals[dict->num] = strdup (name);
        dict->num++;
    }

    navlcm_dictionary_t_publish (lcm, channel, dict);
    navlcm_dictionary_t_destroy (dict);
    g_queue_free (names);

    free (suffix_rev);
}

