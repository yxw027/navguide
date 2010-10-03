/*
 * This is the code for feature tracking. This code is deprecated.
 */

#include "tracker.h"

#define  TRACKER_MAX_ANGLE 15.0 // maximum angle between two frames
                                // of the tracker

/* init tracks from the void
 */
navlcm_track_set_t *init_tracks ()
{
    navlcm_track_set_t *tracks = (navlcm_track_set_t *)malloc
        (sizeof(navlcm_track_set_t));
    tracks->num=0;
    tracks->el=NULL;
    tracks->maxuid=0;
    return tracks;
}

/* reset a set of tracks
 */
navlcm_track_set_t* reset_tracks (navlcm_track_set_t *tracks)
{
     if (tracks)
         navlcm_track_set_t_destroy (tracks);
     tracks = init_tracks ();

     return tracks;
}

/* get track by uid
 */
navlcm_track_t *get_track_by_uid (navlcm_track_set_t *tracks, int uid)
{
    for (int i=0;i<tracks->num;i++) {
        navlcm_track_t *track = tracks->el + i;
        if (track->uid == uid)
            return track;
    }

    return NULL;
}

/* determine the closest edge to a feature
 */
const char* feature_closest_edge_str (double col, double row, int width, int height)
{
    int dleft = math_round(col);
    int dright = math_round(width-1-col);
    int dtop = math_round(row);
    int dbottom = math_round (height-1-row);

    int dmin = MIN(MIN(MIN(dleft, dright),dtop), dbottom);

    if (dmin == dleft) return "left";
    if (dmin == dright) return "right";
    if (dmin == dbottom) return "bottom";
    if (dmin == dtop) return "top";
    assert (false);
}

/* compute the mean descriptor of a track
 */
double* compute_track_mean (navlcm_track_t *track)
{
    assert (track);
    assert (track->ft.num>0);

    int size = track->ft.el[0].size;
    double *val = (double*)malloc(size*sizeof(double));
    for (int i=0;i<size;i++) val[i]=0.0;

    for (int i=0;i<track->ft.num;i++)
        for (int j=0;j<size;j++)
            val[j] += 1.0 * track->ft.el[i].data[j];

    for (int i=0;i<size;i++)
        val[i] = 1.0*val[i]/track->ft.num;

    return val;
}

/* compute the median descriptor of a track as well as 25% and 75% percentiles
 */
void compute_track_median (navlcm_track_t *track, double *median, double *lowerq, double *upperq)
{
    int size = track->ft.el[0].size;

    double *values = (double*)malloc(track->ft.num*sizeof(double));

    for (int i=0;i<size;i++) {

        // fill the values
        for (int j=0;j<track->ft.num;j++) {
            values[j] = track->ft.el[j].data[i];
        }

        // sort
        gsl_sort (values, 1, track->ft.num);

        // compute median
        double med = gsl_stats_median_from_sorted_data (values, 1, track->ft.num);

        // compute upper decile
        double upq = gsl_stats_quantile_from_sorted_data (values, 1, track->ft.num, .75);

        // compute lower decile
        double loq = gsl_stats_quantile_from_sorted_data (values, 1, track->ft.num, .25);
  
        // store values
        median[i] = med;
        if (lowerq) lowerq[i] = loq;
        if (upperq) upperq[i] = upq;
    }

    free (values);
}

/* print out features to file
 */
void track_print_features (navlcm_track_t *track, const char *filename)
{
    // open file
    //
    FILE *fp = fopen (filename, "w");
    if (!fp) {
        dbg (DBG_ERROR, "failed to open %s in write mode", filename);
        return;
    }

    // print features in a matlab boxplot format
    //
    for (int i=0;i<track->ft.num;i++) {
        for (int j=0;j<track->ft.el[i].size;j++) {
            fprintf (fp, "%d %.4f\n", j, 1.0*track->ft.el[i].data[j]);
        }
    }
    fclose (fp);
}

/* compute the standard deviation of a track given the mean of the descriptors
 */
double *compute_track_stdev (navlcm_track_t *track, double *mean)
{
    assert (track); assert (mean);
    assert (track->ft.num > 0);

    int size = track->ft.el[0].size;
    double *val = (double*)malloc(size*sizeof(double));
    for (int i=0;i<size;i++) val[i]=0.0;

    for (int i=0;i<track->ft.num;i++) {
        for (int j=0;j<size;j++) {
            double d = 1.0 * track->ft.el[i].data[j] - mean[j];
            val[j] += d * d;
        }
    }

    for (int i=0;i<size;i++) 
        val[i] = MAX(1.0, sqrt (val[i]/track->ft.num)); // stdev is at least 1.0

    return val;
}

/* compute the number of times the track crossed cameras
 */
int track_n_crossings (navlcm_track_t *track)
{
    int count=0;
    for (int i=0;i<track->ft.num-1;i++) {
        if (track->ft.el[i].sensorid != track->ft.el[i+1].sensorid)
            count++;
    }
    return count;
}

/* compute some statistics about a track
 */
void compute_track_stat (navlcm_track_t *track, int width, int height)
{
    if (!track || track->ft.num == 0) return;

    if (!dir_exists ("/tmp/tracks")) {
        dbg (DBG_ERROR, "dir /tmp/tracks does not exist!");
        return;
    }

    FILE *fp;

    // skip if track did not cross a camera
    gboolean ok = FALSE;
    int idstart = track->ft.el[0].sensorid;
    for (int i=1;i<track->ft.num;i++)
        if (track->ft.el[i].sensorid != idstart) {ok=TRUE; break;}
    if (!ok) return;

    // skip if track is too short
    if (track->ft.num < 10) return;

    // lifespan
    int span = track->time_end - track->time_start + 1;
    fp = fopen ("/tmp/tracks/lifespan.txt", "a");
    fprintf (fp, "%d\n", span);
    fclose (fp);

    // mean 
    double *mean = compute_track_mean (track);

    // stdev
    double *stdev = compute_track_stdev (track, mean);

    // average standard deviation
    double avstdev = math_mean_double (stdev, track->ft.el[0].size);

    free (mean);
    free (stdev);

    fp = fopen ("/tmp/tracks/stdev.txt", "a");
    fprintf (fp, "%.4f\n", avstdev);
    fclose (fp);

    char fname[512];
    sprintf (fname, "/tmp/tracks/%d-stdev.txt", span);
    fp = fopen (fname, "a");
    fprintf (fp, "%.4f\n", avstdev);
    fclose (fp);

    // camera ID history (average number of transitions)
    int ntrans=0;
    for (int i=0;i<track->ft.num-1;i++)
        if (track->ft.el[i].sensorid != track->ft.el[i+1].sensorid)
            ntrans++;

    sprintf (fname, "/tmp/tracks/%d-transitions.txt", span);
    fp = fopen (fname, "a");
    fprintf (fp, "%d\n", ntrans);
    fclose(fp);

    // update the transition table
    const char *edgename_in, *edgename_out;
    for (int i=0;i<track->ft.num-1;i++) {
        // where the feature died
        if (i==0) {
            edgename_out = feature_closest_edge_str (track->ft.el[0].col, track->ft.el[0].row,
                    width, height);
            sprintf (fname, "/tmp/tracks/trans--%d-%s.txt", track->ft.el[0].sensorid, edgename_out);
            fp = fopen (fname, "a"); fprintf (fp, "1\n"); fclose(fp);
        }
        // where the feature transitioned
        if (track->ft.el[i].sensorid != track->ft.el[i+1].sensorid) {
            const char *edgename_out = feature_closest_edge_str (track->ft.el[i+1].col, 
                    track->ft.el[i+1].row, width, height);
            const char *edgename_in = feature_closest_edge_str (track->ft.el[i].col, 
                    track->ft.el[i].row, width, height);
            sprintf (fname, "/tmp/tracks/trans--%d-%s-%s-%d.txt", track->ft.el[i+1].sensorid, 
                    edgename_out,  edgename_in, track->ft.el[i].sensorid);
            fp = fopen (fname, "a"); fprintf (fp, "1\n"); fclose (fp);
        }
        // where the feature appeared
        if (i==track->ft.num-2) {
            edgename_in = feature_closest_edge_str (track->ft.el[i+1].col, track->ft.el[i+1].row,
                    width, height);
            sprintf (fname, "/tmp/tracks/trans--%s-%d.txt", edgename_in, track->ft.el[i+1].sensorid);
            fp = fopen (fname, "a"); fprintf (fp, "1\n"); fclose (fp);
        }
    }
}

/* append two tracks
 */
navlcm_track_t *track_append (navlcm_track_t *t1, navlcm_track_t *t2)
{
    assert (t1 && t2);
    assert (t2->time_end <= t1->time_start);

    t1->time_start = t2->time_start;
    for (int i=0;i<t2->ft.num;i++) {
        navlcm_feature_t *ft = t2->ft.el + i;
        t1 = navlcm_track_t_append (t1, ft);
    }

    return t1;
}

void annihilate_track (navlcm_track_t *t)
{
    assert (t);
    t->time_start = t->time_end;
    navlcm_feature_list_t_destroy (&t->ft);
    t->ft.num = 0;
    t->ft.el = NULL;
    t->ttl = 0;
}

/* update tracks given a set of features and a time to live (ttl)
 */
navlcm_track_set_t *update_tracks (navlcm_track_set_t *tracks,
                                     navlcm_feature_list_t *features,
                                     int ttl, int nbuckets, double tracker_max_dist,
                                     GList **dead_tracks)
{
    // sanity check
    if (!features || features->num==0) {
        dbg (DBG_ERROR, "no features to track.");
        return tracks;
    }

    // performance timer
    GTimer *timer = g_timer_new ();
    gulong usecs;    double secs;

    // init tracks
    if (!tracks) tracks = init_tracks ();

    int width  = features->width;
    int height = features->height;

    assert (width > 0 && height > 0);

    // populate tracks and return if there are none
    //
    if (tracks->num == 0) {
        for (int i=0;i<features->num;i++) {
            navlcm_track_t *new_track = 
                navlcm_track_t_create (features->el + i, width, height, ttl, tracks->maxuid);
            new_track->time_start = features->el[i].utime;
            new_track->time_end = new_track->time_start;
            tracks->maxuid++;
            // insert it in the list of tracks
            tracks = navlcm_track_set_t_append (tracks, new_track);
            navlcm_track_t_destroy (new_track);
        }
        return tracks;
    }

    // create a list of features from the latest track features
    navlcm_feature_list_t *last_fs = navlcm_feature_list_t_create 
        (width, height, 0, 0, features->desc_size);

    int nback=0,nfront=0,nborn=0;
    for (int i=0;i<tracks->num;i++) {
        if (tracks->el[i].ttl <= 0) continue; // skip dead tracks
        assert (tracks->el[i].ft.num > 0); // skip empty tracks
        navlcm_feature_t *ft = tracks->el[i].ft.el;
        if (ft->sensorid == 0 || ft->sensorid == 3) nback++; else nfront++;
        if (tracks->el[i].ttl == ttl && tracks->el[i].ft.num == 1 && \
                (ft->sensorid == 1 || ft->sensorid == 2)) nborn++;
        ft->uid = i;
        last_fs = navlcm_feature_list_t_append (last_fs, ft);
    }

    // match track features against input features (within cameras)
    navlcm_feature_match_set_t *matches = find_feature_matches_multi 
        (last_fs, features, TRUE, FALSE, 5, .6, tracker_max_dist, -1.0, TRUE);

    // destroy features
    navlcm_feature_list_t_destroy (last_fs);

    secs = g_timer_elapsed (timer, &usecs);
    if (matches)
        dbg (DBG_CLASS, "found %d matches in %.3f secs.", matches->num, secs);

    // update each track while keeping a list of matched tracks
    GList *matched_tracks=NULL;
    int matched=0;
    if (matches) {
        for (int i=0;i<matches->num;i++) {
            navlcm_feature_match_t *match = matches->el + i;
            if (match->num == 0) continue;
            int track_index = match->src.uid;
            navlcm_track_t *track = tracks->el + track_index;
            // add feature to track
            track = navlcm_track_t_insert_head (track, match->dst);
            matched_tracks = g_list_prepend (matched_tracks, GINT_TO_POINTER(track_index));
            // reset the track ttl
            track->ttl = ttl;
            // update track time end
            track->time_end = match->dst[0].utime;
        }
        matched = g_list_length (matched_tracks);
    }

    // free matches
    if (matches)
        navlcm_feature_match_set_t_destroy (matches);

    // demote non-matched tracks
    int demoted=0;
    int died=0;
    for (int i=0;i<tracks->num;i++) {
        if (tracks->el[i].ttl <= 0) continue;
        GList *needle = g_list_find (matched_tracks,GINT_TO_POINTER(i));
        if (needle) {continue;}
        tracks->el[i].ttl--;
        demoted++;
        navlcm_feature_t *ft = tracks->el[i].ft.el; // last feature seen
        if (tracks->el[i].ttl == 0 && (ft->sensorid==0 || ft->sensorid==3)) died++;
        if (tracks->el[i].ttl == 0 && dead_tracks) {
            *dead_tracks = g_list_prepend (*dead_tracks, navlcm_track_t_copy (tracks->el + i));
        }
    }

    g_list_free (matched_tracks);

    // performance timer
    secs = g_timer_elapsed (timer, &usecs);
    g_timer_destroy (timer);

    // save info in text file
    //FILE *hp = fopen (REMOTE_DATA_DIR"/output/birth_death_rate.txt", "a");
    //fprintf (hp, "%d %.4f %d %.4f\n", died, 100.0*died/nback, nborn, 100.0*nborn/nfront);
    //fclose (hp);

    dbg (DBG_CLASS, "tracks: %d matched, %d demoted, %d died, total %d (%.3f secs)", matched, demoted, died, tracks->num, secs);

    return tracks;
}

/* this function should return 0 when the two input elements are the same
*/
gint feature_comp (gconstpointer a, gconstpointer b)
{
    const navlcm_feature_t *f1 = (const navlcm_feature_t*)(a);
    const navlcm_feature_t *f2 = (const navlcm_feature_t*)(b);
    if (f1->index == f2->index && f1->utime == f2->utime && f1->sensorid == f2->sensorid)
        return 0;
    return -1;
}

/* add new tracks given a set of features
* param_min_dist: minimum distance between a new track feature and all existing track features, in pixels
*/
navlcm_track_set_t *add_new_tracks (navlcm_track_set_t *tracks, navlcm_feature_list_t *features, int ttl, double param_min_dist)
{
    if (!features) return tracks;
    GTimer *timer = g_timer_new ();
    int width = features->width;
    int height = features->height;

    // create a list of the track features
    GList *track_features = NULL;
    int nfront=0;
    for (int i=0;i<tracks->num;i++) {
        navlcm_track_t *track = tracks->el + i;
        if (track->ttl <=0 || track->ft.num == 0) continue;
        navlcm_feature_t *ft = track->ft.el;
        if (ft->sensorid==1 || ft->sensorid==2) nfront++;
        track_features = g_list_prepend (track_features, ft);
    }

    dbg (DBG_CLASS, "creating new tracks: there are %d tracks alive currently.", g_list_length (track_features));

    // for each input feature not in the list, create a new track
    //
    int ncreated=0;
    for (int i=0;i<features->num;i++) {
        navlcm_feature_t *ft = features->el + i;
        // use a custom find function
        if (g_list_find_custom (track_features, ft, (GCompareFunc)feature_comp))
            continue;
        // check mininum distance
        if (track_features) {
            double min_dist = features_min_distance (ft, track_features);
            if (min_dist < param_min_dist)
                continue;
        }
        // create a new track
        navlcm_track_t *new_track = navlcm_track_t_create (ft, width, height, ttl, tracks->maxuid);
        new_track->time_start = ft->utime;
        new_track->time_end = new_track->time_start;
        tracks->maxuid++;
        // insert it in the list of tracks
        tracks = navlcm_track_set_t_append (tracks, new_track);
        navlcm_track_t_destroy (new_track);
        ncreated++;
    }

    // free memory
    g_list_free (track_features);

    gulong usecs;
    gdouble secs = g_timer_elapsed (timer, &usecs);
    g_timer_destroy (timer);

    dbg (DBG_CLASS, "created %d new tracks in %.3f secs.", ncreated, secs);

    return tracks;
}

navlcm_track_set_t *fuse_tracks (navlcm_track_set_t *tracks, int ttl, int width, int height, lcm_t *lcm)
{
    int ttl2 = 2 * ttl;

    // set of "last feature seen" for dead tracks
    //
    int64_t utime=0;
    navlcm_track_set_t *ptracks = init_tracks ();

    navlcm_feature_list_t *ls = navlcm_feature_list_t_create (width, height, 0, 0, FEATURES_DIST_MODE_UNK);
    for (int i=0;i<tracks->num;i++) {
        navlcm_track_t *track = tracks->el + i;
        if (track->ttl != -ttl2) continue;
        assert (track->ft.num > 0);
        navlcm_feature_t *ft = track->ft.el; // last feature seen on this track
        if (!feature_on_edge (ft->col, ft->row, width, height, 1.0*width/6)) continue;
        ptracks = navlcm_track_set_t_append (ptracks, track); // add to tracks to publish
        ptracks->maxuid = track->uid;
        ft->uid = track->uid;
        if (utime == 0 || ft->utime < utime)
            utime = ft->utime;
        ls = navlcm_feature_list_t_append (ls, ft); 
    }


    // set of "first feature seen" for future tracks
    //
    navlcm_feature_list_t *rs = navlcm_feature_list_t_create (width, height, 0, 0, FEATURES_DIST_MODE_UNK);
    for (int i=0;i<tracks->num;i++) {
        navlcm_track_t *track = tracks->el + i;
        assert (track->ft.num > 0);
        navlcm_feature_t *ft = track->ft.el + track->ft.num - 1; // first feature seen on this track
        if (ft->utime < utime) continue;
        ft->uid = track->uid;
        rs = navlcm_feature_list_t_append (rs, ft);
    }

    dbg (DBG_CLASS, "target set has %d tracks.", rs->num);

    // match features forward
    //
    navlcm_feature_match_set_t *matches = find_feature_matches_multi 
        (ls, rs, FALSE, TRUE, 5, 1.0, -1.0, -1.0, FALSE);

    if (!matches) return tracks;

    // store information in a hash table
    //
    GHashTable *hash_forward = g_hash_table_new_full (g_int_hash, g_int_equal, (GDestroyNotify)g_free, (GDestroyNotify)g_free);

    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *match = matches->el + i;
        if (match->num == 0) continue;
        //if (match->src.utime > match->dst[0].utime) continue; // enforce time constraint
        g_hash_table_replace (hash_forward, g_intdup (match->src.uid), g_intdup (match->dst[0].uid));
        navlcm_track_t *copy = navlcm_track_t_copy (get_track_by_uid (tracks, match->dst[0].uid));
        copy->uid = match->src.uid;
        ptracks = navlcm_track_set_t_append (ptracks, copy); // add to tracks to publish
        navlcm_track_t_destroy (copy);
    }

    // publish tracks
    publish_tracks (lcm, ptracks);
    navlcm_track_set_t_destroy (ptracks);

    dbg (DBG_CLASS, "forward matching: %d matches, %d elements in hash table.", matches->num, g_hash_table_size (hash_forward));

    // free matches
    navlcm_feature_match_set_t_destroy (matches);

    g_hash_table_destroy (hash_forward);
    return tracks;

    // match features backward
    //
    matches = find_feature_matches_multi (rs, ls, FALSE, TRUE, 5, .6, -1.0, -1.0, TRUE);

    if (!matches) return tracks;

    // store information in a hash table
    //
    GHashTable *hash_backward = g_hash_table_new_full (g_int_hash, g_int_equal, (GDestroyNotify)g_free, (GDestroyNotify)g_free);

    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *match = matches->el + i;
        if (match->num == 0) continue;
        if (match->src.utime < match->dst[0].utime) continue; // enforce time constraint
        g_hash_table_replace (hash_backward, g_intdup (match->src.uid), g_intdup (match->dst[0].uid));
    }

    dbg (DBG_CLASS, "backward matching: %d matches, %d elements in hash table.", matches->num, g_hash_table_size (hash_backward));

    // free matches
    navlcm_feature_match_set_t_destroy (matches);

    // free features
    navlcm_feature_list_t_destroy (rs);
    navlcm_feature_list_t_destroy (ls);

    // compute a hash table with cross consistency check
    //
    GHashTable *hash_check = g_hash_table_new_full (g_int_hash, g_int_equal, (GDestroyNotify)g_free, (GDestroyNotify)g_free);

    GHashTableIter iter;
    gpointer key, value;

    // parse hash_forward
    g_hash_table_iter_init (&iter, hash_forward);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
        int *index1 = (int*)key;
        int *index2 = (int*)value;
        // lookup in hash_backward
        gpointer data = g_hash_table_lookup (hash_backward, index2);
        if (data) {
            int *index3 = (int*)data;
            if (*index3 == *index1) {
                g_hash_table_insert (hash_check, g_intdup (*index1), g_intdup (*index2));
            }
        }
    }

    dbg (DBG_CLASS, "consistency check: %d elements.", g_hash_table_size (hash_check));

    // fuse tracks
    //
    GList *rem_list = NULL;

    g_hash_table_iter_init (&iter, hash_check);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
        int *index1 = (int*)key;
        int *index2 = (int*)value;
        navlcm_track_t *track1 = tracks->el + *index1;
        navlcm_track_t *track2 = tracks->el + *index2;
        track2 = track_append (track2, track1);
        rem_list = g_list_prepend (rem_list, GINT_TO_POINTER (*index1));
    }

    dbg (DBG_CLASS, "%d tracks to remove.", g_list_length (rem_list));

    // remove tracks
    //
    navlcm_track_set_t *new_tracks = init_tracks ();
    for (int i=0;i<tracks->num;i++) {
        if (g_list_find (rem_list, GINT_TO_POINTER (i)) == NULL) {
            new_tracks = navlcm_track_set_t_append (new_tracks, tracks->el + i);
        }
    }

    // free memory
    g_hash_table_destroy (hash_forward);
    g_hash_table_destroy (hash_backward);
    g_hash_table_destroy (hash_check);
    g_list_free (rem_list);
    navlcm_track_set_t_destroy (tracks);

    return new_tracks;
}

/* publish a set of tracks (alive tracks only)
*/
void publish_tracks (lcm_t *lcm, navlcm_track_set_t *tracks)
{
    // keep only alive tracks
    navlcm_track_set_t *alive = init_tracks();
    alive->maxuid = tracks->maxuid;

    for (int i=0;i<tracks->num;i++) {
        if (tracks->el[i].ttl<=0) continue;
        navlcm_track_t *copy = navlcm_track_t_copy (tracks->el + i);
        alive = navlcm_track_set_t_append (alive, copy);
        navlcm_track_t_destroy (copy);
    }

    dbg (DBG_CLASS, "[class] publishing %d tracks.", alive->num);

    // publish
    navlcm_track_set_t_publish (lcm, "TRACKS", alive);

    // free
    navlcm_track_set_t_destroy (alive);
}



/* remove dead tracks and return the new list of tracks
*/
navlcm_track_set_t * remove_dead_tracks (navlcm_track_set_t *tracks, navlcm_track_set_t **dead_tracks)
{
    if (!tracks) return NULL;

    // current list of alive tracks
    navlcm_track_set_t *atracks = init_tracks ();

    for (int i=0;i<tracks->num;i++) {
        navlcm_track_t *copy = navlcm_track_t_copy (tracks->el + i);
        if (copy->ttl<=0 && dead_tracks) {
            *dead_tracks = navlcm_track_set_t_append (*dead_tracks, copy);
            (*dead_tracks)->maxuid = MAX((*dead_tracks)->maxuid, copy->uid);
        }
        else {
            atracks = navlcm_track_set_t_append (atracks, copy);
            atracks->maxuid = MAX(atracks->maxuid, copy->uid);
        }
        navlcm_track_t_destroy (copy);
    }

    if (dead_tracks) {
        dbg (DBG_CLASS, "removed %d dead tracks.", (*dead_tracks)->num);
    }

    // destroy input tracks
    navlcm_track_set_t_destroy (tracks);

    return atracks;
}

/* generate a random track
*/
navlcm_track_t *track_random (int length, int ftsize, int ncrossings, int64_t reftime,
        int width, int height)
{
    navlcm_track_t *track = navlcm_track_t_create (NULL, width, height, 0, 0);

    track->time_start = reftime + (int)(100.0 * rand() / RAND_MAX) * 1000000;
    track->time_start = track->time_start + (int)(10.0 * rand() / RAND_MAX) * 1000000;

    double col = 200;
    double row = 300;
    double *data = (double*)malloc(ftsize*sizeof(double));
    for (int i=0;i<ftsize;i++) 
        data[i] = 1.0*rand()/RAND_MAX;

    for (int i=0;i<length;i++) {
        // randomly modify the feature descriptor
        for (int j=0;j<ftsize;j++)
            data[j] *= 1.0 + .10 * (1.0 * rand() / RAND_MAX - .5);
        int sensorid = (int)(1.0 * i / length * ncrossings) % 4;
        double scale = 1.0;
        navlcm_feature_t *ft = navlcm_feature_t_create (sensorid, col, row, 0, scale);
        ft->size = ftsize;
        ft->data = (float*)malloc(ft->size*sizeof(float));
        memcpy (ft->data, data, ftsize*sizeof(float));

        track = navlcm_track_t_insert_head (track, ft);
        navlcm_feature_t_destroy (ft);
    }

    return track;
}

static int g_sensor_pair_hash[6][6] = { { 0,  1,  2,  3,  4,  5},
    { 6,  7,  8,  9, 10, 11},
    {12, 13, 14, 15, 16, 17},
    {18, 19, 20, 21, 22, 23},
    {24, 25, 26, 27, 28, 29},
    {30, 31, 32, 33, 34, 35}};

int track_path_code (navlcm_track_t *track)
{

    assert (track && track->ft.num > 1);

    int start_sensor = track->ft.el[0].sensorid;
    int end_sensor   = track->ft.el[track->ft.num-1].sensorid;

    return g_sensor_pair_hash [start_sensor][end_sensor];
}

/* split a track in sub-tracks
*/
GList *track_split (navlcm_track_t *track)
{
    navlcm_feature_t *ft = track->ft.el + track->ft.num - 1;
    double thresh = 3.0;
    double averror = 0.0;
    GList *tracks = NULL;

    navlcm_track_t *new_track = NULL;

    navlcm_track_t_create (NULL, track->ft.width, track->ft.height, 
            track->ttl, track->uid);

    for (int i=0;i<track->ft.num;i++) {

        navlcm_feature_t *copy = navlcm_feature_t_copy (ft);        

        if (!new_track) {
            new_track = navlcm_track_t_create (copy, track->ft.width, track->ft.height, 
                    track->ttl, track->uid);
            new_track->time_start = copy->utime;
            new_track->time_end = copy->utime;
            averror = 0.0;
        } else {
            int count = new_track->ft.num;
            double error = vect_sqdist_float (copy->data, new_track->ft.el[0].data, copy->size);
            error = sqrt(error)/copy->size;

            if (count > 3 && error > thresh * averror) {
                // finish this track
                assert (new_track->time_end >= new_track->time_start);
                tracks = g_list_prepend (tracks, navlcm_track_t_copy (new_track));
                navlcm_track_t_destroy (new_track);
                new_track = NULL;
            } else {
                // add feature to the current track
                new_track = navlcm_track_t_insert_head (new_track, copy);
                new_track->time_end = copy->utime;
                averror = 1.0 * count / (count+1) * averror + 1.0 / (count+1) * error;            }
        }

        navlcm_feature_t_destroy (copy);
        ft--;
    }

    // add the end of the track
    //
    if (new_track) {
        tracks = g_list_prepend (tracks, navlcm_track_t_copy (new_track));
        navlcm_track_t_destroy (new_track);
    }

    if (g_list_length (tracks) > 1) {
        //        dbg (DBG_CLASS, "split track into %d tracks.", g_list_length (tracks));
    }

    return tracks;
}

/* given a feature on the image, determine what its fate is
 * this method is camera setup- dependent
 */
int feature_fate (navlcm_feature_t *ft, int width, int height)
{
    int edge = feature_edge_code (ft->col, ft->row, width, height);
    assert (edge != -1);

    switch (ft->sensorid) {
        case 1:
        case 3:
            switch (edge) {
                case EDGE_TOP:
                    return FATE_TOP_LEFT;
                case EDGE_BOTTOM:
                    return FATE_BOTTOM_LEFT;
                case EDGE_LEFT:
                    return FATE_LEFT;
                case EDGE_RIGHT:
                    return 0;
                default:
                    assert (false);
            }
            break;
        case 0:
        case 2:
            switch (edge) {
                case EDGE_TOP:
                    return FATE_TOP_RIGHT;
                case EDGE_BOTTOM:
                    return FATE_BOTTOM_RIGHT;
                case EDGE_LEFT:
                    return 0;
                case EDGE_RIGHT:
                    return FATE_RIGHT;
                default:
                    assert (false);
            }
        break;
        default:
        assert (false);
    }

    return 0;
}
/* filter a list of features on utime
 */
navlcm_feature_list_t* track_filter_on_utime (navlcm_feature_list_t *fs, int64_t utime)
{
    if (!fs) return NULL;

    navlcm_feature_list_t *nfs = navlcm_feature_list_t_create (fs->width, fs->height, fs->sensorid, fs->utime, fs->desc_size);

    for (int i=0;i<fs->num;i++) {
        navlcm_feature_t *f = fs->el + i;
        if (f->utime > utime)
            nfs = navlcm_feature_list_t_append (nfs, f);
    }

    navlcm_feature_list_t_destroy (fs);

    return nfs;
}

/* convert a track to a feature
 */
navlcm_feature_t* track_to_feature (navlcm_track_t *t, int width, int height)
{
    assert (t);
    if (t->ft.num == 0) return NULL;
    int size = t->ft.el[0].size;
    int sensorid = t->ft.el[0].sensorid;

    navlcm_feature_t *ft = t->ft.el;    // consider last feature on track for forward-facing cameras 
    if (sensorid == 0 || sensorid == 3) {
        ft = t->ft.el + t->ft.num-1; // consider first feature on track for backward-facing cameras
    }
    int64_t utime = ft->utime;
    double col = ft->col;
    double row = ft->row;
    // compute the feature fate
    int uid = feature_fate (ft, width, height); 

    navlcm_feature_t *f = navlcm_feature_t_create ();
    f->col = col;
    f->row = row;
    f->uid = uid;
    f->utime = utime;
    f->sensorid = sensorid;
    f->size = size;
    f->data = (float*)malloc(size*sizeof(float));
    f->index = 0;
    // compute the mean descriptor
    double *avg = compute_track_mean (t);
    for (int i=0;i<size;i++)
        f->data[i] = MAX(0, MIN(255, math_round (avg[i])));

    return f;
}

/* convert a list of tracks to a list of features
 */
navlcm_feature_list_t* tracks_to_features (GList *ts, int width, int height, int sensorid, int64_t utime, int desc_size)
{
    if (!ts) return NULL;

    navlcm_feature_list_t *fs = navlcm_feature_list_t_create (width, height, sensorid, utime, desc_size);

    for (GList *iter=g_list_first(ts);iter!=NULL;iter=g_list_next(iter)) {
        navlcm_track_t *t = (navlcm_track_t*)(iter->data);
        navlcm_feature_t *f = track_to_feature (t, width, height);
        if (f) {
            if (f->uid > 0)
                fs = navlcm_feature_list_t_append (fs, f);
            navlcm_feature_t_destroy (f);
        }
    }

    return fs;
}

