/* Rotation guidance algorithms (training, query, validation).
 *
 */

#include "classifier.h"

#define CLASS_DEBUG 0
#define CLASS_FEATURE_DEBUG 1

/* estimate the rotation between two IMU values.
*/
double class_compute_imu_rotation (navlcm_imu_t *p1, navlcm_imu_t *p2)
{
    double *q1 = p1->q;
    double *q2 = p2->q;

    double dir1[3] = { 1, 0, 0};
    double dir2[3] = { 1, 0, 0};

    quat_rotate (q1, dir1);
    quat_rotate (q2, dir2);

    double norm2 = 0.0;
    for (int i=0;i<3;i++)
        norm2 += pow(dir2[i],2);
    norm2 = sqrt (norm2);
    double norm1 = 0.0;
    for (int i=0;i<3;i++)
        norm1 += pow(dir1[i],2);
    norm1 = sqrt (norm1);

    double dot = 0.0;
    for (int i=0;i<3;i++)
        dot += dir1[i] * dir2[i] / (norm1 * norm2);

    return acos (dot);

}

/* estimate the rotation between two pose values.
*/
double class_compute_pose_rotation (botlcm_pose_t *p1, botlcm_pose_t *p2)
{
    if (!p1 || !p2)
        return -1;

    double dir1[3] = { 1, 0, 0};
    double dir2[3] = { 1, 0, 0};
    quat_rotate (p1->orientation, dir1);
    quat_rotate (p2->orientation, dir2);

    double dotq = 0.0;
    for (int i=0;i<4;i++)
        dotq += p1->orientation[i]*p2->orientation[i];
    //    return 2.0 * acos (fabs (dotq));

    double norm2 = 0.0;
    for (int i=0;i<3;i++)
        norm2 += pow(dir2[i],2);
    norm2 = sqrt (norm2);
    double norm1 = 0.0;
    for (int i=0;i<3;i++)
        norm1 += pow(dir1[i],2);
    norm1 = sqrt (norm1);

    double dot = 0.0;
    for (int i=0;i<3;i++)
        dot += dir1[i] * dir2[i] / (norm1 * norm2);

    double res = acos (dot);
    if (dir1[0]*dir2[1]-dir1[1]*dir2[0]<0)
        res = -res;

    return res;
}

/* col and row are in normalized coord. frame -- i.e. in [0,1)
 */
int CLASS_BUCKET_INDEX (double col, double row, int sensorid, int nsensors, int nbuckets)
{
    int res = sensorid * nbuckets * nbuckets + (int)(row * nbuckets) * nbuckets + \
        (int)(col * nbuckets);

    if (nbuckets == 1)
        assert (res == sensorid);
}

/* rotation guidance algorithm: estimate the user rotation given two sets of features and a match matrix (calib).
*/
int class_orientation (navlcm_feature_list_t *f1, navlcm_feature_list_t *f2, config_t *config, double *angle_rad, double *variance, lcm_t *lcm)
{
    if (!f1 || !f2) 
        return -1;

    if (!config->classcalib)
        return -1;

    GTimer *timer = g_timer_new ();

    double *calib = config->classcalib;
    int nsensors = config->nsensors;
    int nbuckets = config->nbuckets;

    // reset the indices for polygamy filtering
    for (int i=0;i<f1->num;i++)
        f1->el[i].index = i;
    for (int i=0;i<f2->num;i++)
        f2->el[i].index = i;

    dbg (DBG_CLASS, "matching %d features against %d features.", f1->num, f2->num);

    // match features
    navlcm_feature_match_set_t *matches = (navlcm_feature_match_set_t*)malloc(sizeof(navlcm_feature_match_set_t));
    int matching_mode = f1->feature_type == NAVLCM_FEATURES_PARAM_T_FAST ? MATCHING_NCC : MATCHING_DOTPROD;
    find_feature_matches_fast (f1, f2, TRUE, TRUE, TRUE, TRUE, .9, -1, matching_mode, matches);
    
    //navlcm_feature_match_set_t *matches = find_feature_matches_multi (f2, f1, TRUE, TRUE, 5, 0.80, -1.0, -1.0, TRUE);

    if (matches)
        dbg (DBG_CLASS, "[class] orientation: found %d matches.", matches->num);

    // correspondence between sensor match and rotation angle
    // for an ideal rig:
    // double tab[4][4] = { 
    //    { 0.0, PI/2, PI, -PI/2}, 
    //    { -PI/2, 0.0, PI/2, PI}, 
    //    { PI, -PI/2, 0.0, PI/2}, 
    //    { PI/2, PI, -PI/2, 0.0}
    //};
    
   // compute psi-distance
    double psi_d = .0, max_d = 1.0;
    int count=0;
    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *m = (navlcm_feature_match_t*)matches->el + i;
        if (m->num==0) continue;
        psi_d += sqrt (m->dist[0]);
        count++;
    }

    max_d = 1.0;

    int missing = f1->num - count + f2->num - count;

    psi_d = 2 * psi_d;

    psi_d += max_d * missing;

    if (f1->num > 0 || f2->num > 0)
        psi_d /= (f1->num + f2->num);

    psi_d = 1.0 - 2.0 * matches->num / (f1->num + f2->num);

    if (variance) *variance = psi_d;

    // reset output angle
    if (angle_rad) *angle_rad = .0;

    // skip if no matches
    if (!matches || matches->num == 0) {
        g_timer_destroy (timer);
        return -1;
    }

    // proceed to voting
    FILE *gp = NULL;
    double *angles = (double*)malloc(matches->num*sizeof(double));
    double *dist   = (double*)malloc(matches->num*sizeof(double));
    double maxdist = -1E6;
    double mindist = 1E6;

    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *match = matches->el + i;
        assert (match->num>0);
        int sensorid1 = match->src.sensorid;
        int sensorid2 = match->dst[0].sensorid;
        int idx1 = CLASS_BUCKET_INDEX (match->src.col/f1->width, match->src.row/f1->height, 
                sensorid1, nsensors, nbuckets);
        int idx2 = CLASS_BUCKET_INDEX (match->dst[0].col/f2->width, match->dst[0].row/f2->height, 
                sensorid2, nsensors, nbuckets);
        assert (sensorid2 != -1);
        double ang = CLASS_GET_CALIB (idx1, idx2, calib, nbuckets * nbuckets * nsensors);//calib[sensorid1*nsensors+sensorid2];//tab[sensorid1][sensorid2];
        angles[i] = ang;
        dist[i] = match->dist[0];
        maxdist = fmax (maxdist, match->dist[0]);
        mindist = fmin (mindist, match->dist[0]);

        if (gp)
            fprintf (gp, "%.3f\n", ang * 180.0 / PI);
    }
    if (gp)
        fclose (gp);

    // scale between .5 and 1.0
    for (int i=0;i<matches->num;i++)
        dist[i] = .5 + .5 * (1.0 - (dist[i] - mindist) / (maxdist - mindist));

    double a_rad = vect_mean_sincos (angles, matches->num);

#ifdef CLASS_FEATURE_DEBUG
    if (lcm) {
        // publish features for display
        for (int i=0;i<f1->num;i++)
            f1->el[i].uid = 0;
        for (int i=0;i<matches->num;i++) {
            navlcm_feature_match_t *match = matches->el + i;
            if (match->num == 0) continue;
            f1->el[match->src.index].uid = match->dst[0].sensorid+1;
            f1->el[match->src.index].scale -= match->dst[0].scale;
            if (f1->el[match->src.index].scale < 0)
                f1->el[match->src.index].scale = -1.0;
            else
                f1->el[match->src.index].scale = 1.0;
        }
        navlcm_feature_list_t_publish (lcm, "FEATURES_DBG", f1);
    }
#endif

    // destroy matches
    navlcm_feature_match_set_t_destroy (matches);
    free (angles);
    free (dist);

    if (angle_rad) *angle_rad = a_rad;

    dbg (DBG_CLASS, "rotation guidance rate: %.2f Hz", 1.0/g_timer_elapsed (timer, NULL));
    g_timer_destroy (timer);

    return 0;
}



/* estimate user rotation during training. the user makes <nturns> turns
 * and walks at 1m/s @ 2Hz
 */
double class_calibration_dead_reckoning (int delta, int nframes, int nturns)
{
    double angle = nturns * 2.0 * PI * (delta) / nframes;
    return clip_value (angle, -PI, PI, 1E-6);
}

/* the training algorithm takes as input a series of observations (feature_list)
 * and assumes that the user rotated a number of times at roughly constant speed.
 * <hist> is an optional parameters that stores the histograms of angle distributions
 * for each pair of camera (NULL to skip).
 */
int class_calibration_rotation (GQueue *feature_list, config_t *config, GQueue *hist, lcm_t *lcm)
{
    int nvotes = 0;
    int step = 2;
    int nframes = 0;
    double mean_secs = .0;
    int o_start, o_end, o_count;

    // find start and end of calibration sequence
    class_calibration_find_start_end (feature_list, &o_start, &o_end, &o_count);
    if (o_start == -1 || o_end == -1 || o_count == -1) {
        dbg (DBG_ERROR, "failed to determine start/end/count: %d/%d/%d", o_start, o_end, o_count);
        return -1;
    }

    nframes = o_end - o_start + 1;
    int nsensors = config->nsensors;
    int nbuckets = config->nbuckets;
    int size = nsensors * nbuckets * nbuckets;

    // alloc memory
    double *sinangle = (double*)malloc(size*size*sizeof(double));
    double *cosangle = (double*)malloc(size*size*sizeof(double));
    double *tanangle = (double*)malloc(size*size*sizeof(double));
    int    *votesnum = (int*)malloc(size*size*sizeof(int));
    FILE **fps = (FILE**)calloc (nsensors*nsensors, sizeof(FILE*));

    for (int i=0;i<size*size;i++) {
        sinangle[i] = .0;
        cosangle[i] = .0;
        tanangle[i] = .0;
        votesnum[i] = .0;
    }
    for (int i=0;i<nsensors;i++) {
        for (int j=0;j<nsensors;j++) {
            char filename[128];
            sprintf (filename, "training-raw-%d-%d.txt", i, j);
            fps[i*nsensors+j] = fopen (filename, "w");
        }
    }

    int total = (nframes * (nframes - 1)) / (2 * step * step);

    // for each pair of frames, estimate the user's ego-rotation 
    // and compute feature matches
    int count1 = nframes-1, count2 = 0, nrun = 0;

    GList *iter1 = g_queue_peek_tail_link (feature_list);
    while (iter1) {
        GList *iter2 = iter1->next;
        count2 = count1 + 1;
        if (o_start <= count1 && count1 <= o_end) {
            while (iter2) {
                if (o_start <= count2 && count2 <= o_end) {
                    navlcm_feature_list_t *keys1 = (navlcm_feature_list_t*)iter1->data;
                    navlcm_feature_list_t *keys2 = (navlcm_feature_list_t*)iter2->data;
                    assert (keys1 && keys2);

                    navlcm_feature_list_t *set1 = keys1;
                    navlcm_feature_list_t *set2 = keys2;

                    if (set1->num == 0 || set2->num == 0) {
                        dbg (DBG_ERROR, "Warning: empty feature sets for frames %d and %d.", count1, count2);
                        continue;
                    }

                    GTimer *timer = g_timer_new ();

                    double angle = 0.0;

                    // constant rotation speed assumption
                    angle = class_calibration_dead_reckoning (count2-count1, nframes, o_count);

                    // match features
                    navlcm_feature_match_set_t *matches = (navlcm_feature_match_set_t*)malloc(sizeof(navlcm_feature_match_set_t));
                    int matching_mode = set1->feature_type == NAVLCM_FEATURES_PARAM_T_FAST ? MATCHING_NCC : MATCHING_DOTPROD;
                    find_feature_matches_fast (set2, set1, TRUE, TRUE, TRUE, TRUE, .8, -1, matching_mode, matches);


                    if (!matches || matches->num == 0) {
                        dbg (DBG_ERROR, "Warning: no matches found between frame %d and frame %d", count1, count2);
                        continue;
                    }

                    // for each match, populate the classifier table
                    for (int i=0;i<matches->num;i++) {
                        navlcm_feature_match_t *match = matches->el + i;
                        if (match->num == 0) continue;
                        int sensorid1 = match->src.sensorid;
                        int sensorid2 = match->dst[0].sensorid;
                        int idx1 = CLASS_BUCKET_INDEX (match->src.col/set1->width, 
                                match->src.row/set1->height, sensorid1, nsensors, nbuckets);
                        int idx2 = CLASS_BUCKET_INDEX (match->dst[0].col/set2->width, 
                                match->dst[0].row/set2->height, sensorid2, nsensors, nbuckets);
                        if (sensorid1 < 0 || nsensors <= sensorid1) {
                            dbg (DBG_ERROR, "Warning: sensorid out of bound %d", sensorid1);
                            continue;
                        }
                        if (sensorid2 < 0 || nsensors <= sensorid2) {
                            dbg (DBG_ERROR, "Warning: sensorid out of bound %d", sensorid2);
                            continue;
                        }
                        CLASS_SET_CALIB (idx1, idx2, sinangle, size, CLASS_GET_CALIB (idx1, idx2, sinangle, size) + sin(angle));
                        CLASS_SET_CALIB (idx2, idx1, sinangle, size, CLASS_GET_CALIB (idx2, idx1, sinangle, size) - sin(angle));
                        CLASS_SET_CALIB (idx1, idx2, cosangle, size, CLASS_GET_CALIB (idx1, idx2, cosangle, size) + cos(angle));
                        CLASS_SET_CALIB (idx2, idx1, cosangle, size, CLASS_GET_CALIB (idx2, idx1, cosangle, size) + cos(angle));
                        CLASS_SET_CALIB (idx1, idx2, votesnum, size, CLASS_GET_CALIB (idx1, idx2, votesnum, size) + 1);
                        CLASS_SET_CALIB (idx2, idx1, votesnum, size, CLASS_GET_CALIB (idx2, idx1, votesnum, size) + 1);
                        nvotes+=2;
                    }

                    // save raw data to files
                    for (int i=0;i<matches->num;i++) {
                        navlcm_feature_match_t *match = matches->el + i;
                        if (match->num == 0) continue;
                        int sensorid1 = match->src.sensorid;
                        int sensorid2 = match->dst[0].sensorid;
                        fprintf (fps[sensorid1 * nsensors + sensorid2], "%.4f\n", clip_value (angle, -M_PI, M_PI, 1E-6)); 
                    }

                    // optional: for each match, populate the histograms
                    if (hist) {
                        for (int i=0;i<matches->num;i++) {
                            navlcm_feature_match_t *match = matches->el + i;
                            if (match->num == 0) continue;
                            int sensorid1 = match->src.sensorid;
                            int sensorid2 = match->dst[0].sensorid;
                            classifier_hist_t *c12 = (classifier_hist_t*)g_queue_peek_nth (hist, sensorid1 * size + sensorid2);
                            classifier_hist_t *c21 = (classifier_hist_t*)g_queue_peek_nth (hist, sensorid2 * size + sensorid1);
                            classifier_hist_insert (c12, clip_value (angle, -M_PI, M_PI, 1E-6));
                            classifier_hist_insert (c21, clip_value (-angle, -M_PI, M_PI, 1E-6));
                        }
                        // publish each histogram as a dictionary
                        for (int i=0;i<size;i++) {
                            for (int j=0;j<size;j++) {
                                classifier_hist_t *c = (classifier_hist_t*)g_queue_peek_nth (hist, i * size + j);
                                navlcm_dictionary_t *d = classifier_hist_to_dictionary (c);
                                char channel[40];
                                sprintf (channel, "CLASS_HIST_%d_%d", i, j);
                                navlcm_dictionary_t_publish (lcm, channel, d);
                                navlcm_dictionary_t_destroy (d);
                            }
                        }
                    }

                    navlcm_feature_match_set_t_destroy (matches);

                    // elapsed time
                    double secs = g_timer_elapsed (timer, NULL);
                    g_timer_destroy (timer);
                    mean_secs = mean_secs * 1.0 * nrun / (nrun+1) + secs * 1.0 / (nrun+1);

                    // estimated time remaining
                    double rem_secs = mean_secs * (total - nrun);
                    double rem_mins = rem_secs / 60;
                    double rem_hours = rem_mins / 60;

                    nrun++;
                    dbg (DBG_CLASS, "rotation calibration [%d/%d/%d] progress = %.2f %%, time to finish: %.1f hours (or %.1f mins, or %.1f secs)", o_start, o_end, o_count, 100.0 * nrun / total, rem_hours, rem_mins, rem_secs);
                }

                for (int i=0;i<step;i++) {
                    iter2 = iter2->next;
                    count2++;
                    if (!iter2) break;
                }
            }
        }

        for (int i=0;i<step;i++) {
            iter1 = iter1->prev;
            count1--;
            if (!iter1) break;
        }
    }

    // close files containing raw data
    for (int i=0;i<nsensors;i++) {
        for (int j=0;j<nsensors;j++) {
            fclose (fps[i*nsensors+j]);
        }
    }

    // save histograms to text file (for plotting)
    if (hist) {
        for (int i=0;i<size;i++) {
            for (int j=0;j<size;j++) {
                classifier_hist_t *c = (classifier_hist_t*)g_queue_peek_nth (hist, i * size + j);
                char filename[40];
                sprintf (filename, "training_histogram_%d_%d.txt", i, j);
                classifier_hist_write_to_file (c, filename);
            }
        }
    }

    dbg (DBG_CLASS, "========= %d votes ==========", nvotes);

    // compute classifier
    for (int i=0;i<size*size;i++) {
        tanangle[i] = atan2 (sinangle[i], cosangle[i]);
    }

    // print out classifier in degrees
    //
    for (int i=0;i<size;i++) {
        for (int j=0;j<size;j++) {
            printf ("[%d] %.2f ", CLASS_GET_CALIB (i, j, votesnum, size), CLASS_GET_CALIB (i, j, tanangle, size)*180.0/M_PI);
        }
        printf ("\n");
    }

    // apply to config
    if (config->classcalib)
        free (config->classcalib);

    config->classcalib = tanangle;

    // save to config file
    class_write_calib (config, class_next_calib_filename ());

    // free data
    free (sinangle);
    free (cosangle);
    free (votesnum);

    return 0;
}

/* automatically determine the start and end of a calibration sequence
 * by looking at the distance between features in image space
 */
void class_calibration_find_start_end (GQueue *feature_list, int *o_start, int *o_end, int *o_count)
{
    *o_start = *o_end = *o_count = -1;

    if (!feature_list) return;
    if (g_queue_get_length (feature_list) < 2) return;

    navlcm_feature_list_t *first = (navlcm_feature_list_t*)g_queue_peek_tail (feature_list);

    int num = g_queue_get_length (feature_list);
    int count=0;
    double *vals = (double*)malloc(num*sizeof(double));

    // compute the average distance between feature matches in image space
    // between the first frame and all other frames
    for (GList *iter=g_queue_peek_tail_link(feature_list);iter;iter=iter->prev) {
        navlcm_feature_list_t *curr = (navlcm_feature_list_t*)iter->data;

        // compute feature matches
        navlcm_feature_match_set_t *matches = (navlcm_feature_match_set_t*)malloc(sizeof(navlcm_feature_match_set_t));
        int matching_mode = first->feature_type == NAVLCM_FEATURES_PARAM_T_FAST ? MATCHING_NCC : MATCHING_DOTPROD;
        find_feature_matches_fast (first, curr, TRUE, TRUE, TRUE, TRUE, .8, -1, matching_mode, matches);

        // compute average feature distance
        double dist = .0;
        for (int i=0;i<matches->num;i++) {
            navlcm_feature_t f1 = matches->el[i].src;
            navlcm_feature_t f2 = matches->el[i].dst[0];
            if (f1.sensorid != f2.sensorid)
                dist += pow (curr->width, 2) + pow (curr->height, 2);
            else
                dist += pow (f1.col - f2.col, 2) + pow (f1.row - f2.row, 2);
        }
        dist = sqrt (dist / matches->num);

        vals[count] = dist;
        count++;

        navlcm_feature_match_set_t_destroy (matches);
    }

    // smooth
    double *svals = math_smooth_double (vals, num, 9);

    // save to file
    FILE *fp = fopen ("calib_data.txt", "w");
    if (fp) {
        for (int i=0;i<num-2;i++)
            fprintf (fp, "%d %.5f\n", i, svals[i]);
        fclose (fp);
    }

    // count local minima
    int last_minima = -1;
    int minima_count = 0;
    int window_size = 31;
    int mindist = 30; // min distance between two minima
    for (int i=mindist;i<num;i++) {
        gboolean is_minima = true;
        for (int j=0;j<=window_size;j++) {
            if (window_size / 2 <= i + j && i + j < num + window_size/2 && j != window_size/2) {
                if (svals[i+j-window_size/2] < svals[i]) {
                    is_minima = false;
                    break;
                }
            }
        }
        if (is_minima && (last_minima == -1 || i - last_minima > mindist)) {
            // compute magnitude
            double mag = .0;
            for (int j=0;j<=window_size;j++) {
                if (window_size / 2 <= i + j && i + j < num + window_size/2 && j != window_size/2) {
                    mag = fmax (mag, svals[i+j-window_size/2]-svals[i]);
                }
            }
            if (mag > 50.0) {
                last_minima = i;
                minima_count++;
            }
        }
    }

    dbg (DBG_CLASS, "found %d minima, last minima at %d\n", minima_count, last_minima);

    free (vals);
    free (svals);

    *o_count = minima_count;
    *o_end = last_minima;
    *o_start = 0;
}

/* vertical calibration consists in computing the average Y change over the sequence
*/
int class_calibration_vertical (GQueue *feature_list, config_t *config, int sign)
{
    int step = 2;
    int nframes = g_queue_get_length (feature_list);
    double mean_secs = .0;
    int window = 50;

    int total = (nframes * window) / (2 * step * step);

    // for each pair of frames, estimate the Y delta
    // and compute feature matches
    int count1 = 0, count2 = 0, nrun = 0;

    FILE *fp = fopen ("calibration-vert-data.txt", "w");

    GList *iter1 = g_queue_peek_head_link (feature_list);
    while (iter1) {
        GList *iter2 = iter1->next;
        count2 = count1 + 1;
        while (iter2) {
            navlcm_feature_list_t *keys1 = (navlcm_feature_list_t*)iter1->data;
            navlcm_feature_list_t *keys2 = (navlcm_feature_list_t*)iter2->data;
            assert (keys1 && keys2);

            navlcm_feature_list_t *set1 = keys1;
            navlcm_feature_list_t *set2 = keys2;

            if (set1->num == 0 || set2->num == 0) {
                dbg (DBG_ERROR, "Warning: empty feature sets for frames %d and %d.", count1, count2);
                continue;
            }

            GTimer *timer = g_timer_new ();

            // match features
            navlcm_feature_match_set_t *matches = (navlcm_feature_match_set_t*)malloc(sizeof(navlcm_feature_match_set_t));
            int matching_mode = set1->feature_type == NAVLCM_FEATURES_PARAM_T_FAST ? MATCHING_NCC : MATCHING_DOTPROD;
            find_feature_matches_fast (set2, set1, TRUE, FALSE, TRUE, TRUE, .8, -1,  matching_mode, matches);


            if (!matches || matches->num == 0) {
                dbg (DBG_ERROR, "Warning: no matches found between frame %d and frame %d", count1, count2);
                continue;
            }

            // compute average Y delta
            double avgy = .0;
            for (int i=0;i<matches->num;i++) {
                navlcm_feature_match_t *m = matches->el + i;
                avgy += 1.0 * (m->dst[0].col - m->src.col);
            }
            avgy *= sign * 1.0 / matches->num;

            fprintf (fp, "%d %.3f %d\n", abs(count2-count1), avgy, matches->num);

            navlcm_feature_match_set_t_destroy (matches);
            // elapsed time
            double secs = g_timer_elapsed (timer, NULL);
            g_timer_destroy (timer);
            mean_secs = mean_secs * 1.0 * nrun / (nrun+1) + secs * 1.0 / (nrun+1);

            // estimated time remaining
            double rem_secs = mean_secs * (total - nrun);
            double rem_mins = rem_secs / 60;
            double rem_hours = rem_mins / 60;

            nrun++;
            dbg (DBG_CLASS, "vertical %d calibration progress = %.2f %%, time to finish: %.1f hours (or %.1f mins, or %.1f secs)", sign, 100.0 * nrun / total, rem_hours, rem_mins, rem_secs);

            for (int i=0;i<step;i++) {
                iter2 = iter2->next;
                count2++;
                if (!iter2) break;
            }
            if (count2 - count1 > window)
                break;
        }

        for (int i=0;i<step;i++) {
            iter1 = iter1->next;
            count1++;
            if (!iter1) break;
        }
    }

    fclose (fp);
}

/* main entry point for calibration.
 * step = 1: rotation calibration
 * step = 2: up calibration
 * step = 3: down calibration
 */
int class_calibration (GQueue *feature_list, config_t *config, int step)
{
    if (step == 1) {
        return class_calibration_rotation (feature_list, config, NULL, NULL);
    }

    if (step > 1) {
        return class_calibration_vertical (feature_list, config, step == 2 ? 1 : -1);
    }
}

/* the IMU validation algorithm takes as input a series of observations (feature_list) and
 * assumes that the user rotates in an arbitrary environment at roughly constant speed.
 * the algorithm also runs the rotation baseline algorithm if images are available (upward_image_queue).
 */
void class_imu_validation (GQueue *feature_list, GQueue *upward_image_queue, GQueue *pose_queue, config_t *config, char *filename)
{
    int nframes=0;
    int step = 2;

    FILE *fp = fopen (filename, "w");
    fclose (fp);

    // check number of frames
    nframes = g_queue_get_length (feature_list);
    if (nframes <= 1) {
        dbg (DBG_CLASS, "not enough frames to process.");
        return;
    }

    int total = (nframes * (nframes - 1)) / (2 * step * step);
    double mean_secs = .0;

    // for each pair of frames, estimate the user's ego-rotation 
    // and compute feature matches
    int count1 = 0, count2 = 0, nrun = 0;
    int nfeatures = 0, nnfeatures = 0;

    GList *iter1 = g_queue_peek_head_link (feature_list);
    while (iter1) {
        GList *iter2 = iter1->next;
        count2 = count1 + 1;

        navlcm_feature_list_t *keys1 = (navlcm_feature_list_t*)iter1->data;

        nfeatures += keys1->num;
        nnfeatures += 1;

        while (iter2) {

            navlcm_feature_list_t *keys2 = (navlcm_feature_list_t*)iter2->data;
            assert (keys1 && keys2);

            navlcm_feature_list_t *set1 = keys1;
            navlcm_feature_list_t *set2 = keys2;

            if (set1->num == 0 || set2->num == 0) {
                dbg (DBG_ERROR, "Warning: empty SIFT sets for frames %d and %d.", count1, count2);
                continue;
            }

            int64_t delta_usecs = keys1->utime > keys2->utime ? keys1->utime - keys2->utime : keys2->utime - keys1->utime;
            if (delta_usecs < 30 * 1000000) {

                GTimer *timer = g_timer_new ();

                double angle = 0.0, cangle = .0;

                if (class_orientation (set1, set2, config, &cangle, NULL, NULL)==0) {

                    // get ground truth from IMU
                    botlcm_pose_t *p1 = find_pose_by_utime (pose_queue, keys1->utime);
                    botlcm_pose_t *p2 = find_pose_by_utime (pose_queue, keys2->utime);
                    if (p1 && p2) {
                        angle = class_compute_pose_rotation (p1, p2);

                        int nf = (set1->num+set2->num/2);

                        fp = fopen (filename, "a");
                        fprintf (fp, "%d %d %d %.3f ", count1, count2, nf, angle * 180.0 / PI);
                        fclose (fp);
                        dbg (DBG_CLASS, "IMU rotation: %.4f deg.", angle * 180.0 / PI);

                        // estimate rotation from classifier
                        fp = fopen (filename, "a");
                        double err = clip_value(angle - cangle, -PI, PI, 1E-6)*180/PI;
                        fprintf (fp, "%.3f %.3f\n", cangle * 180.0 / PI, err);
                        fclose (fp);
                        dbg (DBG_CLASS, "classifier rotation: %.4f deg.", cangle *180/PI);
                    }
                }

                double secs = g_timer_elapsed (timer, NULL);
                g_timer_destroy (timer);
                mean_secs = mean_secs * 1.0 * nrun / (nrun+1) + secs * 1.0 / (nrun+1);

                // estimated time remaining
                double rem_secs = mean_secs * (total - nrun);
                double rem_mins = rem_secs / 60;
                double rem_hours = rem_mins / 60;

                nrun++;
                dbg (DBG_CLASS, "progress = %.2f %%, time to finish: %.1f hours (or %.1f mins, or %.1f secs)", 
                        100.0 * nrun / total, rem_hours, rem_mins, rem_secs);
            }

            for (int i=0;i<step;i++) {
                iter2 = iter2->next;
                count2++;
                if (!iter2) break;
            }

        }
        for (int i=0;i<step;i++) {
            iter1 = iter1->next;
            count1++;
            if (!iter1) break;
        }
    }

    dbg (DBG_CLASS, "processed %d frames (%d runs). Average # features: %d", nframes, total, (int)(1.0*nfeatures/nnfeatures));

    // rotation baseline versus IMU
    if (upward_image_queue) {
        for (GList *iter1 = g_queue_peek_head_link (upward_image_queue);iter1;iter1=g_list_next(iter1)) {
            for (GList *iter2 = g_queue_peek_head_link (upward_image_queue);iter2;iter2=g_list_next(iter2)) {
                if (iter1 == iter2) continue;
                botlcm_image_t *img1 = (botlcm_image_t*)iter1->data;
                botlcm_image_t *img2 = (botlcm_image_t*)iter2->data;

                // compute imu ground truth
                botlcm_pose_t *p1 = find_pose_by_utime (pose_queue, img1->utime);
                botlcm_pose_t *p2 = find_pose_by_utime (pose_queue, img2->utime);

                double angle=0.0;
                if (p1 && p2) {
                    angle = class_compute_pose_rotation (p2, p1);
                    FILE *fp = fopen ("upward-vs-imu.txt", "a");
                    fprintf (fp, "%.3f ", angle*180/PI);
                    fclose (fp);
                }

                // compute upward image rotation
                double angle_rad, stdev;
                if (rot_estimate_angle_from_images (img1, img2, &angle_rad, &stdev)==0) {
                    FILE *fp = fopen ("upward-vs-imu.txt", "a");
                    double err = diff_angle_plus_minus_pi (angle, angle_rad)*180/PI;
                    fprintf (fp, "%.3f %.3f\n", angle_rad*180/PI, err);
                    fclose (fp);
                }
            }
        }
    }
}

/* convert lr3 camera orientations to correspondence matrix.
 * this code was used to test our algorithms on the MIT Darpa Urban Challenge datasets.
 */
void lr3_calib_to_matrix ()
{
    double camera_0[4] = {0.598504,-0.772675,0.176464,-0.116734}; // RFL
    double camera_1[4] = {0.49910037152507131, -0.50998100140449931, 0.51356391936558043, -0.47651951062979958}; //RFC
    double camera_2[4] = {0.123769,-0.183141,0.770141,-0.598351}; // RFR
    double camera_3[4] = {0.5096029747,-0.490046465,-0.4884064966,0.5114864264};//RRC

    // initialize quaternion
    double q[4][4];
    for (int i=0;i<4;i++) {
        q[0][i] = camera_0[i];
        q[1][i] = camera_1[i];
        q[2][i] = camera_2[i];
        q[3][i] = camera_3[i];
    }

    // compute rotation between pairs of quaternions
    dbg (DBG_CLASS, "lr3 correspondence matrix");
    for (int i=0;i<4;i++) {
        for (int j=0;j<4;j++) {
            double a = quat_angle (q[i], q[j]);
            if (i==j) a = 0.0;
            printf ("%.5f ", a);
            //dbg (DBG_CLASS, "[%d][%d] %.4f %.3f deg.", i, j, a, a * 180.0/PI);
        }
        printf ("\n");
    }
}

/* returns the differential heading between time1 and time2, in [-180,180]
*/
int applanix_delta (GQueue *data, int64_t utime1, int64_t utime2, double *delta_deg)
{
    assert (utime2 > utime1);

    nav_applanix_data_t *d1 = find_applanix_data (data, utime1);
    nav_applanix_data_t *d2 = find_applanix_data (data, utime2);

    if (!d1 || !d2) return -1;

    double a = d2->heading - d1->heading;
    double r = a * PI / 180.0;
    r = clip_value (r, -PI, PI, 1E-6);

    *delta_deg = r * 180.0/PI;

    return 0;
}

/* compute the psi-distance between two feature sets
*/
double class_psi_distance (navlcm_feature_list_t *f1, navlcm_feature_list_t *f2, lcm_t *lcm)
{
    double max_d = .0;
    double psi_d = .0;
    int missing = 0;
    int count = 0;

    if (f1->num == 0 || f2->num == 0) {
        dbg (DBG_ERROR, "matching zero features.");
        return .0;
    }

    // compute feature matches
    navlcm_feature_match_set_t *matches = (navlcm_feature_match_set_t*)malloc(sizeof(navlcm_feature_match_set_t));
    int matching_mode = f1->feature_type == NAVLCM_FEATURES_PARAM_T_FAST ? MATCHING_NCC : MATCHING_DOTPROD;
    find_feature_matches_fast (f1, f2, TRUE, TRUE, TRUE, TRUE, .8, -1, matching_mode, matches);

    if (!matches) 
        return .0;

    if (matches->num == 0) {
        dbg (DBG_ERROR, "warning: no matches.");
        return .0;
    } else {
        dbg (DBG_CLASS, "%d/%d --> %d matches.", f1->num, f2->num, matches->num);
    }

    FILE *fp = fopen ("matching-rate.txt", "a");
    fprintf (fp, "%.4f\n", 100.0 * (matches->num) / ((f1->num + f2->num)/2));
    fclose (fp);

    double r1 = 1.0 * matches->num / f1->num;
    double r2 = 1.0 * matches->num / f2->num;

    if (lcm) {
        for (int i=0;i<f1->num;i++) {
            f1->el[i].uid = 0;
            f1->el[i].index = i;
        }
    }

    // compute psi-distance
    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *m = (navlcm_feature_match_t*)matches->el + i;
        if (m->num==0) continue;
        f1->el[m->src.index].uid=1;
        psi_d += sqrt (m->dist[0]);
        count++;
        max_d = fmax (max_d, m->dist[0]);
    }
    assert (max_d < 1.001);

    if (lcm)
        navlcm_feature_list_t_publish (lcm, "FEATURES_DBG", f1);

    max_d = 1.0;//*= 2.0;

    missing = f1->num - count + f2->num - count;

    psi_d = 2 * psi_d;

    psi_d += max_d * missing;

    if (f1->num > 0 || f2->num > 0)
        psi_d /= (f1->num + f2->num);

    psi_d = psi_d;

    // using just matching ratio
    //psi_d = 1.0 - 1.0 * matches->num / f1->num;

    // free matches
    navlcm_feature_match_set_t_destroy (matches);

    return psi_d;
}

/* compute the phi-distance between two feature sets
*/
double class_phi_distance (navlcm_feature_list_t *f1, navlcm_feature_list_t *f2)
{
    double max_d = .0;
    double phi_d = .0;
    int missing = 0;
    int count = 0;

    // compute feature matches
    navlcm_feature_match_set_t *matches = (navlcm_feature_match_set_t*)malloc(sizeof(navlcm_feature_match_set_t));
    int matching_mode = f1->feature_type == NAVLCM_FEATURES_PARAM_T_FAST ? MATCHING_NCC : MATCHING_DOTPROD;
    find_feature_matches_fast (f1, f2, TRUE, TRUE, TRUE, TRUE, .8, -1, matching_mode, matches);

    if (!matches) 
        return .0;

    double *vals = NULL;

    // compute psi-distance
    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *mi = (navlcm_feature_match_t*)matches->el + i;
        for (int j=i+1;j<matches->num;j++) {
            navlcm_feature_match_t *mj = (navlcm_feature_match_t*)matches->el + j;
            if (mi->src.sensorid != mj->src.sensorid) continue;
            if (mi->dst[0].sensorid != mj->dst[0].sensorid) continue;
            double d1 = sqrt (powf (mi->src.col - mj->src.col, 2) + powf (mi->src.row - mj->src.row, 2));
            double d2 = sqrt (powf (mi->dst[0].col - mj->dst[0].col, 2) + powf (mi->dst[0].row - mj->dst[0].row, 2));
            vals = (double*)realloc(vals, (count+1)*sizeof(double));
            vals[count] = fabs (d1-d2);
            count++;
        }
    }

    phi_d = math_quick_select (vals, count);

    // save to file
    FILE *fp = fopen ("phi.txt", "a");
    fprintf (fp, "%d %.4f\n", missing, phi_d);
    fclose (fp);

    free (vals);

    return phi_d;
}

int class_read_calib (config_t *cfg)
{
    FILE *fp = fopen (cfg->classcalib_filename, "r");
    if (!fp) {
        dbg (DBG_ERROR, "failed to open calibration file %s", cfg->classcalib_filename);
        return 1;
    }

    int nsensors;
    if (fscanf (fp, "%d", &nsensors) != 1) {
        dbg (DBG_ERROR, "error reading calibration file %s", cfg->classcalib_filename);
        return 1;
    }

    assert (nsensors == cfg->nsensors);

    int nbuckets;
    if (fscanf (fp, "%d", &nbuckets) != 1) {
        dbg (DBG_ERROR, "error reading calibration file %s", cfg->classcalib_filename);
        return 1;
    }

    assert (nbuckets == cfg->nbuckets);

    int size = nsensors * nbuckets * nbuckets;

    cfg->classcalib = (double*)malloc(size*size*sizeof(double));

    for (int i=0;i<size*size;i++) {
        double val;
        if (fscanf (fp, "%lf", &val) != 1) {
            dbg (DBG_ERROR, "error reading calibration file %s", cfg->classcalib_filename);
            return 1;
        }
        cfg->classcalib[i] = val;
    }

    fclose (fp);

    dbg (DBG_CLASS, "read calibration from %s (nsensors = %d, nbuckets = %d, size = %d)", cfg->classcalib_filename, nsensors, nbuckets, size);

    return 0;
}

int class_write_calib (config_t *cfg, const char *filename)
{
    FILE *fp = fopen (filename, "w");

    if (!fp) {
        dbg (DBG_ERROR, "failed to write calib to file %s", filename);
        return 1;
    }

    fprintf (fp, "%d\n", cfg->nsensors);
    fprintf (fp, "%d\n", cfg->nbuckets);

    int size = cfg->nsensors * cfg->nbuckets * cfg->nbuckets;

    for (int i=0;i<size*size;i++) {
        fprintf (fp, "%.9f ", cfg->classcalib[i]);
        if ((i+1)%cfg->nsensors==0)
            fprintf (fp, "\n");
    }

    fclose (fp);

    dbg (DBG_CLASS, "wrote calibration to %s", filename);

    return 0;
}

char *class_next_calib_filename ()
{
    char *filename = (char*)malloc(256);

    int count=0;

    sprintf (filename, "class-calib-%04d-%02d-%02d-%02d.txt", get_current_year(), get_current_month(), get_current_day (), count);

    while (file_exists (filename)) {
        count++;
        sprintf (filename, "class-calib-%04d-%02d-%02d-%02d.txt", get_current_year(), get_current_month(), get_current_day (), count);
    }

    return filename;
}

char *class_next_map_filename ()
{
    char *filename = (char*)malloc(256);

    sprintf (filename, "map-%04d-%02d-%02d.%02d-%02d-%02d.bin", get_current_year(), get_current_month(), get_current_day (), 
            get_current_hour(), get_current_min (), get_current_sec());

    return filename;
}

motion_classifier_t *motion_classifier_init (int size)
{
    motion_classifier_t *m = (motion_classifier_t*)calloc (1, sizeof(motion_classifier_t));
    m->size = size;
    m->pdf = (double*)calloc(size, sizeof(double));
    for (int i=0;i<size;i++) {
        m->pdf[i] = 1.0/size;
    }

    m->hist_size = 100;
    m->motion1 = (int*)calloc(m->hist_size, sizeof(int));
    m->motion2 = (int*)calloc(m->hist_size, sizeof(int));

    for (int i=0;i<m->hist_size;i++) {
        m->motion1[i] = -1;
        m->motion2[i] = -1;
    }

    return m;
}

static double MOTION_CLASSIFIFIER_ALPHA = .98;

void motion_classifier_update (motion_classifier_t *mc, double *prob)
{
    double *c = (double*)calloc(mc->size,sizeof(double));

    for (int i=0;i<mc->size;i++)
        c[i] = .0;

    // transition model
    for (int i=0;i<mc->size;i++) {
        for (int j=0;j<mc->size;j++) {
            c[i] += i == j ? MOTION_CLASSIFIFIER_ALPHA * mc->pdf[i] : (1.0 - MOTION_CLASSIFIFIER_ALPHA) * mc->pdf[j];
        }
    }

    // observation model
    for (int i=0;i<mc->size;i++) {
        c[i] *= .5 + prob[i];
    }

    // normalize
    double norm = .0;
    for (int i=0;i<mc->size;i++)
        norm += c[i];

    if (norm > 1E-5) {
        for (int i=0;i<mc->size;i++) {
            mc->pdf[i] = c[i] / norm;
        }
    }

    free (c);
}

void motion_classifier_write_top_two_motions (motion_classifier_t *mc, const char *filename, int64_t utime)
{
    FILE *fp = fopen (filename, "a");
    if (fp) {
        int motion1, motion2;
        motion_classifier_extract_top_two_motions  (mc, &motion1, &motion2); 
        fwrite (&utime, sizeof(int64_t), 1, fp);
        fwrite (&motion1, sizeof(int), 1, fp);
        fwrite (&motion2, sizeof(int), 1, fp);
        fclose(fp);
    }
}

void motion_classifier_extract_top_two_motions (motion_classifier_t *mc, int *motion1, int *motion2)
{
    double maxpdf[2] = {.0, .0};
    int maxpdfindex[2] = {-1, -1};

    for (int i=0;i<mc->size;i++) {
        if (maxpdfindex[0] == -1 || maxpdf[0] < mc->pdf[i]) {
            maxpdf[1] = maxpdf[0];
            maxpdfindex[1] = maxpdfindex[0];
            maxpdf[0] = mc->pdf[i];
            maxpdfindex[0] = i;
            continue;
        }
        if (maxpdfindex[1] == -1 || maxpdf[1] < mc->pdf[i]) {
            maxpdf[1] = mc->pdf[i];
            maxpdfindex[1] = i;
        }
    }

    // separation test
    for (int i=0;i<mc->size;i++) {
        if (i != maxpdfindex[0] && i != maxpdfindex[1] && maxpdf[0] * .7 < mc->pdf[i])
            maxpdfindex[0] = -1;
        if (i != maxpdfindex[0] && i != maxpdfindex[1] && maxpdf[1] * .7 < mc->pdf[i])
            maxpdfindex[1] = -1;
    }

    *motion1 = maxpdfindex[0];
    *motion2 = maxpdfindex[1];
}

void motion_classifier_print (motion_classifier_t *mc)
{
    for (int i=0;i<mc->hist_size;i++)
        printf ("%d ", mc->motion1[i]);
    printf ("\n");
}

void motion_classifier_update_history (motion_classifier_t *mc, int motion1, int motion2)
{
    for (int i=mc->hist_size-1;i>0;i--) {
        mc->motion1[i] = mc->motion1[i-1];
        mc->motion2[i] = mc->motion2[i-1];
    }

    mc->motion1[0] = motion1;
    mc->motion2[0] = motion2;

}

int motion_classifier_histogram_voting (motion_classifier_t *mc, int startindex)
{
    int hsize = 10;
    int *hist = (int*)calloc(hsize,sizeof(int));

    for (int i=startindex;i<mc->hist_size;i++) {
        if (mc->motion1[i] != -1 && mc->motion1[i] < 10)
            hist[mc->motion1[i]]++;
    }

    // find extremum
    int maxindex = -1;
    int maxval = 0;
    for (int i=0;i<hsize;i++) {
        if (maxindex == -1 || maxval < hist[i]) {
            maxval = hist[i];
            maxindex = i;
        }
    }

    return maxindex;

    free (hist);
}

void motion_classifier_clear_history (motion_classifier_t *mc, int startindex)
{
    for (int i=startindex;i<mc->hist_size;i++) {
        mc->motion1[i] = -1;
        mc->motion2[i] = -1;
    }
}

void class_features_signature (navlcm_feature_list_t *f)
{
    double avcol = .0, avrow = .0;

    for (int i=0;i<f->num;i++) {
        avcol += f->el[i].col;
        avrow += f->el[i].row;
    }

    if (f->num > 0) {
        avcol /= f->num;
        avrow /= f->num;
    }

    printf ("features signature %ld: %.4f %.4f\n", f->utime, avcol, avrow);

}

static double *g_angle_diff = NULL;

double class_guidance_derivative_sliding_window (double angle, int windowsize)
{
    if (!g_angle_diff) 
        g_angle_diff = (double*)calloc (windowsize, sizeof(double));

    for (int i=windowsize-1;i>=0;i--)
        g_angle_diff[i] = g_angle_diff[i-1];

    g_angle_diff[0] = angle;

    double mean = .0;
    for (int i=0;i<windowsize-1;i++)
        mean += fabs (g_angle_diff[i]-g_angle_diff[i+1]);

    mean /= windowsize;

    return mean;
}

classifier_hist_t *classifier_hist_create ()
{
    classifier_hist_t *c = (classifier_hist_t*)malloc(sizeof(classifier_hist_t));
    c->nbins = 0;
    c->hist = NULL;
    c->range_min = .0;
    c->range_max = .0;
    return c;
}

classifier_hist_t *classifier_hist_create_with_data (int nbins, double range_min, double range_max)
{
    classifier_hist_t *c = (classifier_hist_t*)malloc(sizeof(classifier_hist_t));
    c->nbins = nbins;
    c->hist = (int*)calloc(nbins, sizeof(int));
    c->range_min = range_min;
    c->range_max = range_max;
    return c;
}

void classifier_destroy (gpointer data)
{
    classifier_hist_t *c = (classifier_hist_t*)data;
    free (c->hist);
}

void classifier_hist_insert (classifier_hist_t *c, double val)
{
    int ind = MAX (0, MIN (c->nbins, (int)(1.0 * c->nbins * (val - c->range_min) / (c->range_max - c->range_min))));
    c->hist[ind]++;
}

int classifier_hist_sum (classifier_hist_t *c)
{
    int sum=0;
    for (int i=0;i<c->nbins;i++)
        sum += c->hist[i];
    return sum;
}

void classifier_hist_write_to_file (classifier_hist_t *c, char *filename)
{
    FILE *fp = fopen (filename, "w");

    int hist_sum = classifier_hist_sum (c);
    double bin_range = (c->range_max - c->range_min) / c->nbins;

    for (int i=0;i<c->nbins;i++) {
        char key[20], val[20];
        fprintf (fp, "%.4f ", c->range_min + i * bin_range + bin_range / 2);
        fprintf (fp, "%.4f ", hist_sum > 0 ? 1.0 * c->hist[i]/ (hist_sum * bin_range) : 0);
        fprintf (fp, "\n");
    }

    fclose (fp);
}

navlcm_dictionary_t *classifier_hist_to_dictionary (classifier_hist_t *c)
{
    navlcm_dictionary_t *d = (navlcm_dictionary_t*)calloc(1, sizeof(navlcm_dictionary_t));
    d->num = c->nbins;
    d->keys = (char**)calloc(d->num, sizeof(char*));
    d->vals = (char**)calloc(d->num, sizeof(char*));

    int hist_sum = classifier_hist_sum (c);

    double bin_range = (c->range_max - c->range_min) / c->nbins;

    for (int i=0;i<d->num;i++) {
        char key[20], val[20];
        sprintf (key, "%.4f", c->range_min + i * bin_range + bin_range / 2);
        sprintf (val, "%.4f", hist_sum > 0 ? 1.0 * c->hist[i]/ (hist_sum * bin_range) : 0);
        d->keys[i] = strdup (key);
        d->vals[i] = strdup (val);
    }

    return d;
}

