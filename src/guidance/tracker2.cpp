#include <tracker2.h>

/* destroy a track
 */
int track2_t_destroy (track2_t *t)
{
    for (GList *iter=g_queue_peek_head_link (t->data);iter;iter=iter->next) {
        navlcm_feature_t_destroy ((navlcm_feature_t*)iter->data);
    }
    g_queue_free (t->data);
    return 0;
}

/* insert a feature in a track
 */
int track_insert_feature (track2_t *t, navlcm_feature_t *f)
{
    g_queue_push_head (t->data, navlcm_feature_t_copy (f));
    return 0;
}

/* convert track to lcm structure 
 */
navlcm_track_t *track_to_lcm (track2_t *t)
{
    if (g_queue_is_empty (t->data))
        return NULL;

    navlcm_track_t *tr = (navlcm_track_t*)malloc(sizeof(navlcm_track_t));
    tr->ft.num = g_queue_get_length (t->data);
    tr->ft.el = (navlcm_feature_t*)malloc(tr->ft.num*sizeof(navlcm_feature_t));
    int i=0;

    for (GList *iter=g_queue_peek_tail_link(t->data);iter;iter=iter->prev) {
        navlcm_feature_t *f = navlcm_feature_t_copy ((navlcm_feature_t*)iter->data);
        tr->ft.el[i] = *f;
        if (i==0) tr->time_start = f->utime;
        if (i==tr->ft.num-1) tr->time_end = f->utime;
        free (f);
        i++;
    }

    tr->ttl = 1;
    tr->uid = 0;
    return tr;
}

/* convert track list to lcm structure
*/
navlcm_track_set_t *track_list_to_lcm (tracker2_data_t *tracks)
{
    if (g_queue_is_empty (tracks->data))
        return NULL;

    navlcm_track_set_t *tr = (navlcm_track_set_t*)malloc(sizeof (navlcm_track_set_t));

    tr->maxuid = g_queue_get_length (tracks->data);
    tr->num = g_queue_get_length (tracks->data);
    tr->el = (navlcm_track_t*)malloc(tr->num*sizeof(navlcm_track_t));
    int i=0;

    for (GList *iter=g_queue_peek_head_link (tracks->data);iter;iter=iter->next) {
        track2_t *t = (track2_t*)iter->data;
        navlcm_track_t *t2 = track_to_lcm (t);
        t2->uid = i;
        assert (t2);
        tr->el[i] = *t2;
        free (t2);
        i++;
    }
    return tr;
}

/* create a list of features from all tracks
*/
int tracks_head_features (tracker2_data_t *tracks, navlcm_feature_list_t *fs)
{   
    int i=0;
    fs->num = g_queue_get_length (tracks->data);
    if (fs->num == 0) {
        fs->el = NULL;
        return 0;
    }

    fs->el = (navlcm_feature_t*)malloc(fs->num*sizeof(navlcm_feature_t));

    for (GList *iter=g_queue_peek_head_link (tracks->data);iter;iter=iter->next) {
        track2_t *t = (track2_t*)iter->data;
        navlcm_feature_t *f = navlcm_feature_t_copy ((navlcm_feature_t*)g_queue_peek_head (t->data));
        fs->el[i] = *f;
        fs->el[i].index = i;
        fs->desc_size = f->size;
        free (f);
        i++;
    }

    return 0;
}

/* init a tracker */
int tracker_data_init (tracker2_data_t *tracks)
{
    dbg (DBG_CLASS, "[tracker] init tracker empty.");

    tracks->data = g_queue_new ();
    tracks->frontmat = NULL;
    tracks->frontmat_total = NULL;
    tracks->nsensors = 0;
    tracks->frontmat_training = TRUE;
    return 0;
}

/* add a track to the tracker
*/
int tracker_data_add_track (tracker2_data_t *tracks, navlcm_feature_t *f)
{
    track2_t *t = (track2_t*)malloc(sizeof(track2_t));
    t->data = g_queue_new ();
    track_insert_feature (t, f);
    g_queue_push_tail (tracks->data, t);
    return 0;
}

/* init tracker from a set of features
*/
int tracker_data_init_from_data (tracker2_data_t *tracks, navlcm_feature_list_t *f, int nsensors)
{
    dbg (DBG_CLASS, "[tracker] init tracker from %d features.", f->num);

    tracks->nsensors = nsensors;

    for (int i=0;i<f->num;i++) {
        tracker_data_add_track (tracks, f->el + i);
    }
    return 0;
}

/* update the frontier matrix given a co-descriptor matrix <descmat>
 * and the features <fprev, fnext> that generated the matrix
 */
int tracks_train_step (tracker2_data_t *tracks, navlcm_feature_match_set_t *matches, int width, int height, int nsensors)
{
    dbg (DBG_CLASS, "[tracker] training tracker with %d matches.", matches->num);

    /* init if does not exist */
    if (!tracks->frontmat) {
        tracks->frontmat = math_init_mat_float (4*nsensors, 4*nsensors, .0);
        tracks->frontmat_total = math_init_mat_int (4*nsensors, 4*nsensors, 0);
    }

    /* update matrix using cross-camera matches */
    for (int i=0;i<matches->num;i++) {
        navlcm_feature_t *f1 = &matches->el[i].src;
        navlcm_feature_t *f2 = matches->el[i].dst;
        if (f1->sensorid != f2->sensorid) {
            int el1 = edge_to_index (f1, width, height);
            int el2 = edge_to_index (f2, width, height);
            tracks->frontmat_total[el1][el2]++;
        }
    }

    /* update ratio matrix */
    int grandtotal = 0;

    for (int i=0;i<4*nsensors;i++) {
        int total = 0;
        for (int j=0;j<4*nsensors;j++)
            total += tracks->frontmat_total[i][j];
        grandtotal += total;
        for (int j=0;j<4*nsensors;j++) {
            if (total > 0) 
                tracks->frontmat[i][j] = 1.0 * tracks->frontmat_total[i][j] / total;
        }
    }

    /* order not to train anymore */
    if (grandtotal > nsensors * 100000)
        tracks->frontmat_training = FALSE;
    return 0;
}

int tracker_data_printout (tracker2_data_t *tracks)
{
    if (tracks->frontmat) {
        printf ("------- front matrix -------\n");
        int size = 4*tracks->nsensors;
        for (int i=0;i<size;i++) {
            for (int j=0;j<size;j++) {
                printf ("%.4f ", tracks->frontmat[i][j]);
            }
            printf ("\n");
        }   
    }
    if (tracks->frontmat_total) {
        printf ("------- front matrix total-------\n");
        int size = 4*tracks->nsensors;
        for (int i=0;i<size;i++) {
            for (int j=0;j<size;j++) {
                printf ("%d ", tracks->frontmat_total[i][j]);
            }
            printf ("\n");
        }   
    }
    return 0;
}

/* remove tracks that are too old
*/
int tracker_data_remove_outdated (tracker2_data_t *tracks, int64_t utime, int max_delta_sec)
{
    GQueue *new_data = g_queue_new ();
    int max_delta_usec = max_delta_sec * 1000000;
    int count=0;

    for (GList *iter=g_queue_peek_head_link (tracks->data);iter;iter=iter->next) {
        track2_t *t = (track2_t*)iter->data;
        assert (!g_queue_is_empty (t->data));
        navlcm_feature_t *f = (navlcm_feature_t*)g_queue_peek_head (t->data);
        if (utime - f->utime < max_delta_usec) { 
            g_queue_push_tail (new_data, t);
        } else {
            track2_t_destroy (t);
            count++;
        }
    }

    g_queue_free (tracks->data);

    tracks->data = new_data;

    FILE *fp = fopen ("death_count.txt", "a");
    fprintf (fp, "%d\n", count);
    fclose (fp);

    return 0;
}

int tracker_data_update (tracker2_data_t *tracks, navlcm_feature_list_t *fnext, int nsensors)
{
    /* convert tracks to list of features */
    navlcm_feature_list_t *fprev = (navlcm_feature_list_t*)malloc (sizeof(navlcm_feature_list_t));

    tracks_head_features (tracks, fprev);

    fprev->width = fnext->width;
    fprev->height = fnext->height;

    for (int i=0;i<fnext->num;i++)
        fnext->el[i].index = i;

    /* init tracks */
    if (fprev->num == 0) {
        tracker_data_init_from_data (tracks, fnext, nsensors);
        return 0;
    }

    dbg (DBG_CLASS, "[tracker] matching %d vs %d features.", fprev->num, fnext->num);

    /* match features */
    navlcm_feature_match_set_t *matches = (navlcm_feature_match_set_t*)malloc(sizeof(navlcm_feature_match_set_t));

    if (tracks->frontmat_training) {
        find_feature_matches_fast_full (fprev, fnext, TRUE, TRUE, FALSE, .8, 30, to_degrees (40.0), matches, NULL, nsensors, NULL, NULL);
        tracks_train_step (tracks, matches, fprev->width, fprev->height, nsensors);
    } else {
        find_feature_matches_fast_full (fprev, fnext, TRUE, TRUE, FALSE, .8, 30, to_degrees (40.0), matches, NULL, nsensors, NULL, NULL);
    }

    /* update tracks*/
    int matched[fnext->num];
    for (int i=0;i<fnext->num;i++)
        matched[i] = 0;

    for (int i=0;i<matches->num;i++) {
        navlcm_feature_t *f1 = &matches->el[i].src;
        navlcm_feature_t *f2 = matches->el[i].dst;
        track2_t *t = (track2_t*)g_queue_peek_nth (tracks->data, f1->index);
        track_insert_feature (t, f2);
        matched[f2->index] = 1;
    }

    /* remove outdated tracks */
    tracker_data_remove_outdated (tracks, fnext->utime, 2);

    /* create new tracks */
    int count=0;
    for (int i=0;i<fnext->num;i++) {
        if (!matched[i]) {
            navlcm_feature_t *f = fnext->el + i;
            tracker_data_add_track (tracks, f);
            count++;
        }
    }
    dbg (DBG_CLASS, "[tracker] added %d (%d) new tracks.", count, fprev->num);

    /* free */
    navlcm_feature_match_set_t_destroy (matches);
    navlcm_feature_list_t_destroy (fprev);

    return 0;
}


