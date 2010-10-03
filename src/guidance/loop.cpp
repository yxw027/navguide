#include "loop.h"

//#define LOOP_HOUGH_TRANSFORM

double * loop_update_correlation_matrix (double *corrmat, int *size, GQueue *bags, double norm);

double* loop_update_full (GQueue *voctree, double *corrmat, int *corrmat_size, navlcm_feature_list_t *features, int id, double bag_word_radius)
{
    // init vocabulary
    if (g_queue_is_empty (voctree)) {
        bags_init (voctree, features, id);
    }

    // search features in vocabulary
    GQueue *qfeatures = g_queue_new (); // features that are un-matched (and yield new bags)
    GQueue *qbags = g_queue_new ();     // bags that found a match
    bags_naive_search (voctree, features, bag_word_radius, qfeatures, qbags, NULL, 1);

    // the ratio of matched features
    double r = 1.0 - 1.0 * g_queue_get_length (qfeatures) / features->num;

    // update similarity matrix
    double *newmat = loop_update_correlation_matrix (corrmat, corrmat_size, qbags, r);

    // print out
    //corrmat_print (newmat, *corrmat_size);

    //corrmat_print (corrmat, corrmat_size);

    // create new bags for unmatched features
    bags_append (voctree, qfeatures, id);
    g_queue_free (qfeatures);

    // update the list of indices for the matched bags
    bags_update (qbags, id);
    g_queue_free (qbags);

    return newmat;
}

double * loop_update_correlation_matrix (double *corrmat, int *size, GQueue *bags, double norm)
{
    if (!corrmat) {
        *size = 1;
        return corrmat_init (1, .0);
    } 
    
    double *m = corrmat_resize_once (corrmat, size, .0);
    
    double *sim = bags_naive_vote (bags, *size);

    // normalize
    vect_normalize (sim, *size, norm);

    // smoothen
//    double *sims = math_smooth_double  (sim, *size, 3);

    printf ("sim vector: \n");
    for (int i=0;i<*size;i++)
        printf ("%.4f ", sim[i]);
    printf ("\n");

    for (int i=0;i<*size;i++) {
        CORRMAT_SET (m, *size, *size-1, i, sim[i]);
    }

    free (sim);

    return m;

}

double loop_line_slope (CvPoint *l)
{
    if (l[0].x != l[1].x)
        return 1.0*(l[1].y-l[0].y)/(l[1].x-l[0].x);
    return .0;
}

void loop_line_orient (CvPoint *l)
{
    if (l[0].x > l[1].x) {
        double xtmp = l[1].x;
        l[1].x = l[0].x;
        l[0].x = xtmp;
        double ytmp = l[1].y;
        l[1].y = l[0].y;
        l[0].y = ytmp;
    }
}

void loop_print_line (CvPoint *l) 
{    
    printf ("%d %d %d %d (%.3f)\n", l[0].x, l[1].x, l[0].y, l[1].y, loop_line_slope (l));
}

void loop_refine_line (CvPoint *l, double *data, int size)
{
    int winsize = 6;
    int x0 = l[0].x, x1=l[1].x, y0=l[0].y, y1=l[1].y;
    double dref0 = data[y0*size+x0];
    double dref1 = data[y1*size+x1];

    printf ("refining...\n");
    loop_print_line (l);

    for (int i=0;i<winsize;i++) {
        for (int j=0;j<winsize;j++) {
            int xx0 = x0 + i - winsize/2;
            int yy0 = y0 + j - winsize/2;
            if (0 <= xx0 && xx0 < size && 0 <= y0 && yy0 < size) {
                double d = data[yy0*size+xx0];
//                printf ("comparing %d %d (%.3f) with %d %d (%.3f)\n", x0, y0, dref0, xx0, yy0, d);
                if (dref0 < d) {
                    dref0 = d;
                    l[0].x = xx0;
                    l[0].y = yy0;
                }
            }
            int xx1 = x1 + i - winsize/2;
            int yy1 = y1 + j - winsize/2;
            if (0 <= xx1 && xx1 < size && 0 <= y1 && yy1 < size) {
                double d = data[yy1*size+xx1];
                if (dref1 < d) {
                    dref1 = d;
                    l[1].x = xx1;
                    l[1].y = yy1;
                }
            }
        }
    }

    loop_print_line (l);
           
}

void loop_refine_lines (GQueue *lines, double *data, int size)
{
    for (GList *iter=g_queue_peek_head_link (lines);iter;iter=iter->next) {
        CvPoint *l = (CvPoint*)iter->data;
        loop_refine_line (l, data, size);
    }
}

void *loop_print_lines (GQueue *lines)
{
    printf (" ************* %d line(s) *****************\n", g_queue_get_length (lines));

    for (GList *iter=g_queue_peek_head_link (lines);iter;iter=iter->next) {
        CvPoint *l = (CvPoint*)iter->data;
        loop_print_line (l);
    }
}


gboolean loop_fuse_two_lines (CvPoint *l1, CvPoint *l2, double theta_thresh, double pixel_thresh)
{
    gboolean fused0 = FALSE, fused1 = FALSE;

    if (fabs (loop_line_slope (l1) - loop_line_slope (l2)) > theta_thresh)
        return FALSE;

    printf ("fusing: line slopes %.4f %.4f\n", loop_line_slope (l1), loop_line_slope (l2));
    loop_print_line (l1);
    loop_print_line (l2);

    // end points are close
    if (powf (l1[0].x-l2[0].x,2)+powf(l1[0].y-l2[0].y,2) < powf (pixel_thresh,2)) {
        l1[0].x = (l1[0].x+l2[0].x)/2;
        l1[0].y = (l1[0].y+l2[0].y)/2;
        fused0 = TRUE;
    }
    if (powf (l1[1].x-l2[1].x,2)+powf(l1[1].y-l2[1].y,2) < powf (pixel_thresh,2)) {
        l1[1].x = (l1[1].x+l2[1].x)/2;
        l1[1].y = (l1[1].y+l2[1].y)/2;
        fused1 = TRUE;
    }

    printf ("fused: %d %d\n", fused0, fused1);

    // projections are close
    double dist=.0, x0, y0;
    if (!fused0) {
        if (math_project_2d_segment (l1[0].x, l1[0].y, l2[0].x, l2[0].y, l2[1].x, l2[1].y, &x0, &y0, &dist)) {
            if (dist < pixel_thresh) {
                l1[0].x = l2[0].x;
                l1[0].y = l2[0].y;
                fused0 = TRUE;
            }
        }
    }
    if (!fused1) {
        if (math_project_2d_segment (l1[1].x, l1[1].y, l2[0].x, l2[0].y, l2[1].x, l2[1].y, &x0, &y0, &dist)) {
            if (dist < pixel_thresh) {
                l1[1].x = l2[0].x;
                l1[1].y = l2[0].y;
                fused1 = TRUE;
            }
        }
    }

    // second segment is fully contained
    double dist1, dist2;
    if (math_project_2d_segment (l2[0].x, l2[0].y, l1[0].x, l1[0].y, l1[1].x, l1[1].y, &x0, &y0, &dist1) &&
            math_project_2d_segment (l2[1].x, l2[1].y, l1[0].x, l1[0].y, l1[1].x, l1[1].y, &x0, &y0, &dist2)) {
        if (dist1 < pixel_thresh && dist2 < pixel_thresh) {
            fused0 = TRUE;
            fused1 = TRUE;
        }
    }

    printf ("fused: %d %d\n", fused0, fused1);

    return (fused0 || fused1);
}

void loop_fuse_lines (GQueue *lines, double theta_thresh, double pixel_thresh)
{
    gboolean done = FALSE;

    while (!done) {
        
        done = TRUE;

        for (GList *iter1=g_queue_peek_head_link (lines);iter1;iter1=iter1->next) {
            GList *iter2=NULL;
            for (iter2=iter1->next;iter2;iter2=iter2->next) {
                if (loop_fuse_two_lines ((CvPoint*)iter1->data, (CvPoint*)iter2->data, 
                        theta_thresh, pixel_thresh)) {
                    break;
                }
            }
            if (iter2) {
                free ((CvPoint*)iter2->data);
                g_queue_remove (lines, iter2->data);
                done = FALSE;
                break;
            }
        }
    }
}
void loop_hough_transform (double *m, int n, gboolean reverse, GQueue *mlines, double canny_thresh, double hough_thresh)
{
    double maxv = corrmat_max (m, n);

    dbg (DBG_CLASS, "maxv = %.3f\n", maxv);

    // create image
    IplImage *src = cvCreateImage (cvSize (n, n), 8, 1);
    IplImage *dst8 = cvCreateImage (cvSize (n, n), 8, 1);
    IplImage *canny = cvCreateImage (cvSize (n, n), 8, 1);
    IplImage* color_dst = cvCreateImage( cvGetSize(src), 8, 3 );
    IplImage* canny_color_dst = cvCreateImage( cvGetSize(src), 8, 3 );

    int step = src->widthStep / sizeof (unsigned char);
    unsigned char *data = (unsigned char*)src->imageData;

    for (int i=0;i<n;i++) {
        for (int j=0;j<n;j++) {

            unsigned char val = MAX (0, MIN (255, (int)(m[i*n+j]/maxv*255.0)));
            data[i*step+j] = val;
        }
    }

    CvMemStorage* storage = cvCreateMemStorage(0);
    CvSeq* lines = 0;

    cvCvtColor( src, color_dst, CV_GRAY2BGR );

    cvSaveImage (reverse ? "input-reverse.png" : "input.png", color_dst);

    double kern1[] = { 0, 1, 2, -1, 0, 1, -2, -1, 0};
    double kern2[] = { -2, -1, 0, -1, 0, 1, 0, 1, 2};
    CvMat *kernel = cvCreateMatHeader (3, 3, CV_64FC1);

    cvSetData (kernel, reverse ? kern2 : kern1, 3*8);

    cvFilter2D (src, dst8, kernel, cvPoint (-1, -1));

    cvCanny (dst8, canny, canny_thresh, 3*canny_thresh, 3);

    cvCvtColor( canny, canny_color_dst, CV_GRAY2BGR );
    cvSaveImage (reverse ? "canny-reverse.png" : "canny.png", canny_color_dst);

    lines = cvHoughLines2( canny, storage, CV_HOUGH_PROBABILISTIC, 1, CV_PI/180, hough_thresh, 10, 5 );

    for(int i = 0; i < lines->total; i++ )
    {
        CvPoint* line = (CvPoint*)cvGetSeqElem(lines,i);

        double slope = loop_line_slope (line);
        if (reverse && slope > 0) continue;
        if (!reverse && slope < 0) continue;

//        if (fabs (1.0 - fabs (slope)) > 2.1)
//            continue;

        CvPoint *clone = (CvPoint*)malloc(2*sizeof(CvPoint));
        clone[0] = line[0];
        clone[1] = line[1];


        printf ("line at : %d %d %d %d %.3f\n", line[0].x, line[0].y, line[1].x, line[1].y, slope);
        g_queue_push_tail (mlines, clone);

        cvLine( color_dst, line[0], line[1], CV_RGB(255,0,0), 3, 8 );

    }
    cvLine (color_dst, cvPoint (0,0), cvPoint(n-1,n-1), CV_RGB(0,255,0), 3, 8);

    cvFlip (color_dst);
    cvSaveImage (reverse ? "color-reverse.png" : "color.png", color_dst);

    cvReleaseImage (&src);
    cvReleaseImage (&dst8);
    cvReleaseImage (&canny);
    cvReleaseImage (&color_dst);
    cvReleaseImage (&canny_color_dst);
    cvClearMemStorage (storage);

}

void loop_free_lines (GQueue *lines)
{
    for (GList *iter=g_queue_peek_head_link (lines);iter;iter=iter->next) {
        CvPoint *l = (CvPoint*)iter->data;
        free (l);
    }
    g_queue_free (lines);
}

component_t* loop_index_to_component (int x0, int x1, int y0, int y1, gboolean reverse, int min_length)
{
    int stx = x1 - x0 ;
    double sty = y1 - y0 ;
    int lastrj = -1;

    if (stx < min_length || fabs (sty) < min_length) {
        dbg (DBG_ERROR, "component is too short (%d,%.3f)", stx, fabs(sty));
        return NULL;
    }

    component_t *comp = component_t_new ();
    comp->reverse = reverse;

    for (int i=0;i<fabs(stx)+1;i++) {
        int ri = (int)(x0 + i);
        int rj = math_round(y0 + sty/stx*i);
        printf ("%d %d\n", ri, rj);
        pair_int_t *p = (pair_int_t*)malloc(sizeof(pair_int_t));
        p->key = ri;
        p->val = rj;
        assert (p->val <= p->key);
        g_queue_push_tail (comp->pt, p);
        /*
           if (lastrj != -1) {
           lastrj += comp->reverse ? +1 : -1;
           while (lastrj != rj) {

           lastrj += comp->reverse ? -1 : +1;
           pair_int_t *p = (pair_int_t*)malloc(sizeof(pair_int_t));
           p->key = ri;
           p->val = lastrj;
           assert (p->val <= p->key);
           g_queue_push_tail (comp->pt, p);
           }
           } else {
           lastrj = rj;
           }*/
    }

    return comp;
}

void loop_line_to_component (CvPoint *l, GQueue *components, gboolean reverse, int min_length)
{
    component_t *comp = loop_index_to_component (l[0].x, l[1].x, l[0].y, l[1].y, reverse, min_length);

    if (comp)
        g_queue_push_tail (components, comp);
}

void loop_lines_to_components (GQueue *lines, GQueue *components, gboolean reverse, int min_length)
{
    for (GList *iter=g_queue_peek_head_link (lines);iter;iter=iter->next) {
        CvPoint *l = (CvPoint*)iter->data;
        loop_line_to_component (l, components, reverse, min_length);
    }
}
/* extract sequences of similar nodes from similarity matrix
 * suggested parameter values in parenthesis.
 *
 * smooth:  true to apply gaussian smoothing (0)
 * canny_thresh:    threshold for canny edges (180)
 * hough_thresh:    threshold for hough transform (10)
 * min_seq_length:  minimum sequence length (3)
 * correlation_diag_size:   width of diagonal set to zero (10)
 * correlation_threshold:   similarity set to zero under this threshold (0.001)
 * alignment_penalty:       penalty for non-diagonal moves in smith & waterman algorithm (0.0001)
 * alignment_threshold:     only consider peaks above this value in alignment matrix (0.15)
 * alignment_tail_thresh:   end sequence if similarity drops under this value (0.005)
 * alignment_min_diag_distance: minimum distance of sequence to diagonal in alignment matrix (10)
 * alignment_max_slope_error:   maximum sequence slope error, in degrees (25.0)
 * alignment_search_radius:     search radius for local maxima in alignment matrix (10)
 * alignment_min_node_id:       minimum node ID for a sequence (50)
 *
 */
GQueue* loop_extract_components_from_correlation_matrix (double *corrmat, int size, gboolean smooth, double canny_thresh, 
        double hough_thresh, int min_seq_length, double correlation_diag_size, double correlation_threshold, 
        double alignment_penalty, double alignment_threshold, double alignment_tail_thresh, 
        int alignment_min_diag_distance, double alignment_max_slope_error, int alignment_search_radius,
        int alignment_min_node_id)
{
    GQueue *components = g_queue_new ();

    // read the corr. matrix from file
    if (!corrmat)
        return components;

    double *corrmatdup = corrmat_copy (corrmat, size);

#ifdef LOOP_HOUGH_TRANSFORM // method 1 (deprecated)

    GQueue *lines = g_queue_new ();

    loop_hough_transform (corrmatdup, size, FALSE, lines, canny_thresh, hough_thresh);
    loop_fuse_lines (lines, to_radians (10.0), 5);
    loop_print_lines (lines);
    loop_refine_lines (lines, corrmatdup, size);
    loop_lines_to_components (lines, components, FALSE, min_seq_length);
    loop_free_lines (lines);

    // same thing in reverse
    lines = g_queue_new ();
    loop_hough_transform (corrmatdup, size, TRUE, lines, canny_thresh, hough_thresh);
    loop_fuse_lines (lines, to_radians (10.0), 5);
    loop_print_lines (lines);
    loop_refine_lines (lines, corrmatdup, size);
    loop_lines_to_components (lines, components, TRUE, min_seq_length);
    loop_free_lines (lines);

    return components;

#else // method 2 (active)

    double *hmat = NULL, *hmatrev = NULL;
    int *hmatind = NULL, *hmatindrev = NULL;
    double *corrmatrev = NULL;

    double kern1[] = { 0, 1, 2, -1, 0, 1, -2, -1, 0};
    double kern2[] = { -2, -1, 0, -1, 0, 1, 0, 1, 2};

    corrmatrev = corrmat_copy (corrmatdup, size);

    // set diagonal to zero
    int diagn = correlation_diag_size;
    for (int i=0;i<size;i++) {
        for (int j=0;j<2*diagn;j++) {
            if (diagn <= i + j && i + j < size + diagn)
                CORRMAT_SET (corrmatdup, size, i, i + j-diagn, .0);
        }
    }

    corrmat_convolve (corrmatdup, size, kern1, 3);
    corrmat_write (corrmatdup, size, "corrmat-conv.txt");

    // reverse the correlation matrix
    corrmat_convolve (corrmatrev, size, kern2, 3);
    corrmat_reverse (corrmatrev, size);
    corrmat_write (corrmatrev, size, "corrmatrev-conv.txt");

    // compute alignment matrix
    //    double thresh = .001;
    //    double penalty = .0001;

    corrmat_compute_alignment_matrix (corrmatdup, size, correlation_threshold, alignment_penalty, FALSE, &hmat, &hmatind);
    corrmat_compute_alignment_matrix (corrmatrev, size, correlation_threshold, alignment_penalty, TRUE, &hmatrev, &hmatindrev);
    corrmat_reverse (hmatrev, size);
    corrmat_int_reverse (hmatindrev, size);

    // write alignment matrix to file
    corrmat_write (hmat, size, "hmat.txt");
    corrmat_int_write (hmatind, size, "hmatind.txt");
    corrmat_write (hmatrev, size, "hmatrev.txt");
    corrmat_int_write (hmatindrev, size, "hmatindrev.txt");

    // find components
    //double minthresh = .15;
    //double alignment_tail_thresh = 0.005;
    corrmat_find_component_list (hmat, hmatind, hmatrev, hmatindrev, size, alignment_threshold, alignment_tail_thresh, min_seq_length, alignment_min_diag_distance, alignment_max_slope_error, alignment_search_radius, alignment_min_node_id, components);

    free (hmat);
    free (hmatind);
    free (hmatrev);
    free (hmatindrev);
    free (corrmatdup);
    free (corrmatrev);

    return components;
#endif
}

void loop_matrix_components_to_image (double *corrmat, int size, GQueue *components)
{
    double maxv = corrmat_max (corrmat, size);

    // create image
    IplImage *src = cvCreateImage (cvSize (size, size), 8, 1);
    IplImage *src_rgb = cvCreateImage (cvSize (size, size), 8, 3);
    IplImage *src2 = cvCreateImage (cvSize (size, size), 8, 1);
    IplImage *src2_rgb = cvCreateImage (cvSize (size, size), 8, 3);

    int step = src->widthStep / sizeof (unsigned char);
    int step_rgb = src2_rgb->widthStep / sizeof (unsigned char);
    unsigned char *data = (unsigned char*)src->imageData;
    unsigned char *data2 = (unsigned char*)src2->imageData;
    unsigned char *data2_rgb = (unsigned char*)src2_rgb->imageData;

    for (int i=0;i<size;i++) {
        for (int j=0;j<size;j++) {

            unsigned char val = 255-MAX (0, MIN (255, (int)(corrmat[i*size+j]/maxv*255.0)));
            data[(size-1-i)*step+j] = val;
            data2[(size-1-i)*step+j] = val;
        }
    }

    cvCvtColor( src, src_rgb, CV_GRAY2BGR );
    cvCvtColor( src2, src2_rgb, CV_GRAY2BGR );

    for (GList *iter=g_queue_peek_head_link (components);iter;iter=iter->next) {
        component_t *c = (component_t*)iter->data;
        for (GList *piter=g_queue_peek_head_link(c->pt);piter;piter=piter->next) {
            pair_int_t *p = (pair_int_t*)piter->data;
            assert (p->key >= p->val);
            assert (0 <= p->key && p->key < size);
            assert (0 <= p->val && p->val < size);
            CvPoint pt = cvPoint (p->val, size-1-p->key);
            cvLine( src2_rgb, pt, pt, CV_RGB(255,0,0), 3, 8 );
        }
    }

    cvSaveImage ("corrmat.png", src_rgb);
    cvSaveImage ("corrmat-sequences.png", src2_rgb);

    cvReleaseImage (&src);
    cvReleaseImage (&src_rgb);
    cvReleaseImage (&src2);
    cvReleaseImage (&src2_rgb);
}

