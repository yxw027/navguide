/*
 * */

#include "main.h"

int64_t mission_expected_duration (dijk_graph_t *dg, int start_id, int end_id)
{
    if (!dg) 
        return .0;

    dijk_node_t *n1 = dijk_graph_find_node_by_id (dg, start_id);
    dijk_node_t *n2 = dijk_graph_find_node_by_id (dg, end_id);

    if (!n1 || !n2) {
        dbg (DBG_ERROR, "failed to find node %d or node %d in graph.", start_id, end_id);
        return .0;
    }

    GQueue *path = dijk_find_shortest_path (dg, n1, n2);

    double secs, dist_m;
    dijk_integrate_time_distance (path, &secs, &dist_m);

    g_queue_free (path);

    return (int64_t)(secs * 1000000);
}

    static void
on_class_state_event (const lcm_recv_buf_t *buf, const char *channel, 
        const navlcm_class_param_t *msg, void *user)
{
    state_t *self = (state_t*)user;
    
    if (self->class_state)
        navlcm_class_param_t_destroy (self->class_state);

    self->class_state = navlcm_class_param_t_copy (msg);

    if (self->mission_target_node_id != -1 && self->mission_start_node_id == -1) {
        self->mission_start_node_id = msg->nodeid_now;
    }

    if (msg->mission_success) {
        if (self->mission_target_node_id != -1) {
            // mission ended
            int64_t duration = msg->utime - self->mission_start_utime;
            int hours, mins, secs;
            secs_to_hms ((int)duration/1000000, &hours, &mins, &secs);
            // expected excursion time (i.e. during the first visit)
            int64_t exp_duration = mission_expected_duration (self->dg, 
                    self->mission_start_node_id, self->mission_target_node_id);
            int exp_hours, exp_mins, exp_secs;
            secs_to_hms ((int)exp_duration/1000000, &exp_hours, &exp_mins, &exp_secs);
            double speed_ratio = 1.0 * (int)exp_duration / (int)duration;  
            self->total_speed_ratio += speed_ratio;

            printf ("[EVAL] ========== MISSION %d: start node: %d  end node: %d ===========\n", self->n_missions, 
                    self->mission_start_node_id, self->mission_target_node_id); 
            printf ("[EVAL] Actual duration:\tHMS: %02d:%02d:%02d\tSECS: %d\n", hours, mins, secs, (int)(duration)/1000000);
            printf ("[EVAL] Expected duration:\tHMS: %02d:%02d:%02d\tSECS: %d\n", exp_hours, exp_mins, 
                    exp_secs, (int)(exp_duration)/1000000);
            printf ("[EVAL] Speed factor:\t%.2f\t#user confused: %d\t# user lost: %d\n", 
                    speed_ratio, self->unclear_guidance_count, self->user_lost_count);
            fflush (stdout);

            self->mission_target_node_id = -1;
            self->n_missions++;
            if (self->user_lost_count==0)
                self->n_missions_passed++;
            self->total_excursion_time += (int)duration/1000000;
        }
    }
}

    static void
on_botlcm_image_event (const lcm_recv_buf_t *buf, const char *channel, 
        const botlcm_image_t *msg, void *user)
{
    state_t *self = (state_t*)user;
    self->utime = msg->utime;
}

static void
on_generic_cmd_event (const lcm_recv_buf_t *buf, const char *channel, 
        const navlcm_generic_cmd_t *msg, void *user)
{
    state_t *self = (state_t*)user;

    if (msg->code == CLASS_USER_LOST) {
        if (self->mission_target_node_id != -1) {
            if (self->utime - self->last_user_lost_utime > 4000000) {
                self->total_user_lost_count++;
                self->user_lost_count++;
                self->last_user_lost_utime = self->utime;
            }
        }
    } 
    if (msg->code == CLASS_USER_UNCLEAR_GUIDANCE) {
        if (self->mission_target_node_id != -1) {
            if (self->utime - self->last_unclear_guidance_utime > 4000000) {
                self->unclear_guidance_count++;
                self->total_unclear_guidance_count++;
                self->last_unclear_guidance_utime = self->utime;
            }
        }
    }

    if (msg->code == CLASS_GOTO_NODE) {
        self->mission_start_utime = self->utime;
        self->mission_start_node_id = self->class_state ? self->class_state->nodeid_now : -1;
        self->mission_target_node_id = atoi (msg->text);
        self->user_lost_count = 0;
        self->unclear_guidance_count = 0;
    }

    return;
}

static void print_summary (state_t *self)
{
    printf ("[EVAL] =====================================================\n");
    printf ("[EVAL] Summary:\t# missions: %d\tPassed: %d (%.1f %%)\n", self->n_missions, self->n_missions_passed, 
            100.0 * self->n_missions_passed / self->n_missions);
    int hours, mins, secs;
    secs_to_hms (self->total_excursion_time, &hours, &mins, &secs);
    printf ("[EVAL] Total excursion time:\tHMS: %02d:%02d:%02d\tSECS: %d\tMINS: %.1f\n", hours, mins, secs, 
            self->total_excursion_time, 1.0 * self->total_excursion_time / 60);

    double speed = .7; // in m/s
    double total_dist = self->total_excursion_time * speed;

    printf ("[EVAL] Total dist: %.1f m at %.1f m/s (or %.1f km/h)\n", total_dist, speed, speed*3.6);
    printf ("[EVAL] Speed ratio: %.2f\n", self->total_speed_ratio / self->n_missions);
    printf ("[EVAL] Total # unclear guidance: %d\tTotal # user lost: %d\n", self->total_unclear_guidance_count, 
            self->total_user_lost_count);

    double lambda_time = (1.0 * self->total_user_lost_count / self->total_excursion_time) * 60;
    double lambda_dist = (1.0 * self->total_user_lost_count / total_dist) * 1000;
    double gamma_time = (1.0 * self->total_unclear_guidance_count / self->total_excursion_time) * 60;
    double gamma_dist = (1.0 * self->total_unclear_guidance_count / total_dist) * 1000;

    printf ("[EVAL] Lambda: time: %.2f\tdist: %.2f\n", lambda_time, lambda_dist);
    printf ("[EVAL] Gamma:  time: %.2f\tdist: %.2f\n", gamma_time, gamma_dist);
    printf ("[EVAL] =====================================================\n");
    fflush (stdout);

}

gboolean end_of_log_cb (gpointer data)
{
    state_t *self = (state_t*)data;

    if (self->utime == self->end_of_log_utime) {

        print_summary (self);

        exit (0);

        return FALSE;
    }

    self->end_of_log_utime = self->utime;

    return TRUE;
}

/* the main method 
*/
int main (int argc, char *argv[])
{
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
    self->utime = 0;
    self->class_state = NULL;
    self->mission_start_node_id = -1;
    self->mission_target_node_id = -1;
    self->n_missions = 0;
    self->n_missions_passed = 0;
    self->total_excursion_time = 0;
    self->dg = dijk_graph_new ();
    self->utime = 1;
    self->end_of_log_utime = 0;
    self->total_user_lost_count=0;
    self->total_unclear_guidance_count=0;
    self->total_speed_ratio=.0;
    self->last_unclear_guidance_utime = 0;
    self->last_user_lost_utime = 0;

    getopt_t *gopt = getopt_create();

    getopt_add_bool  (gopt, 'h',   "help",    0,        "Show this help");
    getopt_add_bool  (gopt, 'v',   "verbose",    0,     "Be verbose");
    getopt_add_string  (gopt, 'm',   "map-file",    "",     "Map file");

    if (!getopt_parse(gopt, argc, argv, 1) || getopt_get_bool(gopt,"help")) {
        printf("Usage: %s [options]\n\n", argv[0]);
        getopt_do_usage(gopt);
        return 0;
    }

    // read config file
    if (!(self->config = read_config_file ())) {
        dbg (DBG_ERROR, "Error: failed to read config file.");
        return -1;
    }

    // read map
    if (strlen (getopt_get_string (gopt, "map-file")) > 2) {
        dijk_graph_init(self->dg);
        dijk_graph_read_from_file (self->dg, getopt_get_string (gopt, "map-file"));
    }

    self->lcm = lcm_create(NULL);
    if (!self->lcm)
        return 1;


    bot_glib_mainloop_attach_lcm (self->lcm);

    g_timeout_add_seconds (5, end_of_log_cb, self);

    navlcm_generic_cmd_t_subscribe (self->lcm, "CLASS_CMD", on_generic_cmd_event, self);
    navlcm_class_param_t_subscribe (self->lcm, "CLASS_STATE", on_class_state_event, self);
    botlcm_image_t_subscribe (self->lcm, "CAM_THUMB.*", on_botlcm_image_event, self);


    bot_signal_pipe_glib_quit_on_kill (self->loop);

    // run the main loop
    g_main_loop_run (self->loop);

    g_main_loop_unref (self->loop);

    return 0;
}

