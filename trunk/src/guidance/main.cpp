/* The classifier implements the rotation guidance algorithm
 * and the loop closing algorithm. The module takes as input feature sets
 * output by the collector. A feature set is composed of several feature list
 * (one list per type: sift, fast, etc.) but in practice, we use SIFT features only.
 */

#define CLASS_USE_BAYES 1
#define PHI_THRESH 40.0
#define PSI_THRESH .80

#include "main.h"

/* get the n-th feature set in memory
 */
navlcm_feature_list_t *get_nth_features (int n)
{
    state_t *self = g_self;

    if (!self->feature_list) return NULL;

    gpointer data = g_queue_peek_nth (self->feature_list, n);

    if (!data) return NULL;

    return (navlcm_feature_list_t*)data;
}

/* get the utime of the last feature set received
 */
int64_t get_features_utime (state_t *self)
{
    if (!self->feature_list) return 0;

    navlcm_feature_list_t *set = get_nth_features (0);

    if (!set) return 0;

    return set->utime;
}

int64_t get_image_utime (state_t *self)
{
    GQueue *data = self->camimg_queue[0];
    if (data && !g_queue_is_empty (data)) {
        return ((botlcm_image_t*)(g_queue_peek_head (data)))->utime;
    }
    return 0;
}

/* utter text message
 */
void utter_message (state_t *self, int msg_id)
{
    if (bot_timestamp_now () - self->last_utterance_utime < 2000000)
        return;

    char msg[50];
    sprintf (msg, "%d", msg_id);
    navlcm_audio_param_t ap;
    ap.code = AUDIO_PLAY;
    ap.name = msg;
    ap.val = 0.;
    navlcm_audio_param_t_publish (self->lcm, "AUDIO_PARAM", &ap);

    self->last_utterance_utime = bot_timestamp_now ();
}

void utter_message_text (state_t *self, const char *txt)
{
    if (bot_timestamp_now () - self->last_utterance_utime < 2000000)
        return;

    char msg[50];
    navlcm_audio_param_t ap;
    ap.code = AUDIO_SAY;
    ap.name = (char*)txt;
    ap.val = 0.;
    navlcm_audio_param_t_publish (self->lcm, "AUDIO_PARAM", &ap);

    self->last_utterance_utime = bot_timestamp_now ();
}

/* utter directions
*/
void utter_directions (state_t *self, double angle_rad)
{
    int64_t utime = bot_timestamp_now ();

    // do not play audio to often
    if (self->last_audio_utime  > 0 && utime - self->last_audio_utime < 2000000)
        return;

    self->last_audio_utime = utime;

    double angle = 180.0 * clip_value (angle_rad, -PI, PI, 1E-6) / M_PI;

    char msg[10];
    if (angle > 0) {
        if (angle < 30.0) {
            sprintf (msg, "%d", 4);
        } else if (angle < 60.0) {
            sprintf (msg, "%d", 2);
        } else if (angle < 160.0) {
            sprintf (msg, "%d", 0);
        } else {
            sprintf (msg, "%d", 5);
        }
    } else {
        angle = -angle;
        if (angle < 30.0) {
            sprintf (msg, "%d", 4);
        } else if (angle < 60.0) {
            sprintf (msg, "%d", 3);
        } else if (angle < 160.0) {
            sprintf (msg, "%d", 1);
        } else {
            sprintf (msg, "%d", 5);
        }
    }

    navlcm_audio_param_t ap;
    ap.code = AUDIO_PLAY;
    ap.name = msg;
    ap.val = 0.;
    navlcm_audio_param_t_publish (self->lcm, "AUDIO_PARAM", &ap);
}

static void on_gps_to_local_event (const lcm_recv_buf_t *buf, const char *channel, const navlcm_gps_to_local_t *msg, void *user)
{
    state_t *self = (state_t*)user;

    if (self->gps_to_local)
        navlcm_gps_to_local_t_destroy (self->gps_to_local);

    self->gps_to_local = navlcm_gps_to_local_t_copy (msg);

    // clip value in [-180, 180]
    self->gps_to_local->lat_lon_el_theta[0] = clip_value (self->gps_to_local->lat_lon_el_theta[0] , -180, 180, 1E-6);
    self->gps_to_local->lat_lon_el_theta[1] = clip_value (self->gps_to_local->lat_lon_el_theta[1] , -180, 180, 1E-6);

}

static void on_applanix_event (const lcm_recv_buf_t *buf, const char *channel, const botlcm_raw_t *msg, void *user)
{
    state_t *self = (state_t*)user;
    applanix_data_t ad;
    if (applanix_decode (&ad, msg->data, msg->length) < 0) {
        fprintf (stderr, "Failed to decode applanix packet\n");
        return;
    }

    if (!self->applanix_data)
        self->applanix_data = g_queue_new ();

    nav_applanix_data_t *d = (nav_applanix_data_t*)malloc(sizeof(nav_applanix_data_t));
    d->utime = msg->utime;
    d->heading = ad.grp1.heading;

    g_queue_push_head (self->applanix_data, d);

    // applanix frame rate is ~ 300Hz
    if (g_queue_get_length (self->applanix_data) > 10000) {
        nav_applanix_data_t *t = (nav_applanix_data_t *)(g_queue_peek_tail (self->applanix_data));
        g_queue_pop_tail (self->applanix_data);
        free (t);
    }

    //    printf ("applanix data: %d el.\n", g_queue_get_length (self->applanix_data));
}

static void on_imu_event (const lcm_recv_buf_t *buf, const char *channel, const navlcm_imu_t *msg, void *user)
{
    state_t *self = (state_t*)user;

    g_queue_push_head (self->imu_queue, navlcm_imu_t_copy (msg));

}
static void on_pose_event (const lcm_recv_buf_t *buf, const char *channel, const botlcm_pose_t *msg, void *user)
{
    state_t *self = (state_t*)user;

    g_queue_push_head (self->pose_queue, botlcm_pose_t_copy (msg));

    if (self->param->mode != NAVLCM_CLASS_PARAM_T_CALIBRATION_MODE) {
        if (g_queue_get_length (self->pose_queue) > MAX_POSES) {
            botlcm_pose_t *p = (botlcm_pose_t*)g_queue_pop_tail (self->pose_queue);
            botlcm_pose_t_destroy (p);
        }
    }
}

void set_class_param (state_t *self, const navlcm_class_param_t *msg)
{
    // copy
    if (self->param)
        navlcm_class_param_t_destroy (self->param);

    self->param = navlcm_class_param_t_copy (msg);
}

void reset_data (state_t *self)
{
    dbg (DBG_CLASS, "[class] reseting data...");

    // lock mutex
    g_mutex_lock (self->data_mutex);

    // erase feature keys
    while (!g_queue_is_empty (self->feature_list)) {
        navlcm_feature_list_t *fs = (navlcm_feature_list_t*)g_queue_peek_head (self->feature_list);
        navlcm_feature_list_t_destroy (fs);
        g_queue_pop_head (self->feature_list);
    }

    // unlock mutex
    g_mutex_unlock (self->data_mutex);

}

/* update topological path
*/
void update_path (state_t *self, dijk_node_t *src, dijk_node_t *dst)
{
    if (self->path)
        g_queue_free (self->path);
    self->path = NULL;

    self->path = dijk_find_shortest_path (self->d_graph, src, dst);

    if (!self->path || g_queue_is_empty (self->path)) {
        dbg (DBG_ERROR, "empty path between nodes %d and %d on update path.", src->uid, dst->uid);
        return;
    }

    self->current_node = src;
    self->current_edge = (dijk_edge_t*)g_queue_peek_head (self->path);

    if (self->param->path)
        free (self->param->path);
    self->param->path = NULL;
    self->param->path_size = 0;

    for (GList *iter=g_queue_peek_head_link (self->path);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;
        self->param->path = (int*)realloc(self->param->path, (self->param->path_size+1)*sizeof(int));
        self->param->path[self->param->path_size] = e->start->uid;
        self->param->path_size++;
    }

    self->param->nodeid_end = dst->uid;

}

gboolean force_node (state_t *self, int id)
{
    self->param->mode = NAVLCM_CLASS_PARAM_T_NAVIGATION_MODE;
    self->param->mission_success = 0;

    dijk_node_t *n = dijk_graph_find_node_by_id (self->d_graph, id);

    if (!n) {
        dbg (DBG_ERROR, "failed to find node %d in graph", id);
        return FALSE;
    }

    // update path
    if (self->path && !g_queue_is_empty (self->path)) {
        dijk_edge_t *end = (dijk_edge_t*)g_queue_peek_tail (self->path);
        dijk_node_t *dst = end->end ? end->end : end->start;

        update_path (self, n, dst);

    } 


    self->current_node = n;
    self->current_edge = NULL;

    if (!g_queue_is_empty (self->path)) {
        self->current_edge = (dijk_edge_t*)g_queue_peek_head (self->path);
    } else {
        self->current_edge = dijk_get_nth_edge (n, 0);
    }

    assert (self->current_edge);

    // init the state
    state_init (self->d_graph, self->current_edge->start);

    printf ("forcing node %d effectively %d\n", id, self->current_node->uid);

    state_print (self->d_graph, self->current_node);

    // save node id in text file for gentle process restart
    FILE *fp = fopen ("start_id.txt", "w");
    fprintf (fp, "%d", id);
    fclose (fp);
}


    static void
on_generic_cmd_event (const lcm_recv_buf_t *buf, const char *channel, 
        const navlcm_generic_cmd_t *msg, 
        void *user)
{
    state_t *self = g_self;
    int id=0;

    if (msg->code == CLASS_RESET) {
        reset_data (self);
    } else if (msg->code == CLASS_FORCE_NODE) {
        force_node (self, atoi (msg->text));
    } else if (msg->code == CLASS_CHECKPOINT_REVISIT) {
        class_checkpoint_revisit (self, self->utime);
    } else if (msg->code == CLASS_CHECKPOINT_CREATE) {
        class_checkpoint_create (self, self->utime);
    } else if (msg->code == CLASS_SET_REFERENCE_POINT) {
        class_set_reference_point (self);
    } else if (msg->code == CLASS_SET_NODE_LABEL) {
        on_class_node_set_label (self, msg->text);
    } else if (msg->code == CLASS_GOTO_NODE) {
        int id;
        if (sscanf (msg->text, "%d", &id) == 1)
            class_goto_node (self, id);
    } else if (msg->code == CLASS_SET_UTIME1) {
        self->utime1 = self->utime;
    } else if (msg->code == CLASS_SET_UTIME2) {
        self->utime2 = self->utime;
    } else if (msg->code == CLASS_RUN_MATCHING_ANALYSIS) {
        class_run_matching_analysis (self->camimg_queue, self->config->nsensors, self->feature_list, self->utime1, self->utime2, self->lcm);
    } else if (msg->code == CLASS_CHECK_CALIBRATION) {
        printf ("yihaaaaaaaaaa!\n");
        on_class_check_calibration (self);
    } else if (msg->code == CLASS_STOP_CHECK_CALIBRATION) {
        self->param->mode = NAVLCM_CLASS_PARAM_T_STANDBY_MODE;
    } else if (msg->code == CLASS_START_EXPLORATION) {
        on_class_start_exploration (self);
    } else if (msg->code == CLASS_END_EXPLORATION) {
        on_class_end_exploration (self);
    } else if (msg->code == CLASS_ADD_NODE) {
        on_class_add_node (self);
    } else if (msg->code == CLASS_START_HOMING) {
        on_class_homing (self);
    } else if (msg->code == CLASS_START_REPLAY) {
        on_class_replay (self);
    } else if (msg->code == CLASS_BLIT_REQUEST) {
        on_blit_request (self);
    } else if (msg->code == CLASS_LOAD_MAP) {
        on_class_load_map (self, msg->text);
    } else if (msg->code == CLASS_STANDBY) {
        on_class_standby (self);
    } else if (msg->code == CLASS_DUMP_FEATURES) {
        on_class_dump_features (self);
    } else if (msg->code == CLASS_CALIBRATION_STEP) {
        on_class_calibration_step (self, atoi (msg->text));
    } else if (msg->code == CLASS_USER_WHERE_NEXT) {
        on_class_user_where_next (self);
    }
    else if (msg->code == CLASS_USER_LOST) {
        // do nothing. we just log this event.
        publish_phone_msg (self->lcm, "Problem reported.");
    }
    else if (msg->code == CLASS_USER_UNCLEAR_GUIDANCE) {
        // do nothing. we just log this event.
        publish_phone_msg (self->lcm, "Unclear guidance reported.");
    }


    dbg (DBG_CLASS, "[class] classifier is in mode %d", self->param->mode);

    return;
}


void on_class_user_where_next (state_t *self)
{
    if (self->user_where_next_utime > 0 && self->utime - self->user_where_next_utime < 4000000)
        return;

    if (self->param->mode == NAVLCM_CLASS_PARAM_T_NAVIGATION_MODE) {

        // force next edge in the path
        if (self->current_edge && self->current_edge->end)
            force_node (self, self->current_edge->end->uid);

        self->user_where_next_utime = self->utime;
    }
}


void on_class_calibration_step (state_t *self, int step)
{   
    dbg (DBG_CLASS, "Calibration step: %d", step);

    // switch mode
    self->param->mode = NAVLCM_CLASS_PARAM_T_CALIBRATION_MODE;

    self->calibration_step = step;

    if (step != 0) { 
        // start a thread to run calibration
        GThread *t = g_thread_create (calibration_thread_cb, self, FALSE, NULL);

    } else {
        // clear data
        reset_data (self);
    }
}

/* dump features to file
*/
void on_class_dump_features (state_t *self)
{
    if (g_queue_is_empty (self->feature_list))
        return;

    navlcm_feature_list_t *fs = (navlcm_feature_list_t*)g_queue_peek_head (self->feature_list);

    const char *filename = "features.dat";

    FILE *fp = fopen (filename, "a");

    for (int i=0;i<fs->num;i++) {
        navlcm_feature_t *f = fs->el + i;
        for (int j=0;j<f->size;j++) 
            fprintf (fp, "%.4f ", f->data[j]);
        fprintf (fp, "\n");
    }

    fclose (fp);

    dbg (DBG_CLASS, "dumped features to %s", filename);
}


/* standby
*/
void on_class_standby (state_t *self)
{
    self->param->mode = NAVLCM_CLASS_PARAM_T_STANDBY_MODE;

    publish_phone_msg (self->lcm, "Waiting for mission assignment.");

}

/* load a map
*/
void on_class_load_map (state_t *self, char *name)
{
    if (file_exists (name)) {

        dbg (DBG_CLASS, "Loading map %s", name);

        if (self->param->map_filename)
            free (self->param->map_filename);
        self->param->map_filename = strdup (name);

        if (self->d_graph)
            dijk_graph_destroy (self->d_graph);

        self->d_graph = dijk_graph_new ();

        dijk_graph_read_from_file (self->d_graph, self->param->map_filename);

        self->current_node = NULL;
        self->current_edge = NULL;

        publish_phone_msg (self->lcm, "Loaded %s (%d nodes)", name, dijk_graph_n_nodes (self->d_graph));
    }
    else {
        dbg (DBG_ERROR, "Map %s does not exist.", name);
    }
}

/* user requested homing
*/
void on_class_homing (state_t *self)
{
    if (!self->d_graph)
        return;

    self->param->mode = NAVLCM_CLASS_PARAM_T_NAVIGATION_MODE;
    self->param->mission_success = 0;
    self->param->wrong_way = 0;

    // assume we are at the last gate

    force_node (self, ((dijk_node_t*)g_queue_peek_tail (self->d_graph->nodes))->uid);

    if (!self->current_edge) {
        dbg (DBG_ERROR, "failed to set source gate on homing.");
        return;
    }

    dijk_node_t *dst = dijk_graph_find_node_by_id (self->d_graph, 0);

    update_path (self, self->current_node, dst);

    publish_phone_msg (self->lcm, "Mission started. Go to Navigation tab.");
}

/* user requested replay
*/
void on_class_replay (state_t *self)
{
    if (!self->d_graph)
        return;

    self->param->mode = NAVLCM_CLASS_PARAM_T_NAVIGATION_MODE;
    self->param->mission_success = 0;
    self->param->wrong_way = 0;

    // assume that we are at gate 0
    force_node (self, 0);

    if (!self->current_edge) {
        dbg (DBG_ERROR, "failed to set source gate on replay.");
        return;
    }

    // compute new path
    dijk_node_t *dst = (dijk_node_t*)g_queue_peek_tail (self->d_graph->nodes);

    update_path (self, self->current_node, dst);

    publish_phone_msg (self->lcm, "Mission started. Go to Navigation tab.");
}

void create_new_node (state_t *self, gboolean checkpoint)
{
    // have we received an image on each channel?
    for (int i=0;i<self->config->nsensors;i++) {
        if (g_queue_is_empty (self->camimg_queue[i]))
            return;
    }

    navlcm_feature_list_t *f = get_nth_features (0);
    if (!f)
        return;

    GQueue *img = g_queue_new ();
    for (int i=0;i<self->config->nsensors;i++) {
        g_queue_push_tail (img, (botlcm_image_t*)g_queue_peek_head (self->camimg_queue[i]));
    }

    botlcm_image_t *up_img = NULL;
    if (!g_queue_is_empty (self->upward_image_queue)) {
        up_img = (botlcm_image_t*)g_queue_peek_head (self->upward_image_queue);
    }

    navlcm_gps_to_local_t *gps = NULL;
    if (self->gps_to_local)
        gps = navlcm_gps_to_local_t_copy (self->gps_to_local);
    else
        gps = navlcm_gps_to_local_t_new (f->utime);

    botlcm_pose_t *pose = NULL;
    if (!g_queue_is_empty (self->pose_queue))
        pose = (botlcm_pose_t*)g_queue_peek_head (self->pose_queue);

    int nnodes = g_queue_get_length (self->d_graph->nodes);
    printf ("*****************  NEW NODE %d *******************\n", nnodes);

    int motion_type = motion_classifier_histogram_voting (self->mc, 0);

    motion_classifier_clear_history (self->mc, 0);

    dijk_graph_add_new_node (self->d_graph, f, img, up_img, pose, gps, checkpoint, self->current_edge, motion_type);

    self->current_edge = dijk_graph_latest_edge (self->d_graph);

    self->param->nodeid_now = self->current_edge->start ? self->current_edge->start->uid : -1;

#if 0
    // update sim. matrix
    self->corrmat = loop_update_full (self->voctree, self->corrmat, &self->corrmat_size, 
            f, nnodes, BAGS_WORD_RADIUS);
    corrmat_write (self->corrmat, self->corrmat_size, "corrmat.dat");

    // publish sim. matrix
    navlcm_dictionary_t *dict = corrmat_to_dictionary (self->corrmat, self->corrmat_size);
    navlcm_dictionary_t_publish (self->lcm, "SIMILARITY_MATRIX", dict);
    navlcm_dictionary_t_destroy (dict);

    // detect loop closures
    dijk_graph_t *d_graph = dijk_graph_copy (self->d_graph);
    process_correlation_matrix (self, "corrmat.dat", d_graph);

    // publish the UI map
    navlcm_ui_map_t *mp = dijk_graph_to_ui_map (d_graph, "neato");
    navlcm_ui_map_t_publish (self->lcm, "UI_MAP_LOOP", mp);
    navlcm_ui_map_t_destroy (mp);
    char filename[128];
    sprintf (filename, "graph-%d.ps", nnodes);
    dijk_graph_layout_to_file (d_graph, "neato", "ps", filename, -1);
    dijk_graph_destroy (d_graph);
#endif

    // save graph to file
    dbg (DBG_CLASS, "saving graph to file %s", self->param->map_filename);
    dijk_graph_write_to_file (self->d_graph, self->param->map_filename);

    // save graph images to files
    dijk_save_images_to_file (self->d_graph, self->nodes_snap_dir, FALSE);

    // free data
    g_queue_free (img);
    navlcm_gps_to_local_t_destroy (gps);
}

/* manually add a node
*/
void on_class_add_node (state_t *self)
{

    if (!self->param->mode == NAVLCM_CLASS_PARAM_T_EXPLORATION_MODE)
        return;

    create_new_node (self, FALSE);

}

/* start a new exploration
*/
void on_class_start_exploration (state_t *self)
{
    self->param->mode = NAVLCM_CLASS_PARAM_T_EXPLORATION_MODE;

    // next gates filename
    if (self->param->map_filename)
        free (self->param->map_filename);
    self->param->map_filename = class_next_map_filename ();

    // reset graph
    if (self->d_graph)
        dijk_graph_destroy (self->d_graph);

    self->d_graph = dijk_graph_new ();

    dbg (DBG_CLASS, "Exploration started\n");

    publish_phone_msg (self->lcm, "Exploration started.");
}

/* end current exploration
*/
void on_class_end_exploration (state_t *self)
{
    self->param->mode = NAVLCM_CLASS_PARAM_T_STANDBY_MODE;

    publish_phone_msg (self->lcm, "Exploration ended.");
}

/* 
*/
void on_class_node_set_label (state_t *self, char *txt)
{
    int id;
    sscanf (txt, "%d", &id);
    char *label = txt;
    while (*label != ' ') label++;
    label++;

    dijk_node_t *nd = dijk_graph_find_node_by_id (self->d_graph, id);

    if (nd) {
        dijk_node_set_label (nd, label);

        // utter the message
        utter_message_text (self, label);

        dbg (DBG_CLASS, "assigned label %s to node %d", label, id);
    } else {
        dbg (DBG_ERROR, "could not find node %d for label assignment.", id);
    }
}

/* 
*/
void on_class_check_calibration (state_t *self)
{
    self->param->mode = NAVLCM_CLASS_PARAM_T_CALIBRATION_CHECK_MODE;

    // save a copy of the latest features received
    if (self->ref_point_features)
        navlcm_feature_list_t_destroy (self->ref_point_features);
    self->ref_point_features = NULL;
    if (!g_queue_is_empty (self->feature_list)) {
        self->ref_point_features = navlcm_feature_list_t_copy ((navlcm_feature_list_t*)g_queue_peek_head (self->feature_list));
    }
}

/* when a gate has been set as a target
*/
void class_goto_node (state_t *self, int id)
{
    self->param->mode = NAVLCM_CLASS_PARAM_T_NAVIGATION_MODE;
    self->param->mission_success = 0;

    if (!self->d_graph || !self->current_node)
        return;

    dbg (DBG_CLASS, "******************  Going to node %d ********************* \n", id);

    dijk_node_t *dst = dijk_graph_find_node_by_id (self->d_graph, id);

    if (!dst) {
        dbg (DBG_ERROR, "failed to find node %d in graph.", id);
        return;
    }

    // save node id to text file for gentle process restart
    FILE *fp = fopen ("end_id.txt", "w");
    fprintf (fp, "%d", id);
    fclose (fp);

    update_path (self, self->current_node, dst);
}

/* save features in a buffer
*/
    static void
on_feature_list_event (const lcm_recv_buf_t *buf, const char *channel, const navlcm_feature_list_t *msg, void *user)
{
    state_t *self = (state_t*)user;

    if (self->computing) {
        dbg (DBG_ERROR, "still computing. skipping features...");
        return;
    }

    self->features_width = msg->width;
    self->features_height = msg->height;

    // fetch latest features
    navlcm_feature_list_t *last_f = NULL;
    if (!g_queue_is_empty (self->feature_list))
        last_f =  (navlcm_feature_list_t*)g_queue_peek_head (self->feature_list);

    // take mutex
    g_mutex_lock (self->data_mutex);

    // add to buffer
    g_queue_push_head (self->feature_list, navlcm_feature_list_t_copy (msg));

    // resize buffer (except in calibration mode)
    if (self->param->mode != NAVLCM_CLASS_PARAM_T_CALIBRATION_MODE && 
            self->param->mode != NAVLCM_CLASS_PARAM_T_CALIBRATION_CHECK_MODE) {
        if (g_queue_get_length (self->feature_list) > BUFFSIZE) {
            navlcm_feature_list_t *tail = (navlcm_feature_list_t*)g_queue_pop_tail (self->feature_list);
            navlcm_feature_list_t_destroy (tail);
        }
    }

    dbg (DBG_CLASS, "buffer has %d feature sets.", g_queue_get_length (self->feature_list));

    // release mutex
    g_mutex_unlock (self->data_mutex);

    // trigger computation (except in training mode)
    if (self->param->mode != NAVLCM_CLASS_PARAM_T_CALIBRATION_MODE) {
        g_mutex_lock (self->compute_mutex);
        g_cond_signal (self->compute_cond);
        g_mutex_unlock (self->compute_mutex);
    }

    return;
}

gpointer calibration_thread_cb (gpointer data)
{
    state_t *self = (state_t*)data;
    self->computing = TRUE;
    publish_phone_msg (self->lcm, "Running calibration. Please wait...");
    int status = class_calibration (self->feature_list, self->config, self->calibration_step);
    if (!status)
        publish_phone_msg (self->lcm, "Calibration done.");
    else
        publish_phone_msg (self->lcm, "Calibration failed. Please start again.");
    self->computing = FALSE;

    // clear data
    reset_data (self);

    return NULL;
}

gpointer compute_thread_cb (gpointer data)
{
    state_t *self = (state_t*)data;

    while (!self->exit) {

        // wait for signal
        g_mutex_lock (self->compute_mutex);
        g_cond_wait (self->compute_cond, self->compute_mutex);
        g_mutex_unlock (self->compute_mutex);

        if (self->exit) {
            break;
        }

        // take the computing flag 
        self->computing = TRUE; 

        printf ("mode: %d\n", self->param->mode);

        // call relevant routines
        if (self->param->mode == NAVLCM_CLASS_PARAM_T_EXPLORATION_MODE) {
            node_trigger_timeout_thread_func(self);
        } 

        if (self->param->mode == NAVLCM_CLASS_PARAM_T_NAVIGATION_MODE) {
            // local node estimation
            node_estimation_cb (self);

            // rotation guidance
            rotation_guidance_cb (self);
        }

        if (self->param->mode == NAVLCM_CLASS_PARAM_T_CALIBRATION_CHECK_MODE) {
            // calibration check
            calibration_check_cb (self);
        }

        // motion classifier update
        if (self->param->mode == NAVLCM_CLASS_PARAM_T_EXPLORATION_MODE) {
            motion_classifier_update_cb (self);
        }

        // release the computing flag
        self->computing = FALSE;

    }

    return NULL;
}

// this method is called upon CLASS_REVISIT
//
void class_checkpoint_revisit (state_t *self, int64_t utime)
{
    // request ground truth comparison
    if (self->gt_request_utime != 0) {
        dbg (DBG_ERROR, "warning: previous ground truth request not fully processed...");
    }

    dbg (DBG_ERROR, "received ground truth request for utime %ld", utime);

    self->gt_request_utime = utime;

}
void class_checkpoint_create (state_t *self, int64_t utime)
{
    g_mutex_lock (self->data_mutex);

    // force creation of a new node 
    if (self->param->mode == NAVLCM_CLASS_PARAM_T_EXPLORATION_MODE) {
        dbg (DBG_ERROR, "checkpoint. forcing new node.");
        create_new_node (self, TRUE);
    }

    g_mutex_unlock (self->data_mutex);

    self->gt_request_utime = utime;
}

void class_set_reference_point (state_t *self)
{
    if (self->ref_point_features)
        navlcm_feature_list_t_destroy (self->ref_point_features);

    self->ref_point_features = NULL;

    dbg (DBG_CLASS, "setting reference point...");

    if (get_nth_features (0)) {
        self->ref_point_features = navlcm_feature_list_t_copy (get_nth_features (0));
        publish_phone_msg (self->lcm, "Set reference point: OK");
    } else {
        publish_phone_msg (self->lcm, "Set reference point: NOK (no features)");
    }
}

/* compute ground truth orientation from upward camera images
*/
int compute_ground_truth (state_t *self, botlcm_image_t *img1, botlcm_image_t *img2, double *angle_rad) 
{
    if (!img1 || !img2) return -1;

    // compute ground truth from IMU
    botlcm_pose_t *p1 = find_pose_by_utime (self->pose_queue, img1->utime);
    botlcm_pose_t *p2 = find_pose_by_utime (self->pose_queue, img2->utime);
    if (p1 && p2) {

        double angle = class_compute_pose_rotation (p1, p2);
        dbg (DBG_CLASS, "IMU rotation: %.4f deg.", angle * 180.0 / PI);
        printf ("%.4f ", angle *180/PI);
    } 

    dbg (DBG_VIEWER, "computing ground truth. img1 : %ld", img1->utime);
    dbg (DBG_VIEWER, "                        img2 : %ld", img2->utime);

    /*
    // compute rotation estimation
    double stdev = 0.0;
    return rot_estimate_angle_from_images (img1, img2, self->config->upward_calib_fc, 
    self->config->upward_calib_cc, self->config->upward_calib_kc, angle_rad, &stdev);
    */
    return 0;
}

void* ground_truth_thread_cb (void* data)
{
    state_t *self = (state_t*)data; 

    if (g_queue_get_length (self->upward_image_queue) < 3)
        return NULL;

    self->ground_thread_running = TRUE;


    botlcm_image_t *img1 = (botlcm_image_t*)((g_queue_peek_tail_link (self->upward_image_queue)->prev)->data);
    botlcm_image_t *img2 = (botlcm_image_t*)((g_queue_peek_head_link (self->upward_image_queue)->next)->data);
    double angle_rad;
    compute_ground_truth (self, img1, img2, &angle_rad);
    self->ground_thread_running = FALSE;
    return NULL;
}

    static void
on_upward_image_event (const lcm_recv_buf_t *buf, const char *channel, const botlcm_image_t *msg, void *user)
{
    state_t *self = (state_t*)user;

    return;

    // add the image to the queue
    g_queue_push_head (self->upward_image_queue, botlcm_image_t_copy (msg));

    if (g_queue_get_length (self->upward_image_queue) > 30) {
        botlcm_image_t *im = (botlcm_image_t*)g_queue_pop_tail (self->upward_image_queue);
        botlcm_image_t_destroy (im);
    }

    if (self->ground_thread_running) {
        return;
    }

    // compute rotation between first image and latest image
    g_thread_create (ground_truth_thread_cb, self, FALSE, NULL);
}


    static void
on_botlcm_image_event (const lcm_recv_buf_t *buf, const char *channel, 
        const botlcm_image_t *msg, void *user)
{
    state_t *self = g_self;

    int sensorid = find_string (channel, (const char**)self->config->channel_names, 
            self->config->nsensors);

    if (self->config->nsensors - 1 < sensorid)
        assert (false);

    self->utime = msg->utime;
    self->image_width = msg->width;
    self->image_height = msg->height;

    // fetch latest image
    botlcm_image_t *last_img = NULL;
    if (!g_queue_is_empty (self->camimg_queue[sensorid]))
        last_img = (botlcm_image_t*)g_queue_peek_head (self->camimg_queue[sensorid]);

    // store the image in local memory
    g_queue_push_head (self->camimg_queue[sensorid], botlcm_image_t_copy (msg));
    if (g_queue_get_length (self->camimg_queue[sensorid]) > 100) {
        botlcm_image_t *img = (botlcm_image_t*)g_queue_pop_tail (self->camimg_queue[sensorid]);
        botlcm_image_t_destroy (img);
    }

    // store image in file
    if (self->save_images) {
        char *dirname = (char*)"frames";
        if (!dir_exists (dirname))
            create_dir (dirname);
        if (dir_exists (dirname)) {
            char fname[512];
            sprintf (fname, "%s/%ld-%d.pgm", dirname, (long int)0/*msg->utime*/, sensorid);
            write_pgm (msg->data, msg->width, msg->height, fname);
        } else {
            dbg (DBG_ERROR, "failed to create directory %s", dirname);
        }
    }

    GTimer *timer = g_timer_new ();
    /* compute optical flow and update flow field
    */
    botlcm_image_t *prev = botlcm_image_t_find_by_utime (self->camimg_queue[sensorid], msg->utime - self->flow_integration_time);
    if ((self->param->mode == NAVLCM_CLASS_PARAM_T_EXPLORATION_MODE ||
                self->param->mode == NAVLCM_CLASS_PARAM_T_FLOW_CALIBRATION_MODE) && prev && self->flow_scale > 0) {

        flow_t *flow = flow_compute (prev->data, msg->data, msg->width, msg->height, prev->utime, msg->utime, self->flow_scale, NULL);

        g_queue_push_tail (self->flow_queue[sensorid], flow);

        // infinite average for flow calibration
        if (self->param->mode == NAVLCM_CLASS_PARAM_T_FLOW_CALIBRATION_MODE)
            flow_field_update_from_flow (&self->flow_field_set->fields[sensorid], flow);

        // apply sliding window
        int64_t utime1 = ((flow_t*)g_queue_peek_head(self->flow_queue[sensorid]))->utime1;
        int64_t utime2 = ((flow_t*)g_queue_peek_tail(self->flow_queue[sensorid]))->utime1;
        if (utime2 - utime1 > 4 * self->flow_integration_time && g_queue_get_length (self->flow_queue[sensorid]) > 1) {
            flow_t *prev_flow = (flow_t*)g_queue_pop_head (self->flow_queue[sensorid]);
            flow_t_destroy (prev_flow);
        }

        // finite-window average
        if (self->param->mode != NAVLCM_CLASS_PARAM_T_FLOW_CALIBRATION_MODE) {
            flow_field_reset (&self->flow_field_set->fields[sensorid]);

            for (GList *iter=g_queue_peek_head_link (self->flow_queue[sensorid]);iter;iter=iter->next) {
                flow_t *f = (flow_t*)iter->data;
                flow_field_update_from_flow (&self->flow_field_set->fields[sensorid], f);
            }
        }

        // save field to file
        if (self->param->mode == NAVLCM_CLASS_PARAM_T_FLOW_CALIBRATION_MODE) {
            FILE *fp = fopen (self->flow_field_filename, "wb");
            if (fp) {
                flow_field_set_write_to_file (self->flow_field_set, fp);
                fclose (fp);
            }
        }
    }

    double secs = g_timer_elapsed (timer, NULL);
    //    dbg (DBG_CLASS, "image timer: %.4f secs. (%.2f Hz)", secs, 1.0/secs);
    g_timer_destroy (timer);
}

/* user requested images
*/
void on_blit_request (state_t *self)
{
    for (int i=0;i<self->config->nsensors;i++) {
        if (self->param->mode == NAVLCM_CLASS_PARAM_T_EXPLORATION_MODE) {
            if (!g_queue_is_empty (self->camimg_queue[i])) {
                botlcm_image_t *img = (botlcm_image_t*)g_queue_peek_head (self->camimg_queue[i]);
                // send to phone
                char channel[20];
                sprintf (channel, "PHONE_THUMB%d", i);
                botlcm_image_t_publish (self->lcm, channel, img);
            }
        }
        if (self->param->mode == NAVLCM_CLASS_PARAM_T_NAVIGATION_MODE) {
            if (self->current_edge) {
                botlcm_image_t *img = (botlcm_image_t*)g_queue_peek_nth (self->current_edge->img, i);
                // send to phone
                char channel[20];
                sprintf (channel, "PHONE_THUMB%d", i);
                botlcm_image_t_publish (self->lcm, channel, img);
            }
        }
    }
}

static double g_psi_buffer[10] = {0,0,0,0,0,0,0,0,0,0};

/* return 1 if a gate is triggered, 0 otherwise
*/
static int g_psi_count = 0;

gboolean node_trigger_on_psi_distance (state_t *self, navlcm_feature_list_t *features)
{
    if (!self->current_edge)
        return 0;

    GTimer *timer = g_timer_new ();

    // compute psi-distance
    double psi_dist = class_psi_distance (features, self->current_edge->features, self->lcm);

    //FILE *fp = fopen ("psi.txt", "a");
    //fprintf (fp, "%d %.5f\n", g_psi_count, psi_dist);
    //fclose (fp);
    //g_psi_count++;

    // filter
    for (int i=0;i<9;i++) 
        g_psi_buffer[i] = g_psi_buffer[i+1];
    g_psi_buffer[9] = psi_dist;

    double mean_psi_dist = math_mean_double (g_psi_buffer, 10);

    double time_dist = ((int)(features->utime-self->current_edge->features->utime))/1000000.0;
    dbg (DBG_CLASS, "mean psi distance (%d nodes): %.3f (mean: %.3f) (thresh: %.3f) (time dist.: %.3f secs.)", 
            dijk_graph_n_nodes (self->d_graph), psi_dist, mean_psi_dist, PSI_THRESH, time_dist);

    double secs = g_timer_elapsed (timer, NULL);

    dbg (DBG_CLASS, "place graph generation timer: %.3f secs (%.1f Hz) for %d features", secs, 1.0/secs, features->num);
    
    g_timer_destroy (timer);

    self->param->psi_distance = mean_psi_dist;
    self->param->psi_distance_thresh = PSI_THRESH;

    if (mean_psi_dist > PSI_THRESH)
        return 1;

    return 0;
}

static int g_body_rotation_count = 0;

gpointer node_trigger_timeout_thread_func (gpointer data)
{
    state_t *self = (state_t*)data;

    // create a new node if none exists
    if (g_queue_is_empty (self->d_graph->nodes)) {
        create_new_node (self, FALSE);
        return NULL;
    }

    if (!self->current_edge)
        return NULL;

    assert (self->current_edge);

    // get the latest features
    navlcm_feature_list_t *fs = get_nth_features (0);
    if (!fs)
        return NULL;

    gboolean body_rotation = FALSE;

    // compute psi distance with latest node
    if (node_trigger_on_psi_distance (self, fs) || body_rotation) {

        // check time distance to latest node
        int64_t delta = fs->utime - self->current_edge->start->utime;
        if (delta < 1000000) {
            dbg (DBG_ERROR, "too early to create a new gate. delta = %.3f secs.", 1.0*delta/1000000);
            return NULL;
        }

        // create a new node
        create_new_node (self, body_rotation);

        utter_message (self, 6);

        // check gate filename
        if (!self->param->map_filename) {
            dbg (DBG_ERROR, "WARNING: gates are not being saved while in exploration mode.");
        }

    }

    /*
    // update tracker
    tracker_data_update (self->tracker, fs, self->config->nsensors);

    // print out tracker
    tracker_data_printout (self->tracker);

    // publish to LCM
    navlcm_track_set_t *lcmtracks = track_list_to_lcm (self->tracker);
    if (lcmtracks) {
    navlcm_track_set_t_publish (self->lcm, "TRACKS", lcmtracks);
    navlcm_track_set_t_destroy (lcmtracks);
    }
    */

    return NULL;
}

/* compute visual odometry
*/
void motion_classifier_update_cb (state_t *self)
{

    //flow_field_set_draw (self->flow_field_set, 400, 300, (char*)"flow.png");

    // publish flow for display
    navlcm_flow_t *nvf = flow_field_set_to_navlcm_flow (self->flow_field_set);
    navlcm_flow_t_publish (self->lcm, "FLOW_INSTANT", nvf);
    navlcm_flow_t_destroy (nvf);

    // update motion classifier
    GTimer *timer = g_timer_new();

    double *scores = flow_field_set_score_motions (self->flow_field_types, self->flow_field_set);

    motion_classifier_update (self->mc, scores);
    free (scores);

    int motion1, motion2;
    motion_classifier_extract_top_two_motions (self->mc, &motion1, &motion2);

    self->param->motion_type[0] = motion1;
    self->param->motion_type[1] = motion2;

    motion_classifier_write_top_two_motions (self->mc, "motion-class.bin", self->utime);

    motion_classifier_update_history (self->mc, motion1, motion2);

    double secs = g_timer_elapsed (timer, NULL);

    dbg (DBG_CLASS, "odometry timer: %.4f secs. (%.2f Hz) image size: %d x %d", secs, 1.0/secs, self->image_width, self->image_height);

    g_timer_destroy (timer);
}

/* send ui map to tablet
*/
void publish_ui_map (dijk_graph_t *dg, const char *channel, lcm_t *lcm)
{
    navlcm_ui_map_t *mp = dijk_graph_to_ui_map_basic (dg, "null");
    //navlcm_ui_map_t *mp = dijk_graph_to_ui_map (dg, "neato");

    if (mp) {
        navlcm_ui_map_t_publish (lcm, channel, mp);
        navlcm_ui_map_t_destroy (mp);
    }
}

gboolean update_future_direction_cb (gpointer data)
{
    state_t *self = (state_t*)data;

    if (self->param->mode == NAVLCM_CLASS_PARAM_T_NAVIGATION_MODE) {

        int motion_type;
        double secs, distance_m;

        if (self->path) {
            dijk_find_future_direction_in_path (self->path, 2, 15, self->future_directions, 3);
            self->param->next_direction = self->future_directions[0];
        }

        for (int i=0;i<3;i++) {
           navlcm_generic_cmd_t p;
           p.code = i;
           p.text = (char*)malloc(10);
           sprintf (p.text, "%d", self->future_directions[i]);
           navlcm_generic_cmd_t_publish (self->lcm, "FUTURE_DIRECTION", &p);
           free (p.text);
        }
    }

    return TRUE;
}

gboolean end_of_log_cb (gpointer data)
{
    state_t *self = (state_t*)data;

    // classifier versus IMU
    if (self->param->mode == NAVLCM_CLASS_PARAM_T_CALIBRATION_CHECK_MODE) {

        if (self->utime == self->end_of_log_utime) {

            printf ("end of log detected...%d x %d\n", self->features_width, self->features_height);

            // imu validation
            char filename[256];
            sprintf (filename, "imu-validation-%dx%d.txt", self->features_width, self->features_height);
            class_imu_validation (self->feature_list, NULL, self->pose_queue, self->config, filename);

            exit (0);

            return FALSE;
        }

        self->end_of_log_utime = self->utime;
    }

    // classifier training
    if (self->param->mode == NAVLCM_CLASS_PARAM_T_CALIBRATION_MODE) {

        if (self->utime == self->end_of_log_utime) {

            printf ("end of log detected...%d x %d\n", self->features_width, self->features_height);

            // imu validation
            class_calibration_rotation (self->feature_list, self->config, self->class_hist, self->lcm);

            exit (0);

            return FALSE;
        }

        self->end_of_log_utime = self->utime;
    }

    return TRUE;
}

gboolean publish_map_list_cb (gpointer data)
{
    state_t *self = (state_t*)data;

    util_publish_map_list (self->lcm, "map-", ".bin", "MAP_LIST");

    return TRUE;
}

gboolean publish_ui_images_cb (gpointer data)
{
    state_t *self = (state_t*)data;

    dijk_edge_t *edge = self->current_edge;

    if (self->current_edge && self->path) {
        GList *iter = g_queue_find (self->path, self->current_edge);
        if (iter->next)
            edge = (dijk_edge_t*)iter->next->data;
    }

    if (edge) {
        if (!edge->reverse) {
            botlcm_image_t *img_left = (botlcm_image_t*)g_queue_peek_nth (edge->img, 1);
            botlcm_image_t *img_right = (botlcm_image_t*)g_queue_peek_nth (edge->img, 2);
            util_publish_image (self->lcm, img_left, "PHONE_THUMB0", 282, 180);
            util_publish_image (self->lcm, img_right, "PHONE_THUMB1", 282, 180);
        } else {
            botlcm_image_t *img_left = (botlcm_image_t*)g_queue_peek_nth (edge->img, 3);
            botlcm_image_t *img_right = (botlcm_image_t*)g_queue_peek_nth (edge->img, 0);
            util_publish_image (self->lcm, img_left, "PHONE_THUMB0", 282, 180);
            util_publish_image (self->lcm, img_right, "PHONE_THUMB1", 282, 180);
        }
    }

    return TRUE;
}

gboolean publish_ui_map_cb (gpointer data)
{
    state_t *self = (state_t*)data;

    return TRUE;

    if (self->computing)
        return TRUE;

    if (self->d_graph)
        publish_ui_map (self->d_graph, "UI_MAP", self->lcm);

    return TRUE;
}

/* timeout called for local node estimation
*/
gboolean node_estimation_cb (gpointer data)
{
    state_t *self = (state_t*)data;
    double secs;

    if (!self->path || g_queue_is_empty (self->path))
        return TRUE;

    if (self->param->mission_success == 1)
        return TRUE;

    if (!self->current_edge)
        return TRUE;

    //if (self->user_where_next_utime > 0 && self->utime - self->user_where_next_utime < 4000000)
    //    return TRUE;

    dbg (DBG_CLASS, "############  NODE ESTIMATION ################\n");

    // fetch latest features
    navlcm_feature_list_t *features = get_nth_features (0);

    if (!features)
        return TRUE;

    // enforce max update rate
    //if (self->last_node_estimate_utime > 0 &&
    //        features->utime - self->last_node_estimate_utime < 1000000)
    //    return TRUE;

    self->param->node_estimation_period = features->utime - self->last_node_estimate_utime;

    self->last_node_estimate_utime = features->utime;

    // local node estimation
    dbg (DBG_CLASS, "******* state update ***********\n");
    GTimer *timer = g_timer_new ();
    double variance = .0;

    state_transition_update (self->d_graph, 5, 1.0);

    state_observation_update (self->d_graph, self->current_edge, 5, features, self->path, &variance);

    //state_print (self->d_graph, self->current_edge->start);
    secs = g_timer_elapsed (timer, NULL);

    dbg (DBG_CLASS, "[1] state update speed: %.3f secs (%.1f Hz)", secs, 1.0/secs);

    if (fabs (state_sum (self->d_graph)-1.0) > 1E-3) {
        dbg (DBG_ERROR, "Error: inconsistent state PDF. Sum = %.4f", state_sum (self->d_graph));
    }

    //assert (fabs (state_sum (self->d_graph)-1.0) < 1E-3);

    dbg (DBG_CLASS, "current edge.");

    dijk_edge_print (self->current_edge);


    //state_print_to_file (self->d_graph, "state.txt");

    state_update_class_param (self->param, self->d_graph);

    // find peak in state estimator
    dijk_node_t *current_node = NULL;
    state_find_maximum (self->d_graph, &current_node);
    assert (current_node);
    self->current_node = current_node;

    // did we reach our destination?
    if (!self->path) 
        return TRUE;
    assert (g_queue_get_length (self->path) > 0);
    dijk_edge_t *last_edge = (dijk_edge_t*)g_queue_peek_tail (self->path);
    assert (last_edge);
    dijk_node_t *target_node = last_edge->end;

    if (current_node == target_node) {
        self->param->mission_success =1;
        dbg (DBG_CLASS, "************** Mission success ******************\n");

        // fetch next target node
        if (!g_queue_is_empty (self->mission)) {
            int *nextid = (int*)g_queue_pop_head (self->mission);
            class_goto_node (self, *nextid);
        }
        return TRUE;
    }

    // otherwise, look for next edge in the path
    self->current_edge = dijk_find_edge (self->path, current_node);

    // recompute the path
    update_path (self, current_node, target_node);

    // integrate time and distance to destination
    dijk_integrate_time_distance (self->path, &self->param->eta_secs, &self->param->eta_meters);
    dbg (DBG_CLASS, "ETA: %.1f secs  %.1f m.", self->param->eta_secs, self->param->eta_meters);

    if (!g_queue_is_empty (self->pose_queue)) {
        botlcm_pose_t *pose = (botlcm_pose_t*)g_queue_peek_head (self->pose_queue);
        dijk_graph_ground_truth_localization (self->d_graph, pose, self->fix_poses, self->current_node, features->num, "loc-evaluation.txt");
    }

    // update the state of the robot
    self->param->nodeid_now = self->current_edge->start ? self->current_edge->start->uid : -1;
    self->param->nodeid_next = self->current_edge->end ? self->current_edge->end->uid : -1;

    // save node id to text file for gentle process restart
    FILE *fp = fopen ("start_id.txt", "w");
    fprintf (fp, "%d", self->param->nodeid_now);
    fclose (fp);

    secs = g_timer_elapsed (timer, NULL);

    dbg (DBG_CLASS, "state update speed: %.3f secs (%.1f Hz)", secs, 1.0/secs);
    g_timer_destroy (timer);

    dbg (DBG_CLASS, "############################################");

    return TRUE;
}

/* calibration check callback
*/
gboolean calibration_check_cb (gpointer data)
{
    double angle_rad = .0;
    state_t *self = (state_t*)data;

    // fetch latest features
    navlcm_feature_list_t *features = get_nth_features (0);

    if (features && self->ref_point_features) {
        if (class_orientation (features, self->ref_point_features, self->config, &angle_rad, NULL, self->lcm) == 0) {
            dbg (DBG_CLASS, "orientation : %.3f deg.", angle_rad * 180.0 / M_PI);

            utter_directions (self, angle_rad);

            // update class state
            if (!self->param->map_filename)
                self->param->map_filename = strdup ("");
            self->param->orientation[0] = angle_rad;
            self->param->orientation[1] = angle_rad;
        }

    }

    return TRUE;
}

    static void
on_features_param (const lcm_recv_buf_t *buf, const char *channel, 
        const navlcm_features_param_t *msg,
        void *user)
{
    state_t *self = (state_t*)user;

    if (self->features_param)
        return;
    self->features_param = navlcm_features_param_t_copy (msg);
}

navlcm_feature_list_t *sift_debug (state_t *self, GQueue *img)
{
    int nimg = g_queue_get_length (img);
    botlcm_image_t **im = (botlcm_image_t**)malloc(nimg*sizeof(botlcm_image_t*));
    for (int i=0;i<nimg;i++)
        im[i] = (botlcm_image_t*)g_queue_peek_nth (img, i);

    navlcm_feature_list_t *fs = features_driver (im, nimg, self->features_param);

    free (im);

    return fs;
}

/* a buffer for temporal smoothing */
static int64_t *g_guidance_utime_buf = NULL;
static double *g_guidance_angle_buf[2] = {NULL, NULL};

/* timeout called for rotation guidance
*/
gboolean rotation_guidance_cb (gpointer data)
{
    state_t *self = (state_t*)data;

    // initialize buffer
    int bufsize = 100;
    if (!g_guidance_utime_buf) {
        g_guidance_utime_buf    = (int64_t*)calloc(bufsize, sizeof(int64_t));
        g_guidance_angle_buf[0] = (double*)calloc(bufsize, sizeof(double));
        g_guidance_angle_buf[1] = (double*)calloc(bufsize, sizeof(double));
    }

    if (self->param->mission_success == 1)
        return TRUE;

    dbg (DBG_CLASS, "************  NODE ORIENTATION ****************\n");

    // check that path exists
    if (!self->current_edge || !self->path) {
        fprintf (stderr, "no edge or no path to follow.\n");
        return TRUE;
    }

    // fetch latest features
    navlcm_feature_list_t *features = get_nth_features (0);

    if (!features)
        return TRUE;

    GTimer *timer = g_timer_new ();

    self->param->rotation_guidance_period = features->utime - self->last_rotation_guidance_utime;

    self->last_rotation_guidance_utime = features->utime;

    // compute orientation with two nodes on the edge
    double angles[2];
    double weights[2];

    dijk_edge_t *e1 = self->current_edge;
    dijk_edge_t *e2 = NULL;
    GList *iter = g_queue_find (self->path, e1);
    double variance = .0;

    if (!iter) {
        dbg (DBG_ERROR, "[rotation guidance] edge is not on path.");
        g_timer_destroy (timer);
        return TRUE;
    }

    // apparently we have arrived at our destination
    if (!iter->next) {
        g_timer_destroy (timer);
        return TRUE;
    }

    e2 = (dijk_edge_t*)iter->next->data;

    assert (e1->end == e2->start);

    class_orientation (features, e1->features, self->config, &angles[0], &variance, self->lcm);
    class_orientation (features, e2->features, self->config, &angles[1], NULL, NULL);

    self->param->psi_distance = variance;
    self->param->psi_distance_thresh = PSI_THRESH;

    if (e1->reverse) angles[0] = clip_value (angles[0]+M_PI, -M_PI, M_PI, 1-6);
    if (e2->reverse) angles[1] = clip_value (angles[1]+M_PI, -M_PI, M_PI, 1-6);

    weights[0] = e1->start->pdf1;
    weights[1] = e2->start->pdf1;

    // normalize
    double d = weights[0] + weights[1];
    weights[0] /= d;
    weights[1] /= d;

    dbg (DBG_CLASS, "rotation [%.1f deg., %.3f] [%.1f deg., %.3f]",
            to_degrees (angles[0]), weights[0], to_degrees (angles[1]), weights[1]);

    double angle = vect_weighted_mean_sincos (angles, weights, 2);

    double err = class_guidance_derivative_sliding_window (angle, 20);

    self->param->confidence = fmin (1.0, fmax (.0, 1.0 - (err/M_PI)));

    if (self->param->confidence < CONFIDENCE_THRESH)
        self->param->wrong_way = 1;
    else
        self->param->wrong_way = 0;

    // temporal smoothing
    for (int i=bufsize-1;i>=1;i--) {
        g_guidance_utime_buf [i] = g_guidance_utime_buf [i-1];
        g_guidance_angle_buf [0][i] = g_guidance_angle_buf [0][i-1];
        g_guidance_angle_buf [1][i] = g_guidance_angle_buf [1][i-1];
    }
    g_guidance_utime_buf [0] = features->utime;
    g_guidance_angle_buf [0][0] = angle;
    g_guidance_angle_buf [1][0] = angles[1];

    int winsize = 1;
    for (int i=1;i<bufsize-1;i++) {
        if (g_guidance_utime_buf[i] != 0 && g_guidance_utime_buf[0] - g_guidance_utime_buf [i] < 2000000) {
            winsize = i;
        } else {
            break;
        }
    }

    // update class state
    self->param->orientation[0] = vect_mean_sincos (g_guidance_angle_buf[0], winsize);//angle_rad;
    self->param->orientation[1] = vect_mean_sincos (g_guidance_angle_buf[1], winsize);//angle_rad;

    dbg (DBG_CLASS, "output orientation %.2f / %.2f deg.", to_degrees (self->param->orientation[0]), to_degrees (self->param->orientation[1]));

    dijk_edge_print (e1);
    dijk_edge_print (e2);

    dbg (DBG_CLASS, "*********************************************");
    
    double secs = g_timer_elapsed (timer, NULL);
    dbg (DBG_CLASS, "rotation guidance timer: %.3f secs  (%.1f Hz)", secs, 1.0/secs);
    g_timer_destroy (timer);

    return TRUE;
}

/* timeout called to publish the classifier status
*/
gboolean publish_class_param (gpointer data)
{
    state_t *self = (state_t*)data;

    self->param->utime = self->utime;

    if (!self->param->pdfval) {
        self->param->pdfval = (double*)calloc(1, sizeof(double));
        self->param->pdfind = (int*)calloc(1, sizeof(int));
        self->param->pdf_size = 1;
    }

    if (!self->param->path) {
        self->param->path = (int*)calloc(1,sizeof(int));
        self->param->path_size = 1;
    }

    if (!self->debug && self->param->path && self->param->pdfval)
        navlcm_class_param_t_publish (self->lcm, "CLASS_STATE", self->param);

    if (self->path) {
        int count=0;
        for (GList *piter=g_queue_peek_head_link(self->path);piter;piter=piter->next) {
            dijk_edge_t *e = (dijk_edge_t*)piter->data;
            if (e->start)
                self->param->path[count] = e->start->uid;
            count++;
            if (count > 4)
                break;
        }
    }

    if (self->param->mode == NAVLCM_CLASS_PARAM_T_NAVIGATION_MODE) {
        /* publish direction for fishbot */
        char msg[128];
        memset (msg, '\0', 128);
        sprintf (msg, "%f", self->param->orientation[0]);

        botlcm_raw_t rd;
        rd.utime = self->param->utime;
        rd.length = strlen (msg)+1;
        rd.data = (uint8_t*)msg;

        botlcm_raw_t_publish (self->lcm, "GOAL_HEADING", &rd);
    }

    return TRUE;
}

/* timeout callback
*/
gboolean publish_map_list (gpointer data)
{
    //dbg (DBG_CLASS, "publishing map list");

    state_t *self = (state_t*)data;

    class_publish_map_list (self->lcm);

    return TRUE;
}

/* speaker timeout
*/
gboolean speak (gpointer data)
{
    return TRUE;
    state_t *self = (state_t*)data;
    if (self->param->mode == NAVLCM_CLASS_PARAM_T_NAVIGATION_MODE)
        utter_directions (self, self->param->orientation[0]);
    return TRUE;
}

// return 1 if a gate is triggered, 0 otherwise
//
int gate_trigger_on_pose_data (state_t *self)
{
    /*
    // check that we are receiving pose data
    botlcm_pose_t *pose = (botlcm_pose_t*)g_queue_peek_head (self->pose_queue);
    if (!pose) return 0;

    // compute pose distance in meters
    double dist = pow(pose->pos[0]-g->pose.pos[0],2) + pow(pose->pos[1]-g->pose.pos[1],2);

    dist = sqrt (dist);

    if (dist > 10.0)
    return 1;
    */
    return 0;
}

// return 1 if a gate is triggered, 0 otherwise
//
int gate_trigger_on_gps_data (state_t *self)
{
    /*
    // check if last node is too far away in lat-lon space
    navlcm_gate_t *g = get_latest_gate (self);
    if (!g) return 0;

    // check that lat-lon data is available
    if (g->gps_to_local.utime == 0) return 0;

    // check that we are receiving gps data
    if (!self->gps_to_local) return 0;

    // compare lat-lon positions
    double dlat = diff_angle_plus_minus_180 (g->gps_to_local.lat_lon_el_theta[0], self->gps_to_local->lat_lon_el_theta[0]);
    double dlon = diff_angle_plus_minus_180 (g->gps_to_local.lat_lon_el_theta[1], self->gps_to_local->lat_lon_el_theta[1]);

    double earth_radius_m = 6371000 * 2 * PI;
    double threshold_m = 10.0;
    double threshold_deg = threshold_m * 360.0 / earth_radius_m;

    if (fabs (dlat) > threshold_deg) return 1;
    if (fabs (dlon) > threshold_deg) return 1;

    return 0;
    */
}

void run_loop_closure_full (state_t *self) 
{
    int count = 0;
    for (GList *iter=g_queue_peek_head_link (self->d_graph->edges);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;
        if (e->reverse)
            continue;
        self->corrmat = loop_update_full (self->voctree, self->corrmat, &self->corrmat_size, 
                e->features, e->start->uid, BAGS_WORD_RADIUS);
        count++;
    }

    corrmat_write (self->corrmat, self->corrmat_size, "corrmat.dat");

    dbg (DBG_CLASS, "Saved correlation matrix %d x %d to corrmat.dat", self->corrmat_size, self->corrmat_size);
}

void process_graph_time_lapse (state_t *self)
{
    for (int i=0;i<g_queue_get_length (self->d_graph->nodes);i++) {
        char name[256];
        sprintf (name, "graph-%d.ps", i);

        dijk_graph_layout_to_file (self->d_graph, "neato", "ps", name, i);
    }
}

void join_nodes (state_t *self, int x0, int x1, int y0, int y1)
{
    int min_seq_length;
    bot_conf_get_int (self->conf, "loop_closure.min_seq_length", &min_seq_length);
    gboolean reverse = (x1-x0)*(y1-y0) < 0;

    // create a component from input
    GQueue *components = g_queue_new();
    component_t *comp = loop_index_to_component (x0, x1, y0, y1, reverse, min_seq_length);
    if (comp) {
        dbg (DBG_CLASS, "applying component %d-%d  %d-%d", x0, x1, y0, y1);
        g_queue_push_tail (components, comp);
        dijk_apply_components (self->d_graph, components);
    } else {
        dbg (DBG_ERROR, "failed to apply component  %d-%d  %d-%d", x0, x1, y0, y1);
    }

    g_queue_free (components);
}

void process_correlation_matrix (state_t *self, const char *filename, dijk_graph_t *dg)
{
    double canny_thresh, hough_thresh;
    double correlation_threshold, alignment_penalty;
    double alignment_threshold, alignment_tail_thresh, alignment_max_slope_error;
    int alignment_min_diag_distance, min_seq_length, correlation_diag_size;
    int alignment_search_radius, alignment_min_node_id;

    bot_conf_get_double (self->conf, "loop_closure.canny_thresh", &canny_thresh);
    bot_conf_get_double (self->conf, "loop_closure.hough_thresh", &hough_thresh);
    bot_conf_get_int (self->conf, "loop_closure.min_seq_length", &min_seq_length);
    bot_conf_get_int (self->conf, "loop_closure.correlation_diag_size", &correlation_diag_size);
    bot_conf_get_double (self->conf, "loop_closure.correlation_treshold", &correlation_threshold);
    bot_conf_get_double (self->conf, "loop_closure.alignment_penalty", &alignment_penalty);
    bot_conf_get_double (self->conf, "loop_closure.alignment_threshold", &alignment_threshold);
    bot_conf_get_double (self->conf, "loop_closure.alignment_tail_thresh", &alignment_tail_thresh);
    bot_conf_get_int (self->conf, "loop_closure.alignment_min_diag_distance", &alignment_min_diag_distance);
    bot_conf_get_double (self->conf, "loop_closure.alignment_max_slope_error", &alignment_max_slope_error);
    bot_conf_get_int (self->conf, "loop_closure.alignment_search_radius", &alignment_search_radius);
    bot_conf_get_int (self->conf, "loop_closure.alignment_min_node_id", &alignment_min_node_id);

    int size;
    double *corrmat = corrmat_read (filename, &size);

    // extract similar sequences
    GQueue *comp = loop_extract_components_from_correlation_matrix (corrmat, size, FALSE, canny_thresh, hough_thresh, 
            min_seq_length, correlation_diag_size, correlation_threshold, alignment_penalty, alignment_threshold, 
            alignment_tail_thresh, alignment_min_diag_distance, alignment_max_slope_error, alignment_search_radius,
            alignment_min_node_id); 

    // save results to images
    loop_matrix_components_to_image  (corrmat, size, comp);

    free (corrmat);

    for (GList *iter=g_queue_peek_head_link(comp);iter;iter=iter->next) {
        dijk_print_component (dg, (component_t*)iter->data, filename, "match.txt");
    }

    if (dijk_graph_n_nodes (dg) > 1) {
        //    dijk_graph_layout_to_file (dg, "neato", "ps", "before.ps", -1);

        dijk_apply_components (dg, comp);

        //  dijk_graph_layout_to_file (dg, "neato", "ps", "after.ps", -1);

        dijk_graph_remove_triplets (dg);

        dijk_graph_enforce_type_symmetry (dg);

        dijk_graph_layout_to_file (dg, "neato", "ps", "new-map.ps", -1);

        dijk_graph_write_to_file (dg, "new-map.bin");
    }
}

static void main_shutdown (int sig)
{
    state_t *self = g_self;

    g_main_loop_unref (self->loop);

    self->exit = TRUE;

    // unlock tracker thread
    g_mutex_lock (self->compute_mutex);
    g_cond_signal (self->compute_cond);
    g_mutex_unlock (self->compute_mutex);

    g_thread_join (self->compute_thread);

    self->exit = 1;
}

/* the main method 
*/
int main (int argc, char *argv[])
{
    srand (time (NULL));

    g_type_init ();

    dbg_init ();

    // init thread
    if (!g_thread_supported ()) {
        g_thread_init (NULL);
        dbg (DBG_INFO, "[logplayer] g_thread supported");
    } else {
        dbg (DBG_ERROR,"[logplayer] g_thread NOT supported");
        exit (1);
    }

    // initialize application state and zero out memory
    g_self = (state_t*) calloc( 1, sizeof(state_t) );

    state_t *self = g_self;

    // run the main loop
    self->loop = g_main_loop_new(NULL, FALSE);

    getopt_t *gopt = getopt_create();

    getopt_add_bool  (gopt, 'h',   "help",    0,        "Show this help");
    getopt_add_int   (gopt, 'm',   "mode",  "4",     "Classifier state (exploration, navigation, calibration, calibration check, standby)");
    getopt_add_int   (gopt, ' ',   "node-id",  "-1",     "Gate ID");
    getopt_add_int   (gopt, ' ',   "target-id",  "-1",     "Target gate ID");
    getopt_add_int    (gopt, ' ', "flow-scale", "50", "Optical flow image scaling (0-100)");
    getopt_add_bool (gopt, ' ', "loop-closure", 0, "Run loop closure");
    getopt_add_bool  (gopt, ' ',   "imu-validation", 0, "Run IMU validation upon exit");
    getopt_add_bool (gopt, ' ', "save-graph-images", 0, "Save node images in PNG format and place graph is PS format");
    getopt_add_string (gopt, ' ', "map-file", "map.bin", "Map file");
    getopt_add_string (gopt, ' ', "corrmat", "", "Correspondence matrix file");
    getopt_add_string (gopt, ' ', "gates-snap-dir", "img", "Directory to save gates images");
    getopt_add_string (gopt, ' ', "mission-file", "mission.txt", "Mission file");
    getopt_add_string (gopt, ' ', "flow-calib-file", "", "Flow calibration file");
    getopt_add_string (gopt, ' ', "fix-pose-map", "", "Pose file to fix map");
    getopt_add_string (gopt, ' ', "fix-pose", "", "Pose file to fix live poses");
    getopt_add_string (gopt, ' ', "join-nodes", "", "Node correspondences (four int)");
    getopt_add_string (gopt, ' ', "del-edge", "", "Delete an edge in the graph (two int)");
    getopt_add_string (gopt, ' ', "merge-nodes", "", "Fuse two nodes in the graph (two int)");
    getopt_add_string (gopt, ' ', "label-node", "", "Label node");

    printf ("1\n"); fflush(stdout);
    if (!getopt_parse(gopt, argc, argv, 1) || getopt_get_bool(gopt,"help")) {
        printf("Usage: %s [options]\n\n", argv[0]);
        getopt_do_usage(gopt);
        return 0;
    }

    printf ("1\n"); fflush(stdout);

    // read config file
    if (!(self->config = read_config_file ())) {
        dbg (DBG_ERROR, "Error: failed to read config file.");
        return -1;
    }

    // read classifier calibration
    if (class_read_calib (self->config)) {
        dbg (DBG_ERROR, "Warning: failed to read calibration file.");
    }


    self->param = (navlcm_class_param_t*)malloc(sizeof(navlcm_class_param_t));
    self->param->mode = getopt_get_int (gopt, "mode");
    printf ("mode : %d\n", self->param->mode);
    self->verbose = 0;
    self->save_images = getopt_get_bool (gopt, "save-graph-images");
    self->debug = 0;//getopt_get_bool (gopt, "debug");
    self->param->map_filename = strdup (getopt_get_string (gopt, "map-file"));
    self->param->orientation[0] = .0;
    self->param->orientation[1] = .0;
    self->param->nodeid_now = -1;
    self->param->nodeid_next = -1;
    self->param->nodeid_end = -1;
    self->param->psi_distance = .0;
    self->param->psi_distance_thresh = .0;
    self->param->pdf_size = 0;
    self->param->pdfval = NULL;
    self->param->pdfind = NULL;
    self->param->path_size = 0;
    self->param->path = NULL;
    self->param->motion_type[0] = -1;
    self->param->motion_type[1] = -1;
    self->param->next_direction = -1;
    self->param->next_direction_meters = .0;
    self->param->eta_secs = .0;
    self->param->eta_meters = .0;
    self->param->mission_success = 0;
    self->param->wrong_way = 0;

    self->nodes_snap_dir = strdup (getopt_get_string (gopt, "gates-snap-dir"));
    self->flow_field_filename = strdup (getopt_get_string (gopt, "flow-calib-file"));

    self->lcm = lcm_create(NULL);
    if (!self->lcm)
        return 1;

    self->lcmgl = globals_get_lcmgl ("GUIDANCE", 1);
    self->conf = globals_get_config ();

    self->compute_mutex = g_mutex_new ();
    self->data_mutex = g_mutex_new ();
    self->compute_cond = g_cond_new ();
    self->exit = FALSE;
    self->feature_list = g_queue_new ();
    self->seq_filename = NULL;
    self->current_edge = NULL;
    self->current_node = NULL;
    self->last_audio_utime = 0;
    self->pose_queue = g_queue_new ();
    self->imu_queue = g_queue_new ();
    self->gps_to_local = NULL;
    self->applanix_data = NULL;
    self->upward_image_queue = g_queue_new ();
    self->ground_thread_running = FALSE;
    self->gt_request_utime = 0;
    self->ref_point_features = NULL;
    self->path = NULL;
    self->computing = FALSE;
    self->last_utterance_utime = 0;
    self->d_graph = dijk_graph_new ();
    self->corrmat = NULL;
    self->corrmat_size = 0;
    self->voctree = g_queue_new ();
    self->last_node_estimate_utime = 0;
    self->last_rotation_guidance_utime = 0;
    self->features_param = NULL;
    self->mission = g_queue_new ();
    self->flow_scale = .01 * getopt_get_int (gopt, "flow-scale");
    self->fix_poses = NULL;
    self->user_where_next_utime = 0;
    self->end_of_log_utime = 0;

    bot_conf_get_int (self->conf, "motions.integration-time", &self->flow_integration_time);

    ///////////// unit tests //////////////////////////
    //tree_unit_testing (10000, 10, BAGS_WORD_RADIUS);
    //dijk_unit_testing ();
    //bags_performance_testing ();
//    int bags_nsets = 1000;
//    int bags_nfeatures = 500;
//    bags_performance_testing (1, bags_nsets, bags_nfeatures, 1, "vocabulary-naive.txt");
//    bags_performance_testing (2, bags_nsets, bags_nfeatures, 1, "vocabulary-naive-opt.txt");
//    bags_performance_testing (3, bags_nsets, bags_nfeatures, 2, "vocabulary-tree-N-2.txt");
//      bags_performance_testing (3, bags_nsets, bags_nfeatures, 3, "vocabulary-tree-N-3.txt");
//    bags_performance_testing (3, bags_nsets, bags_nfeatures, 5, "vocabulary-tree-N-5.txt");
//    bags_performance_testing (3, bags_nsets, bags_nfeatures, 7, "vocabulary-tree-N-7.txt");
//    bags_performance_testing (3, bags_nsets, bags_nfeatures, 10, "vocabulary-tree-N-10.txt");


    // math unit testing
    //math_matrix_mult_unit_testing_float ();

    // read gates from command line file
    if (file_exists (self->param->map_filename)) {

        dijk_graph_read_from_file (self->d_graph, self->param->map_filename);

        if (getopt_get_bool (gopt, "save-graph-images")) {
            dijk_graph_layout_to_file (self->d_graph, "neato", "ps", "new-map.ps", -1);
            dijk_save_images_to_file (self->d_graph, self->nodes_snap_dir, FALSE);
        }

        //        process_graph_time_lapse (self);
        //
        //dijk_graph_print (self->d_graph);

        publish_ui_map (self->d_graph, "UI_MAP", self->lcm);

        if (getopt_get_bool (gopt, "loop-closure")) {
            run_loop_closure_full (self);
            return 0;
        }
    } else {
        dbg (DBG_ERROR, "WARNING! Input file %s does not exist.", self->param->map_filename);
    }

    dijk_graph_print_node_utimes (self->d_graph, "nodes.txt");

    // set current node
    int gateid = getopt_get_int (gopt, "node-id");
    if (gateid == -1) {
        FILE *fp = fopen ("start_id.txt", "r");
        if (fp) {
            if (fscanf (fp, "%d", &gateid) != 1) {
                fprintf (stderr, "Invalid content for start_id.txt.\n");
                exit (0);
            }
            fclose (fp);
        }
    }
    if (gateid != -1) {
        force_node (self, gateid);
    }
    printf ("**** current node: %d\n", gateid);

    // set target node
    int target_id = getopt_get_int (gopt, "target-id");
    if (target_id == -1) {
        FILE *fp = fopen ("end_id.txt", "r");
        if (fp) {
            if (fscanf (fp, "%d", &target_id) != 1) {
                fprintf (stderr, "Invalid content for end_id.txt.\n");
                exit (0);
            }
            fclose (fp);
        }
    }
    if (target_id != -1) {
        class_goto_node (self, target_id);
    }
    printf ("---------> going to node : %d\n", target_id);

    // label node
    if (strlen (getopt_get_string (gopt, "label-node")) > 2) {
        if (self->current_node) {
            dijk_node_set_label (self->current_node, getopt_get_string (gopt, "label-node"));
            dijk_graph_write_to_file (self->d_graph, "new-map.bin");
        } else {
            fprintf (stderr, "Use --node-id option to specify a node.\n");
            return 0;
        }
    }

    if (strlen (getopt_get_string (gopt, "fix-pose")) > 2) {
        self->fix_poses = util_read_poses (getopt_get_string (gopt, "fix-pose"));
        // switch to reverse chronological order for time-based search
        g_queue_reverse (self->fix_poses);
    }

    if (strlen (getopt_get_string (gopt, "fix-pose-map")) > 2) {
        GQueue *poses = util_read_poses (getopt_get_string (gopt, "fix-pose-map"));
        // switch to reverse chronological order for time-based search
        g_queue_reverse (poses);
        dijk_graph_fix_poses (self->d_graph, poses);
        dijk_graph_write_to_file (self->d_graph, "new-map.bin");
    }

    // read mission file
    if (strlen (getopt_get_string (gopt, "mission-file")) > 2) {
        read_mission_file (self->mission, getopt_get_string (gopt, "mission-file"));

        // test mission
        dijk_graph_test_mission (self->d_graph, self->mission);
    }

    // read correspondence matrix 
    if (strlen (getopt_get_string (gopt, "corrmat")) > 2) {
        process_correlation_matrix (self, getopt_get_string (gopt, "corrmat"), self->d_graph);
        return 0;
    }

    // apply matches in the graph
    if (strlen (getopt_get_string (gopt, "join-nodes")) > 2) {
        int x0, x1, y0, y1;
        if (sscanf (getopt_get_string (gopt, "join-nodes"), "%d%d%d%d", &x0, &x1, &y0, &y1)==4) {
            join_nodes (self, x0, x1, y0, y1);
            dijk_graph_layout_to_file (self->d_graph, "neato", "ps", "new-map.ps", -1);
            dijk_graph_write_to_file (self->d_graph, "new-map.bin");
        } else {
            fprintf (stderr, "Invalid arguments for --node-match option.\n");
            return 0;
        }
    }

    // delete an edge in the graph
    if (strlen (getopt_get_string (gopt, "del-edge")) > 2) {
        int id1, id2;
        if (sscanf (getopt_get_string (gopt, "del-edge"), "%d%d", &id1, &id2)) {
            dijk_graph_remove_edge_by_id (self->d_graph, id1, id2);
            dijk_graph_layout_to_file (self->d_graph, "neato", "ps", "new-map.ps", -1);
            dijk_graph_write_to_file (self->d_graph, "new-map.bin");
        }
    }

    // fuse two in the graph
    if (strlen (getopt_get_string (gopt, "merge-nodes")) > 2) {
        int id1, id2;
        if (sscanf (getopt_get_string (gopt, "merge-nodes"), "%d%d", &id1, &id2)) {
            dijk_graph_merge_nodes_by_id (self->d_graph, id1, id2);
            dijk_graph_enforce_type_symmetry (self->d_graph);
            dijk_graph_layout_to_file (self->d_graph, "neato", "ps", "new-map.ps", -1);
            dijk_graph_write_to_file (self->d_graph, "new-map.bin");
        }
    }

    // copy of gate images
    self->camimg_queue = (GQueue**)malloc(self->config->nsensors * sizeof(GQueue*));
    for (int i=0;i<self->config->nsensors;i++)
        self->camimg_queue[i] = g_queue_new();

    // flow
    self->flow_field_set = flow_field_set_init_with_data (self->config->nsensors, 4, NULL);

    self->flow_queue = (GQueue**)calloc (self->config->nsensors, sizeof(GQueue*));
    for (int i=0;i<self->config->nsensors;i++)
        self->flow_queue[i] = g_queue_new ();

    // flow motions
    self->flow_field_types = g_queue_new ();
    int nmotions = bot_conf_get_array_len (self->conf, "motions.calib-file");
    char **flow_motion_filenames = (char**)malloc(nmotions*sizeof(char*));
    char **flow_motion_names = (char**)malloc(nmotions*sizeof(char*));
    bot_conf_get_str_array (self->conf, "motions.calib-file", flow_motion_filenames, nmotions);
    bot_conf_get_str_array (self->conf, "motions.names", flow_motion_names, nmotions);
    if (nmotions > 0)
        flow_field_set_series_read_from_file (self->flow_field_types, flow_motion_filenames, flow_motion_names, nmotions, self->config->nsensors, CONFIG_DIR);
    self->mc = motion_classifier_init (nmotions);

    // classifier histograms
    self->class_hist = g_queue_new ();
    for (int i=0;i<self->config->nsensors;i++) {
        for (int j=0;j<self->config->nsensors;j++) {
            g_queue_push_tail (self->class_hist, classifier_hist_create_with_data (72, -M_PI, M_PI));
        }
    }

    // subscribe to the image channel
    for (int i=0;i<self->config->nsensors;i++) {
        botlcm_image_t_subscribe (self->lcm, self->config->channel_names[i], 
                on_botlcm_image_event, self);
    }


    glib_mainloop_attach_lcm (self->lcm);

    navlcm_feature_list_t_subscribe (self->lcm, "FEATURE_SET", on_feature_list_event, self);
    navlcm_generic_cmd_t_subscribe (self->lcm, "CLASS_CMD", on_generic_cmd_event, self);
    //    navlcm_imu_t_subscribe (self->lcm, "IMU", on_imu_event, self);
    botlcm_pose_t_subscribe (self->lcm, "POSE", on_pose_event, self);
    navlcm_gps_to_local_t_subscribe (self->lcm, "GPS_TO_LOCAL", on_gps_to_local_event, self);
    botlcm_raw_t_subscribe (self->lcm, "APPLANIX", on_applanix_event, self);
    botlcm_image_t_subscribe (self->lcm, "CAM_THUMB_UPWARD", on_upward_image_event, self);
    navlcm_features_param_t_subscribe (self->lcm, "FEATURES_PARAM", &on_features_param, self);

    g_timeout_add_seconds (5, publish_ui_map_cb, self);
    g_timeout_add_seconds (4, publish_map_list_cb, self);
    g_timeout_add_seconds (4, publish_ui_images_cb, self);
    g_timeout_add_seconds (2, update_future_direction_cb, self);
    g_timeout_add_seconds (10, end_of_log_cb, self);

    // publish cam settings every now and then
    g_timeout_add (500, &publish_class_param, self);
    g_timeout_add_seconds (10, &publish_map_list, self);
    //    g_timeout_add_seconds (2, &speak, self);

    // start main computation thread
    self->compute_thread = g_thread_create (compute_thread_cb, self, TRUE, NULL);

    //    signal (SIGINT, main_shutdown);
    //    signal (SIGHUP, main_shutdown);

    signal_pipe_glib_quit_on_kill (self->loop);

    // run the main loop
    g_main_loop_run (self->loop);

    main_shutdown (0);

    return 0;
}

