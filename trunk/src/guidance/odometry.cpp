#include "odometry.h"

/* compute odometry based on flow (deprecated)
 */
int odometry_compute_from_flow (flow_t *f, double *rot_speed, double *trans_speed, double *relative_z)
{
    double yd = .0;
    int nvalid = 0;

    int64_t delta_usecs = f->utime1 - f->utime0;

    if (!f->flow_grey || !f->flow_points[0] || !f->flow_points[1] || delta_usecs == 0) return 0;

    for (int i=0 ; i<f->flow_count; i++) {
        if (!f->flow_status)
            continue;
        yd += 1.0 * (f->flow_points[1][i].y - f->flow_points[0][i].y) / f->flow_grey->height;
        nvalid++;
    }

    if (nvalid > 0) {
        yd /= nvalid;
    }

    *rot_speed = .0;
    *trans_speed = .0;
    *relative_z = yd * 1000000.0 / delta_usecs;

    return 1;
}

static int64_t g_first_update_utime = 0;
static int64_t g_last_update_utime = 0;

/* compute odometry from set of flow (deprecated)
 */
void odometry_compute_from_flow_set (flow_t **flow, int nflow, double *rot_speed, double *trans_speed, double *relative_z)
{
    double rs=.0, ts=.0, rz=.0;
    int64_t utime = flow[0]->utime1;
    int count = 0;

    if (g_first_update_utime == 0)
        g_first_update_utime = utime;

    // skip if no new flow information
    if (utime == g_last_update_utime)
        return;

    g_last_update_utime = utime;

    for (int i=0;i<nflow;i++) {
        double irs=.0, its=.0, irz=.0;
        if (odometry_compute_from_flow (flow[i], &irs, &its, &irz)) {
            rs += irs;
            ts += its;
            rz += irz;
            count++;
        }
    }

    if (count > 0) {
        double secs = 1.0 * ((int)(utime - g_first_update_utime))/1000000.0;
        FILE *fp = fopen ("odometry.txt", utime == g_first_update_utime ? "w" : "a");
        fprintf (fp, "%.2f %.3f %.3f %.3f\n", secs, rs/count, ts/count, rz/count);
        fclose (fp);
    }
}

void odometry_classify_motion_from_flow (GQueue *motions, flow_t **flow, int num)
{
    int nmotions = g_queue_get_length (motions);
    double totalscore = .0;
    double maxscore = .0;
    int maxscoreindex = -1;

    int64_t utime = flow[0]->utime1;

    if (g_first_update_utime == 0)
        g_first_update_utime = utime;

    // skip if no new flow information
    if (utime == g_last_update_utime)
        return;

    g_last_update_utime = utime;
    
    // save flow to images
    for (int i=0;i<num;i++) {
        char fname[256];
        sprintf (fname, "flow-%d.png", i);
        flow_draw (flow[i], fname);
    }

    int nbuckets = 4;
    double *scores = math_3d_array_alloc_double (num, nbuckets*nbuckets, nmotions);
    int    *nscores = math_3d_array_alloc_int (num, nbuckets*nbuckets, nmotions);

    // for each sensor
    for (int i=0;i<num;i++) {
        flow_t *fl = flow[i];

        int64_t delta_usecs = fl->utime1 - fl->utime0;

        // for each flow vector...
        for (int j=0;j<fl->flow_count;j++) {
            if (!fl->flow_status[j])
                continue;

            double x = fl->flow_points[0][j].x/fl->width;
            double y = fl->flow_points[0][j].y/fl->height;

            double fx = ( fl->flow_points[1][j].x - fl->flow_points[0][j].x ) / fl->width * 1000000.0 / delta_usecs;
            double fy = ( fl->flow_points[1][j].y - fl->flow_points[0][j].y ) / fl->height * 1000000.0 / delta_usecs;

            int idx = MIN (int (x * nbuckets), nbuckets-1); 
            int idy = MIN (int (y * nbuckets), nbuckets-1); 

            int bucket_index = idx * nbuckets + idy;

            if (!(-1E-6 < x && x < 1.0 + 1E-6) || !(-1E-6 < y && y < 1.0 + 1E-6)) 
                continue;

            // score each motion type
            for (int k=0;k<nmotions;k++) {

                // the k-th reference flow for sensor i
                flow_field_t *flow_field = (flow_field_t*)((flow_field_t**)g_queue_peek_nth (motions, k))[i];

                double s = flow_field_score_base (flow_field, x, y, fx, fy);

                assert (0 <= bucket_index && bucket_index < nbuckets*nbuckets);

                double val = math_3d_array_get_double (scores, num, nbuckets*nbuckets, nmotions, i, bucket_index, k) + s;
                int n = math_3d_array_get_int (nscores, num, nbuckets*nbuckets, nmotions, i, bucket_index, k) + 1;

                math_3d_array_set_double (scores, num, nbuckets*nbuckets, nmotions, i, bucket_index, k, val);
                math_3d_array_set_int   (nscores, num, nbuckets*nbuckets, nmotions, i, bucket_index, k, n);

            }
        }
    }

    // aggregated classifier rankings
    int *ascores = (int*)calloc (nmotions, sizeof(int));
    for (int i=0;i<num;i++) {
        for (int j=0;j<nbuckets*nbuckets;j++) {
            GList *motion_votes = NULL;
            int nvotes=0;
            for (int k=0;k<nmotions;k++) {
                pair_t *p = (pair_t*)malloc(sizeof(pair_t));
                p->key = k;
                p->val = math_3d_array_get_double (scores, num, nbuckets*nbuckets, nmotions, i, j, k);
                motion_votes = g_list_append (motion_votes, p);
                nvotes += math_3d_array_get_int (nscores, num, nbuckets*nbuckets, nmotions, i, j, k);
            }
            if (nvotes > 0) {
                motion_votes = g_list_sort (motion_votes, pair_comp_val);
                for (int k=0;k<nmotions;k++) {
                    int key = ((pair_t*)g_list_nth_data (motion_votes, k))->key;
                    assert (0 <= key && key < nmotions);
                    ascores[key] += k;
                }
            }
            for (int k=0;k<nmotions;k++)
                free ((pair_t*)g_list_nth_data (motion_votes, k));
            g_list_free (motion_votes);
        }
    }

    printf ("finding best scoring motion\n");

    /* find best scoring motion */
    for (int i=0;i<nmotions;i++) {
        //scores[i] = flow_field_set_compute_score ((flow_field_t**)g_queue_peek_nth (motions, i), flow, num);
        if (maxscoreindex < 0 || maxscore < ascores[i]) {
            maxscore = ascores[i];
            maxscoreindex = i;
        }
    }

    printf ("ascores: %d %d %d %d %d\n", ascores[0], ascores[1], ascores[2], ascores[3], ascores[4]);

    FILE *fp = fopen ("motion-type.txt", "a");
    fprintf (fp, "%d\n", maxscoreindex);
    fclose (fp);

    printf ("MOTION TYPE: %d\n", maxscoreindex);

    free (scores);
    free (nscores);
    free (ascores);
}

/* compute coarse estimate of odometry using features (deprecated)
*/
void odometry_compute (navlcm_feature_list_t *f1, navlcm_feature_list_t *f2, double pix_to_rad, double scene_depth,
        double *rot_speed, double *trans_speed, double *relative_z)
{
    int count = 0;

    int dutime = (int)(f2->utime - f1->utime);
    double dt = dutime/1000000.0;

    // match features
    navlcm_feature_match_set_t *matches = (navlcm_feature_match_set_t*)malloc(sizeof(navlcm_feature_match_set_t));
    int matching_mode = f1->feature_type == NAVLCM_FEATURES_PARAM_T_FAST ? MATCHING_NCC : MATCHING_DOTPROD;
    find_feature_matches_fast (f1, f2, TRUE, TRUE, TRUE, TRUE, .75, -1, matching_mode, matches);

    // estimate average optical flow vector
    double avflow[2] = {.0, .0};
    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *m = (navlcm_feature_match_t*)matches->el + i;
        navlcm_feature_t *u1 = &m->src;
        navlcm_feature_t *u2 = &m->dst[0];
        if (u1->sensorid != u2->sensorid)
            continue;
        avflow[0] += u2->col - u1->col;
        avflow[1] += u2->row - u1->row;
        count++;
    }

    if (count > 0) {
        avflow[0] /= count;
        avflow[1] /= count;
    }

    *relative_z = - sin (avflow[0] * pix_to_rad) * scene_depth;

    *rot_speed = avflow[1] * pix_to_rad / dt;

    // compute residual
    double res = .0;
    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *m = (navlcm_feature_match_t*)matches->el + i;
        navlcm_feature_t *u1 = &m->src;
        navlcm_feature_t *u2 = &m->dst[0];
        if (u1->sensorid != u2->sensorid)
            continue;
        res += sqrt (powf (u2->col - u1->col - avflow[0], 2) + powf (u2->row - u1->row - avflow[1], 2));
    }

    if (count > 0)
        res /= count;

    *trans_speed = sin (res * pix_to_rad) * scene_depth / dt;

    navlcm_feature_match_set_t_destroy (matches);

}

void odometry_from_pose (botlcm_pose_t *p1, botlcm_pose_t *p2,
        double *rot_speed, double *trans_speed, double *relative_z)
{
    int dutime = (int)(p2->utime - p1->utime);
    double dt = dutime/1000000.0;

    *relative_z = p2->pos[2] - p1->pos[2];

    double d = .0;
    for (int i=0;i<3;i++)
        d += powf (p2->pos[i]-p1->pos[i],2);
    d = sqrt(d);

    *trans_speed = d / dt;

    double a = class_compute_pose_rotation (p1, p2);

    *rot_speed = a / dt;
}

