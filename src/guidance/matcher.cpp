/*
 * This module performs feature matching.
 */

#define MATCH_DBG 0

#include "matcher.h"

int patch_is_border (int col, int row, int radius, int width, int height)
{
    if (col < radius || width-1-radius < col)
        return 1;
    if (row < radius || height-1-radius < row)
        return 1;

    return 0;
}

// compute average color of patch in RGB
//
int patch_average (float *data, int width, int height, float c, float r, int radius, 
                   float *valr, float *valg, float *valb)
{
    if (!data)
        return -1;

    int col = math_round (c);
    int row = math_round (r);

    if (patch_is_border (col, row, radius, width, height))
        return -1;

    float *ptr = data + 3*(row*width+col);
    
    float vr = 0.0;
    float vg = 0.0;
    float vb = 0.0;
    
    int n = 0;

    for (int rr=0;rr<radius;rr++) {
        for (int cc=0;cc<radius;cc++) {
            
            vr += *ptr;
            ptr++;
            vg += *ptr;
            ptr++;
            vb += *ptr;
            ptr++;

            n++;
        }
        
        ptr = ptr - 3*radius + 3*width;
    }

    *valr = vr / n;
    *valg = vg / n;
    *valb = vb / n;
    
    return 0;
}

// compute NCC between two patches in RGB
//
int patch_ncc (float *data1, float *data2, float *data1_mean, float *data2_mean, int width, int height,
               float c1, float r1, float c2, float r2, int radius, float *val)
{
    if (!data1 || !data2)
        return -1;

    
    // compute closest position on the image
    int col1 = math_round (c1);
    int row1 = math_round (r1);
    if (patch_is_border (col1, row1, radius, width, height))
        return -1;
    
    // compute closest position on the image
    int col2 = math_round (c2);
    int row2 = math_round (r2);
    if (patch_is_border (col2, row2, radius, width, height))
        return -1;

    // compute patch average
    float *ptr1 = data1_mean + 3*(width*row1+col1);
    float *ptr2 = data2_mean + 3*(width*row2+col2);

    float av1r = *ptr1; ptr1++;
    float av1g = *ptr1; ptr1++;
    float av1b = *ptr1; ptr1++;

    float av2r = *ptr2; ptr2++;
    float av2g = *ptr2; ptr2++;
    float av2b = *ptr2; ptr2++;
    /*
    if (patch_average (data1, width, height, c1, r1, radius, &av1r, &av1g, &av1b) < 0)
        return -1;

    if (patch_average (data2, width, height, c2, r2, radius, &av2r, &av2g, &av2b) < 0)
        return -1;
    */

    // compute normalized cross-correlation
    float nom = 0.0, denom = 0.0;
    float  d1 = 0.0, d2 = 0.0;
    
    col1 -= radius-1; col2 -= radius-1;
    row1 -= radius-1; row2 -= radius-1;

    ptr1 = data1 + 3*(width*row1+col1);
    ptr2 = data2 + 3*(width*row2+col2);

    for (int ii=0;ii<2*radius-1;ii++) {
        for (int jj=0;jj<2*radius-1;jj++) {
            
            float r1 = *ptr1; ptr1++;
            float g1 = *ptr1; ptr1++;
            float b1 = *ptr1; ptr1++;
            
            float r2 = *ptr2; ptr2++;
            float g2 = *ptr2; ptr2++;
            float b2 = *ptr2; ptr2++;
            
            d1 += (r1-av1r)*(r1-av1r)+(g1-av1g)*(g1-av1g)+(b1-av1b)*(b1-av1b);
            d2 += (r2-av2r)*(r2-av2r)+(g2-av2g)*(g2-av2g)+(b2-av2b)*(b2-av2b);

            nom += (r1-av1r)*(r2-av2r)+(g1-av1g)*(g2-av2g)+(b1-av1b)*(b2-av2b);
        }

        ptr1 = ptr1 + 3*(width-2*radius+1);
        ptr2 = ptr2 + 3*(width-2*radius+1);
    }

    denom = sqrt(d1*d2);

    if (fabs(denom)<1E-6)
        return -1;

    *val = nom/denom;

    return 0;
}

// standard deviation of a patch
int patch_stdev (float *data, float *data_mean, int width, int height, int col, int row, int radius, float *val)
{
    if (patch_is_border (col, row, radius, width, height))
        return -1;

    float stdev = 0.0;

    // compute patch average
    float *ptr = data_mean + 3*(width*row+col);

    float avr = *ptr; ptr++;
    float avg = *ptr; ptr++;
    float avb = *ptr; ptr++;

    // compute stdev
    int c = col - (radius-1);
    int r = row - (radius-1);

    ptr = data + 3*(width*r+c);

    for (int ii=0;ii<2*radius-1;ii++) {
        for (int jj=0;jj<2*radius-1;jj++) {
            
            float cr = *ptr; ptr++;
            float cg = *ptr; ptr++;
            float cb = *ptr; ptr++;
    
            stdev += (cr-avr)*(cr-avr)+(cg-avg)*(cg-avg)+(cb-avb)*(cb-avb);

        }

        ptr = ptr + 3*(width-2*radius+1);
    }

    float surf = (2*radius-1)*(2*radius-1);

    stdev = sqrt(stdev)/surf;

    *val = stdev;

    return 0;
}

// exhaustive search of a patch using NCC
//
int patch_search (float *data1, float *data2, float *data1_mean, float *data2_mean,
                  int width, int height, int c1, int r1, int radius, 
                  int search_radius, int *c2, int *r2, float *val)
{
    float best_ncc = 0.0;
    int best_r = 0, best_c = 0;

    //    for (int cc=MAX(0,c1-search_radius);cc<MIN(width,c1+search_radius);cc++) {
    //    for (int rr=MAX(0,r1-search_radius);rr<MIN(height,r1+search_radius);rr++) {
    for (int ii=0;ii<=2*search_radius;ii++) {

        if (c1 + ii < search_radius)
            continue;

        for (int jj=0;jj<=2*search_radius;jj++) {

            if (r1 + jj < search_radius)
                continue;

            int cc = c1 + ii - search_radius;
            int rr = r1 + jj - search_radius;
            
            float ncc = 0.0;

            // skip if outside of search radius
            if (sqrt ((cc-c1)*(cc-c1)+(rr-r1)*(rr-r1)) > search_radius)
                continue;

            // skip if ncc computation failed
            if (patch_ncc (data1, data2, data1_mean, data2_mean, width, height, c1, r1, cc, rr, radius, &ncc) < 0)
                continue;
            
            // keep if better
            if (ncc > best_ncc) {
                best_ncc = ncc;
                best_c = cc;
                best_r = rr;
            }
        }
    }

    // failed
    if (best_ncc < 1E-6)
        return -1;

    *c2 = best_c;
    *r2 = best_r;
    *val = best_ncc;

    return 0;
}
tracker_t::tracker_t ()
{
    prev = next = NULL;
    prev_32f = next_32f = NULL;
    prev_mean_32f = next_mean_32f = NULL;
    pts = NULL;

    width = height = 0;
}

tracker_t::~tracker_t ()
{
    if (next) ippFree (next);
    if (prev) ippFree (prev);

    if (next_32f) ippFree (next_32f);
    if (prev_32f) ippFree (prev_32f);

    if (next_mean_32f) ippFree (next_mean_32f);
    if (prev_mean_32f) ippFree (prev_mean_32f);

    free (pts);
}

void tracker_t::init (int w, int h)
{
    if (!prev)
        prev = (Ipp8u*)ippMalloc (3*w*h);
    if (!next)
        next = (Ipp8u*)ippMalloc (3*w*h);

    if (!prev_32f)
        prev_32f = (Ipp32f*)ippMalloc (3*w*h*sizeof(float));
    if (!next_32f)
        next_32f = (Ipp32f*)ippMalloc (3*w*h*sizeof(float));

    if (!prev_mean_32f)
        prev_mean_32f = (Ipp32f*)ippMalloc (3*w*h*sizeof(float));
    if (!next_mean_32f)
        next_mean_32f = (Ipp32f*)ippMalloc (3*w*h*sizeof(float));

    width = w;
    height = h;
}

void tracker_t::copy_data (unsigned char *data, int w, int h, int radius)
{
    init (w, h);

    IppiSize dst_roi = { width, height };

    // copy next to prev
    ippiCopy_8u_C3R (next, 3*width, prev, 3*width, dst_roi);

    // copy next_32f to prev_32f
    ippiCopy_32f_C3R (next_32f, 3*width*sizeof(float), prev_32f, 3*width*sizeof(float), dst_roi);
    
    // copy next_mean_32f to prev_mean_32f
    ippiCopy_32f_C3R (next_mean_32f, 3*width*sizeof(float), prev_mean_32f, 3*width*sizeof(float), dst_roi);

    // scale from unsigned char [0-255] to float [0.0-1.0]
    ippiScale_8u32f_C3R (data, 3*width, next_32f,
                         3*width * sizeof(float), dst_roi , 0.0, 1.0);
    
    // compute mean image
    compute_mean_image (next_32f, next_mean_32f, w, h, radius);

    // copy to next
    ippiCopy_8u_C3R (data, 3*width, next, 3*width, dst_roi);

}

void tracker_t::ncc_run (int col, int row, int radius, int search_radius)
{
    if (pts) {
        free (pts);
        pts = NULL;
    }

    npts = 0;

    int col2, row2;
    float ncc;
    
    if (patch_search (prev_32f, next_32f, prev_mean_32f, next_mean_32f, width, height, col, row, radius, 
                      search_radius, &col2, &row2, &ncc) < 0) {
        dbg (DBG_INFO, "[track] failed to search patch!");
        return;
    }

    pts = (int*)realloc (pts, 4*(npts+1)*sizeof(int));

    pts[4*npts+0] = col;
    pts[4*npts+1] = row;
    pts[4*npts+2] = col2;
    pts[4*npts+3] = row2;
    
    npts++;
}

void tracker_t::ncc_run_straight_calibration (int nx, int ny, int radius, int search_radius)
{
    if (pts) {
        free (pts);
        pts = NULL;
    }

    npts = 0;

    GTimer *timer = g_timer_new ();

    // track a grid of points
    int dc = math_round(1.0*width/nx);
    int dr = math_round(1.0*height/ny);

    for (int col=dc;col<width;col+=dc) {
        for (int row=dr;row<height;row+=dr) {
            
            int col2, row2;
            float ncc;

            // compute patch stdev
            float stdev = 0.0;
            if (patch_stdev (prev_32f, prev_mean_32f, width, height, col, row, radius, &stdev) < 0)
                continue;

            // search for patch using NCC
            if (patch_search (prev_32f, next_32f, prev_mean_32f, next_mean_32f, width, height, col, row, radius, 
                search_radius, &col2, &row2, &ncc) < 0)
                continue;
            
            // filter
            if (stdev*1000 < 2)// || (1.0-ncc)*1000 > 10)
                continue;

            pts = (int*)realloc(pts, 5*(npts+1)*sizeof(int));
            
            pts[5*npts+0] = col;
            pts[5*npts+1] = row;
            pts[5*npts+2] = col2;
            pts[5*npts+3] = row2;
            pts[5*npts+4] = math_round((1.0-ncc)*1000);

            npts++;
        }
    }
    
    gulong usecs;
    
    dbg (DBG_INFO, "[tracker] process %d points in %.3f sec", npts, g_timer_elapsed (timer, &usecs));
    
    g_timer_destroy (timer);
}

// keep only the top <ratio>% of the tracks based on the NCC
//
void tracker_t::ncc_filter_tracks (float ratio)
{
    if (npts <= 1)
        return;

    // build histogram
    int hist_size = 100;

    double *data = (double*)malloc(npts*sizeof(double));
    for (int i=0;i<npts;i++)
        data[i] = pts[5*i+4];

    int *hist = histogram_build (data, npts, hist_size, NULL, NULL);

    // determine threshold value
    int count = 0;
    int i;
    for (i=0;i<hist_size;i++) {
        count += hist[i];
        if (1.0 * count / npts > ratio)
            break;
    }

    double min, max;
    histogram_min_max (data, npts, &min, &max);

    float thresh = (float)(min + (1.0*i/hist_size) * (max-min));

    dbg (DBG_INFO, "[tracker] threshold value : %.3f", thresh);
    
    // filter elements
    int *new_pts = NULL;
    int n_new_pts = 0;

    for (int j=0;j<npts;j++) {
        if (pts[5*j+4] < thresh) {
            new_pts = (int*)realloc(new_pts, (n_new_pts+1)*5*sizeof(int));
            for (int k=0;k<5;k++)
                new_pts[5*n_new_pts+k] = pts[5*j+k];
            n_new_pts++;
        }
    }

    // copy
    free (pts);
    pts = new_pts;
    npts = n_new_pts;

    // free
    free (hist);
    free (data);
}

void tracker_t::ncc_publish (lcm_t *lcm, int sensor_id)
{
}

void tracker_t::compute_mean_image (Ipp32f *src, Ipp32f *dst, int w, int h, int radius)
{
    GTimer *timer = g_timer_new ();

    // temp images
    Ipp32f *mean_x = (Ipp32f*)ippMalloc(3*width*height*sizeof(float));

    // mean X
    for (int r=0;r<height;r++) {
        
        float valr = 0.0, valg = 0.0, valb = 0.0;
        
        float *ptr1 = src + 3 * r * width;
        float *ptr2 = ptr1 + 3 * (2*radius-1);
        float *ptrd = mean_x + 3 * (r * width + radius - 1);

        for (int c=0;c<width-2*radius;c++) {
            
            // first column, init mean value
            if (c == 0) {
                float *ptr3 = ptr1;
                for (int k=0;k<2*radius-1;k++) {
                    valr += *ptr3; ptr3++;
                    valg += *ptr3; ptr3++;
                    valb += *ptr3; ptr3++;
                }
            } else {
                // other columns, simply update
                valr += *ptr2-*ptr1; ptr1++; ptr2++;
                valg += *ptr2-*ptr1; ptr1++; ptr2++;
                valb += *ptr2-*ptr1; ptr1++; ptr2++;
            }

            *ptrd = valr; ptrd++;
            *ptrd = valg; ptrd++;
            *ptrd = valb; ptrd++;

        }
    }
            
    float surf = (2*radius-1)*(2*radius-1);

    // dst
    for (int c=0;c<width;c++) {
        
        float valr = 0.0, valg = 0.0, valb = 0.0;
        
        float *ptr1 = mean_x + 3 * c;
        float *ptr2 = ptr1 + 3 * (2*radius-1) * width;
        float *ptrd = dst + 3 * c + 3 * width * (radius - 1);

        for (int r=0;r<height-2*radius;r++) {
            
            // first row, init mean value
            if (r == 0) {
                float *ptr3 = ptr1;
                for (int k=0;k<2*radius-1;k++) {
                    valr += *ptr3; ptr3++;
                    valg += *ptr3; ptr3++;
                    valb += *ptr3; ptr3++;
                    ptr3 += 3*(width-1);
                }
            } else {
                // other columns, simply update
                valr += *ptr2-*ptr1; ptr1++; ptr2++;
                valg += *ptr2-*ptr1; ptr1++; ptr2++;
                valb += *ptr2-*ptr1; ptr1++; ptr2++;
                ptr1 += 3*(width-1); ptr2 += 3*(width-1);
            }

            *ptrd = valr/surf; ptrd++;
            *ptrd = valg/surf; ptrd++;
            *ptrd = valb/surf; ptrd++;
            ptrd += 3*(width-1);
        }
    }
    
    // free
    ippFree (mean_x);

    gulong usecs;
    
    dbg (DBG_INFO, "[track] compute mean perf %.3f sec.", g_timer_elapsed (timer, &usecs));
    
    g_timer_destroy (timer);
}

int feature_edge_code (double col, double row, int width, int height)
{
    double ratio = 1.0 * width / height;

    double abot = ratio * (1.0*height - 1 - row);
    double atop = ratio * row;
    double alef = col;
    double arig = 1.0*width - 1 - col;

    double min_dist = fmin (fmin (fmin (atop, abot), alef), arig);

    if (fabs (min_dist-alef) < 1E-6) return EDGE_LEFT;
    if (fabs (min_dist-atop) < 1E-6) return EDGE_TOP;
    if (fabs (min_dist-arig) < 1E-6) return EDGE_RIGHT;
    if (fabs (min_dist-abot) < 1E-6) return EDGE_BOTTOM;
    return -1;
}

gboolean feature_on_edge (double col, double row, int width, int height, double maxdist)
{
    if (col < maxdist) return TRUE;
    if (row < maxdist) return TRUE;
    if (width-1-col < maxdist) return TRUE;
    if (height-1-row < maxdist) return TRUE;
    return FALSE;
}

int neighbor_features (float col1, float row1, int sid1,
                       float col2, float row2, int sid2,
			           int width, int height,
                       int ncells, int max_cell_dist)
{
    int r1 = (int)(1.0*row1 / height * ncells);
    int c1 = (int)(1.0*col1 / width * ncells);

    int r2 = (int)(1.0*row2 / height * ncells);
    int c2 = (int)(1.0*col2 / width * ncells);
  
    if (sid1 == sid2)
        return (fabs(r1-r2) < max_cell_dist && fabs(c1-c2) < max_cell_dist);

    gboolean onedge1 = (r1<max_cell_dist || r1>ncells-1-max_cell_dist) && 
			(c1<max_cell_dist || c1>ncells-1-max_cell_dist);
    gboolean onedge2 = (r2<max_cell_dist || r2>ncells-1-max_cell_dist) && 
			(c2<max_cell_dist || c2>ncells-1-max_cell_dist);

    return onedge1 && onedge2;
}
// compute multiple feature matches
// keep the <topK> best matches in set2 for each feature in set1
// <maxdist>: maximum distance between two matches in pixels.
// set it to a negative value if you do not want to use it.
// <thresh>: threshold ratio for the second closest neighbor testing. 
// set it to 1.0 if you do not want to use it. typical value: .80
// <maxdist_ft>: maximum distance in feature space.
// set it to a negative value if you do not want to use it.
// <monogamy>: if set to true, monogamy is enforced within each camera
// <within_camera>: if this is set to true, matches are allowed with the same camera.
// <across_camera>: if this is set to true, matches are allowed across cameras.
//
navlcm_feature_match_set_t *
find_feature_matches_multi (navlcm_feature_list_t *keys1, 
                            navlcm_feature_list_t *keys2, gboolean within_camera,
                            gboolean across_cameras,
                            int topK, double thresh, 
                            double maxdist, double maxdist_ft, gboolean monogamy)
{
    // init to empty set
    navlcm_feature_match_set_t *matches = 
        (navlcm_feature_match_set_t *)malloc
        (sizeof(navlcm_feature_match_set_t *));
    matches->num = 0;
    matches->el = NULL;

    // skip if no features
    if (!keys1 || !keys2 || keys1->num < 1 || keys2->num < 1)
        return matches;

    // for each feature in set1, perform a brute-force search
    // for the topK best matches in set2
    //
    int index = 0;
    double sqmaxdist = maxdist * maxdist;

    for (int i=0;i<keys1->num;i++) {

        navlcm_feature_t *key = keys1->el + i;

        // init the topK indices
        int ind_j[topK];     // index
        int ind_s[topK];     // sensorid
        int64_t ind_u[topK]; // utime
        double ind_d[topK];  // distance (in feature space)
        for (int k=0;k<topK;k++) {
             ind_j[k] = -1;
             ind_s[k] = -1;
             ind_d[k] = -1.0;
             ind_u[k] = 0;
        }

        // parse set 2
        for (int j=0 ; j<keys2->num ; j++) {

            navlcm_feature_t *tar = keys2->el + j;

            // use laplacian to skip early
            if (key->laplacian != tar->laplacian)
                continue;

            // within and across camera check
            //
            if (!across_cameras && key->sensorid != tar->sensorid)
                continue;
            if (!within_camera && key->sensorid == tar->sensorid)
                continue;

            // test distance in image space
            double img_sqdist = (key->col - tar->col)*(key->col - tar->col) +
                                (key->row - tar->row)*(key->row - tar->row);
            
            if (maxdist > 1E-6 && key->sensorid == tar->sensorid && img_sqdist > sqmaxdist) continue;

            // compute distance in feature space
            double dist = 
                vect_sqdist_float (key->data, tar->data, key->size) / key->size;

            // filter on feature space distance
            // this criteria only applies within a camera
            //
            if (maxdist_ft > 1E-6 && key->sensorid == tar->sensorid && dist > maxdist_ft) continue;

            // update the topK indices
            for (int k=0;k<topK;k++) {
                if (ind_j[k] == -1 || dist < ind_d[k]) {
                    // slide elements
                    for (int kk=topK-1;kk>k;kk--) {
                        ind_j[kk] = ind_j[kk-1];
                        ind_d[kk] = ind_d[kk-1];
                        ind_s[kk] = ind_s[kk-1];
                        ind_u[kk] = ind_u[kk-1];
                    }
                    ind_j[k] = j;
                    ind_d[k] = dist;
                    ind_s[k] = tar->sensorid;
                    ind_u[k] = tar->utime;
                    break;
                }
            }
        }
       
        // count actual number of matches
        int nm=0;
        for (int k=0;k<topK;k++) {
            if (ind_j[k] != -1)
                nm++;
        }

        // skip empty matches
        if (nm == 0) continue;

        // skip non-selective matches
        if (nm > 1 && thresh * ind_d[1] < 1.0 * ind_d[0]) continue;

        // create a new multi-match
        navlcm_feature_match_t *match = 
            navlcm_feature_match_t_create (key);

        if (nm > 0) {

            for (int k=0;k<topK;k++) {

                if (ind_j[k] == -1) break;
                
                navlcm_feature_t *key2 = navlcm_feature_t_copy (keys2->el + ind_j[k]);

                match = navlcm_feature_match_t_insert (match, key2, ind_d[k]);

                free (key2);

            }
        }

        navlcm_feature_match_set_t_insert (matches, match);
        free (match);

        index++;

    }

    if (monogamy)
        matches = filter_matches_polygamy (matches, TRUE);

    return matches;
}

typedef struct { int index; double dist; } matcher_pair_t;

// filter matches to avoid polygamy within the same camera
// <within_camera>: if set to true, monogamy is only reinforced within the same camera
//
navlcm_feature_match_set_t *
filter_matches_polygamy (navlcm_feature_match_set_t *matches, gboolean within_camera)
{
    if (!matches || matches->num == 0) return matches;
   
    int nmatches = matches->num;

    GTimer *timer = g_timer_new ();

    // create a hash table
    GHashTable *hash = g_hash_table_new_full (g_str_hash, g_str_equal, (GDestroyNotify)g_str_free, 
            (GDestroyNotify)g_free);

    // populate the hash table

    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *match = matches->el + i;
        if (match->num == 0) continue;
        for (int k=0;k<match->num;k++) {
            if (match->dst[k].index < 0) continue;

            // do not incorporate in hash table if across different cameras
            gboolean same_camera = match->dst[k].sensorid == match->src.sensorid;
            if (within_camera && !same_camera) continue; // these matches will be accepted

            matcher_pair_t pa; pa.index = i; pa.dist = match->dist[k];

            // query hash table
            char key[20];
            sprintf (key, "%d.%d", match->dst[k].sensorid, 
                                    match->dst[k].index);
            gpointer data = g_hash_table_lookup (hash, key);
            if (data) {
                matcher_pair_t *pb = (matcher_pair_t*)data;
                if (pb->dist < pa.dist) continue;
            }

            // if there was no entry or a worse entry, replace
            matcher_pair_t *copy = g_new (matcher_pair_t, 1);
            copy->index = pa.index; copy->dist = pa.dist;
            g_hash_table_replace (hash, g_strdup(key), copy);

        }
    }

    // create a new set of matches from scratch
    navlcm_feature_match_set_t *new_set = 
        navlcm_feature_match_set_t_create ();

    // parse the old set
    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *match = matches->el + i;
        navlcm_feature_match_t *new_match = 
            navlcm_feature_match_t_create(&match->src);
        for (int k=0;k<match->num;k++) {
            gboolean ok = FALSE;
            
            // do not incorporate in hash table if across different cameras
            gboolean same_camera = match->dst[k].sensorid == match->src.sensorid;
            if (within_camera && !same_camera) {
                ok = TRUE;
            } else {
                // search in the hash table
                char key[20];
                sprintf (key, "%d.%d", match->dst[k].sensorid, 
                                        match->dst[k].index);
                gpointer data = g_hash_table_lookup (hash, key);
                if (data) {
                    matcher_pair_t *pa = (matcher_pair_t*)data;
                    // keep if does correspond to this match
                    if (pa->index  == i) {
                        ok = TRUE;
                    }
                }
            }

            if (ok) {
                navlcm_feature_t *fc = navlcm_feature_t_copy (match->dst + k);
                new_match = navlcm_feature_match_t_insert (new_match, fc, match->dist[k]);
                free (fc);
            }
        }

        if (new_match->num == 0) { 
            navlcm_feature_match_t_destroy (new_match); continue;
        } else {
            // add match to set
            navlcm_feature_match_set_t_insert (new_set, new_match);
            free (new_match);
        }
    }
       
    // destroy the old set
    navlcm_feature_match_set_t_destroy (matches);

    // destroy the hash table
    g_hash_table_destroy (hash);

    gulong usecs;
    double secs = g_timer_elapsed (timer, &usecs);
    dbg (DBG_INFO, "[tracker] filter returned %d/%d matches in %.3f secs.", new_set->num, nmatches, secs);
    g_timer_destroy (timer);

    return new_set;
}

/* Manhattan distance between two features
 */
double manhattan_distance (navlcm_feature_t *f1, navlcm_feature_t *f2)
{
    return fabs (f1->col - f2->col) + fabs (f1->row - f2->row);
}

/* find the closest image edge and return the corresponding index
 */
unsigned char edge_to_index (navlcm_feature_t *f, int width, int height)
{
    double dleft = f->col;
    double dright = width - f->col;
    double dtop = f->row;
    double dbottom = height - f->row;

    double d = fmin (fmin (fmin (dleft, dright), dtop), dbottom);

    if (fabs (d-dleft) < 1E-4)   return 4 * f->sensorid + 0;
    if (fabs (d-dright) < 1E-4)  return 4 * f->sensorid + 1;
    if (fabs (d-dtop) < 1E-4)    return 4 * f->sensorid + 2;
    if (fabs (d-dbottom) < 1E-4) return 4 * f->sensorid + 3;
    assert (false);
    return -1;

}

float ncc_float (float *d1, float *d2, int size)
{
    float mean1 = .0, mean2 = .0;
    for (int i=0;i<size;i++) {
        mean1 += d1[i];
        mean2 += d2[i];
    }
    mean1 /= size;
    mean2 /= size;

    float norm1 = .0, norm2 = .0;
    for (int i=0;i<size;i++) {
        norm1 += powf (d1[i]-mean1, 2);
        norm2 += powf (d2[i]-mean2, 2);
    }
    norm1 = sqrt (norm1);
    norm2 = sqrt (norm2);

    float ncc = .0;
    for (int i=0;i<size;i++) {
        ncc += (d1[i]-mean1)*(d2[i]-mean2)/(norm1*norm2);
    }
    return ncc;
}

/* Match features between two sets. We assume that feature descriptors are normalized.
 * Therefore, minimizing the SSD is equivalent to maximazing the dot product, which
 * is computed between each pair of features using a matrix representation.
 *
 * options:
 *              <monogamy>: enforce monogamy
 *              <mutual_consistency>: mutual consistency check
 *              <maxdist> : maximum distance between features in pixels (<0 to skip)
 *              <matching_mode>: MATCHING_DOTPROD or MATCHING_NCC
 */
int
find_feature_matches_fast (navlcm_feature_list_t *keys1, 
                            navlcm_feature_list_t *keys2, gboolean within_camera,
                            gboolean across_cameras, gboolean monogamy, gboolean mutual_consistency,
                            double thresh, double maxdist, int matching_mode,
                            navlcm_feature_match_set_t *matches)
{
    // init to empty set
    matches->num = 0;
    matches->el = NULL;

    // skip if no features
    if (!keys1 || !keys2 || keys1->num == 0 || keys2->num == 0)
        return -1;

    GTimer *timer = g_timer_new ();

    // stack descriptors in a matrix
    float *desc1 = (float*)malloc(keys1->num * keys1->desc_size * sizeof (float));
    float *desc2 = (float*)malloc(keys2->num * keys2->desc_size * sizeof (float));
    if (keys1->desc_size != keys2->desc_size) {
        dbg (DBG_ERROR, "descriptor size inconsistency: %d %d", keys1->desc_size, keys2->desc_size);
    }
    assert (keys1->desc_size == keys2->desc_size);
    for (int i=0;i<keys1->num;i++) {
        for (int j=0;j<keys1->desc_size;j++) {
            desc1[i*keys1->desc_size+j] = keys1->el[i].data[j];
        }
    }
    for (int i=0;i<keys2->desc_size;i++) {
        for (int j=0;j<keys2->num;j++) {
            desc2[i*keys2->num+j] = keys2->el[j].data[i];
        }
    }

    float *dotprod = (float*)malloc(keys1->num * keys2->num * sizeof(float));

    if (matching_mode == MATCHING_DOTPROD) {
        // multiply the two matrices (fast)
        math_matrix_mult_mkl_float (keys1->num, keys2->num, keys1->desc_size, desc1, desc2, dotprod);
    } else if (matching_mode == MATCHING_NCC) {
        // cross-correlation (slow)
        for (int i=0;i<keys1->num;i++) {
            for (int j=0;j<keys2->num;j++) {
                double ncc = ncc_float (keys1->el[i].data, keys2->el[j].data, keys1->el[i].size);
                dotprod[i*keys2->num+j] = ncc;
            }
        }
    }

    // enforce laplacian correlation
    for (int i=0;i<keys1->num;i++) {
        for (int j=0;j<keys2->num;j++) {
            if (keys1->el[i].laplacian != keys2->el[j].laplacian) {
                dotprod[i*keys2->num+j] = .0;
            }
        }
    }

    // enforce camera tests
    if (!within_camera || !across_cameras) {
        for (int i=0;i<keys1->num;i++) {
            for (int j=0;j<keys2->num;j++) {
                if (!within_camera && keys1->el[i].sensorid == keys2->el[j].sensorid) {
                    dotprod[i*keys2->num+j] = .0;
                }
                if (!across_cameras && keys1->el[i].sensorid != keys2->el[j].sensorid) {
                    dotprod[i*keys2->num+j] = .0;
                }
            }
        }
    }

    // enforce max distance in image space
    if (maxdist > 0) {
        navlcm_feature_t *f1 = (navlcm_feature_t*)keys1->el;
        for (int i=0;i<keys1->num;i++) {
            navlcm_feature_t *f2 = (navlcm_feature_t*)keys2->el;
            for (int j=0;j<keys2->num;j++) {
                if (f2->sensorid == f1->sensorid) {
                    double mandist = manhattan_distance (f1, f2);
                    if (mandist > maxdist) {
                        dotprod[i*keys2->num+j] = .0;
                    }
                }
                f2++;
            }
            f1++;
        }
    }

    // parse distance matrix
    int *best_inds = (int*)malloc(keys1->num*sizeof(int));
    int *secn_inds = (int*)malloc(keys1->num*sizeof(int));

    for (int i=0;i<keys1->num;i++) {
        int *best_ind=best_inds+i;
        int *secn_ind=secn_inds+i;
        *best_ind = -1;
        *secn_ind = -1;
        float best_dot = .0;
        float secn_dot = .0;

        // find the first and second best matches
        for (int j=0;j<keys2->num;j++) {
            double dot = dotprod[i*keys2->num+j];
            if (*best_ind == -1 || best_dot < dot) {
                *secn_ind = *best_ind;
                secn_dot = best_dot;
                *best_ind = j;
                best_dot = dot;
            } else if (secn_dot < dot) {
                *secn_ind = j;
                secn_dot = dot;
            }
        }
        assert (*best_ind != -1);

        // skip if threshold criteria not met
        if (1.0-best_dot > thresh  * (1.0-secn_dot)) {
            *best_ind = -1;
        } else {
            if (mutual_consistency) {
                // skip if mutual consistency fails
                for (int ii=0;ii<keys1->num;ii++) {
                    if (ii != i && best_dot < dotprod[ii*keys2->num+*best_ind]) {
                        *best_ind = -1;
                        break;
                    }
                }
            }
        }
    }

    // monogamy
    if (monogamy) {
        for (int i=0;i<keys1->num;i++) {
            for (int j=i+1;j<keys1->num;j++) {
                if (best_inds[i] == best_inds[j] && best_inds[i] != -1) {
                    double doti = dotprod[i*keys2->num + best_inds[i]];
                    double dotj = dotprod[j*keys2->num + best_inds[j]];
                    if (doti < dotj)
                        best_inds[i] = -1;
                    else
                        best_inds[j] = -1;
                }
            }
        }
    }

    // generate matches
    if (matches) {
        for (int i=0;i<keys1->num;i++) {

            if (best_inds[i] != -1) {

                // create a new match
                navlcm_feature_match_t *match = navlcm_feature_match_t_create (keys1->el + i);
                navlcm_feature_t *fc = navlcm_feature_t_copy ( keys2->el + best_inds[i]);
                match = navlcm_feature_match_t_insert (match, fc, fabs(1.0 - dotprod[i*keys2->num+best_inds[i]]));
                free (fc);

                navlcm_feature_match_set_t_insert (matches, match);
                free (match);
            }
        }
    }

    //    dbg (DBG_INFO, "generated %d matches in %.4f secs.", matches->num, g_timer_elapsed (timer, NULL));

    g_timer_destroy (timer);

    free (desc1);
    free (desc2);
    free (secn_inds);
    free (best_inds);
    free (dotprod);

    // sanity check
#if MATCH_DBG
    if (matches)
        match_sanity_check (matches, keys1, keys2, within_camera, across_cameras);
#endif

    return 0;
}

/* Matching sanity check.
*/
void match_sanity_check (
        navlcm_feature_match_set_t *matches,
        navlcm_feature_list_t *set1, navlcm_feature_list_t *set2, 
        gboolean within_camera, gboolean across_cameras)
{
    // for each match, make sure that no better match can be found
    //
    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *match = matches->el + i;

        navlcm_feature_t *f1 = &match->src;
        navlcm_feature_t *f2 = match->dst;

        if (!within_camera  && f1->sensorid == f2->sensorid) assert (false);
        if (!across_cameras && f1->sensorid != f2->sensorid) assert (false);

        double d = vect_sqdist_float (f1->data, f2->data, f1->size);
        double dot = vect_dot_float (f1->data, f2->data, f1->size);

        for (int j=0;j<set2->num;j++) {
            navlcm_feature_t *ff2 = set2->el + j;

            if (ff2->laplacian != f1->laplacian) continue;

            double dd = vect_sqdist_float (f1->data, ff2->data, f1->size);
            double ddot = vect_dot_float (f1->data, ff2->data, f1->size);

            if (d > dd + 1E-6) {
                dbg (DBG_ERROR, "d(%d,%d) = %.5f (dot: %.4f) , dd(%d,%d) = %.5f (dot: %.4f)", f1->index, f2->index, d, dot, f1->index, ff2->index, dd, ddot);
            }
            assert (d < dd + 1E-6);
        }
    }

}

/* compute the min distance between a feature and a set of features, in pixels
*/
double features_min_distance (navlcm_feature_t *f1, GList *features)
{
    assert (f1 && features);

    double min_dist = 1000000.0;

    for (GList *iter = g_list_first (features); iter!=NULL; iter=g_list_next (iter)) {
        navlcm_feature_t *f2 = (navlcm_feature_t*)iter->data;
        if (f2->sensorid != f1->sensorid) continue;
        double dist = sqrt ((f2->col - f1->col)*(f2->col - f1->col) + (f2->row - f1->row)*(f2->row - f1->row));
        if (dist < min_dist)
            min_dist = dist;
    }

    return min_dist;
}

/*
   GTimer *timer = g_timer_new ();

// compute a table containing the correspondence between 
// feature indices and distances
//
printf ("pol: init table.\n"); fflush(stdout);

int sid1max=0, sid2max=0, id1max=0, id2max=0;
navlcm_feature_match_t *match = matches->el;
for (int i=0;i<matches->num;i++) {
if (match->num == 0) continue;
sid1max = MAX(sid1max, match->src.sensorid);
id1max  = MAX(id1max,  match->src.index);

for (int k=0;k<match->num;k++) {
if (match->dst[k].index < 0) continue;
sid2max = MAX(sid2max, match->dst[k].sensorid);
id2max  = MAX(id2max,  match->dst[k].index);
}
match++;
}

printf ("pol: malloc table.\n"); fflush(stdout);
double ****tab;
tab = (double****)malloc(sid1max*sizeof(double***));
for (int i=0;i<=sid1max;i++) {
tab[i] = (double***)malloc(sid2max*sizeof(double**));
for (int j=0;j<=sid2max;j++) {
tab[i][j] = (double**)malloc(id1max*sizeof(double*));
for (int k=0;k<=id1max;k++) {
tab[i][j][k] = (double*)malloc(id2max*sizeof(double));
for (int r=0;r<=id2max;r++) {
tab[i][j][k][r] = -1.0;
}
}
}
}

// populate the table
//
printf ("pol: populate table.\n"); fflush(stdout);

match = matches->el;
for (int i=0;i<matches->num;i++) {
if (match->num == 0) continue;
for (int k=0;k<match->num;k++) {
if (match->dst[k].index < 0) continue;
double dist =  tab[match->src.sensorid][match->dst[k].sensorid]
[match->src.index][match->dst[k].index];
if (dist < 0 || match->dist[k] < dist) {
tab[match->src.sensorid][match->dst[k].sensorid]
[match->src.index][match->dst[k].index] = match->dist[k];
}
}
match++;
}

// now for each match, check that its distance corresponds to
// the one in the table
// (create a new set of matches from scratch)
printf ("pol: populate new set.\n"); fflush(stdout);

navlcm_feature_match_set_t *new_set =
(navlcm_feature_match_set_t *)malloc(sizeof
(navlcm_feature_match_set_t));
new_set->el = NULL;
new_set->num = 0;

int count = 0;

// parse old matches
//
match = matches->el;
for (int i=0;i<matches->num;i++) {
    if (match->num == 0) continue;

    navlcm_feature_match_t new_match = 
        navlcm_feature_match_t_create (&match->src);

    for (int k=0;k<match->num;k++) {
        int dist = tab[match->src.sensorid]
            [match->dst[k].sensorid]
            [match->src.index]
            [match->dst[k].index];

        if (fabs(match->dist[k] - dist) > 1E-6) {
            if (k==0) count++;
            continue;
        }

        // keep match
        new_match = navlcm_feature_match_t_insert (new_match,
                &match->dst[k],
                match->dist[k]);

    }

    // add match to set
    navlcm_feature_match_set_t_insert (new_set, new_match);

    // move pointer
    match++;
}

gulong usecs;
double secs = g_timer_elapsed (timer, &usecs);

dbg (DBG_INFO, "[tracker] enforced matches monogamy in %.3f secs."
        " dropped %d matches.", secs, count);

g_timer_destroy (timer);

// destroy old set

navlcm_feature_match_set_t_destroy (matches);

// free memory
printf ("pol: free table.\n"); fflush(stdout);
for (int i=0;i<=sid1max;i++) {
    for (int j=0;j<=sid2max;j++) {
        for (int k=0;k<=id1max;k++) {
            free(tab[i][j][k]);
        }
        free (tab[i][j]);
    }
    free (tab[i]);
}
free (tab);

return new_set;
}
*/

int track_find_closest_index (track_t tr, double col, double row, int sensorid, double *dist)
{
    int bindex = -1;
    double best_dist = 0.0;

    for (int b=0;b<tr.end-tr.start;b++) {
        if (tr.sid[b] != sensorid)
            continue;
        double d = math_square_distance_double (col, row, tr.col[b], tr.row[b]);
        if (bindex == -1) {
            bindex = b;
            best_dist = d;
            continue;
        } else {
            if (d < best_dist) {
                bindex = b;
                best_dist = d;
            }
        }
    }

    *dist = best_dist;

    return bindex;
}
