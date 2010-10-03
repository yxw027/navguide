/*
 * This module grabs images from a dc1394 camera.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>
#include <signal.h>
#include <math.h>

/* from common */
#include <common/getopt.h>

/* From LCM */
#include <lcm/lcm.h>
#include <bot/lcmtypes/botlcm_image_t.h>

/* From camunits */
#include <camunits/cam.h>

#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>

#include <glib/gstdio.h>

typedef struct state_t_ {
   
    int verbose;
    CamUnitChain *chain;
    lcm_t *lcm;

    gboolean exit;
} state_t ;

static state_t *g_self;

CamUnit *get_cam_unit (state_t *self, int id)
{
    GList *units = cam_unit_chain_get_units (self->chain);
    
    if (g_list_length (units) == 0) {
        g_list_free (units);
        return NULL;
    }
    
    CamUnit *unit = (CamUnit*)g_list_nth_data (units, id);
    
    return unit;
}

int add_camera_xml (state_t *self, const char *file_name)
{
    // Create the chain from the contents of a file.
    printf ("Creating chain with initial input from file %s ....\n",
            file_name);

    gsize length = 3000;
    gchar *xml = NULL;
    GError *error = NULL;
    g_file_get_contents(file_name, &xml, &length, &error);
    if (error)
    {
        fprintf (stderr, "Could not get contents of file: %s\n%s\n", file_name, error->message);
        g_error_free(error);
        return -1;
    }

    // Create the image processing chain.
    printf ("Creating cam_unit_chain.\n");
    self->chain = cam_unit_chain_new();

    int i;
    do // ... and load it with the XML string.
    {
        cam_unit_chain_load_from_str(self->chain, xml, &error);
        if (error)
        {
            g_error_free(error);
            fprintf (stderr, "Waiting to try again\n");
            for (i = 0; i < 5; i++)
            {
                sleep(1);
                printf(" .");
                fflush(stdout);
            }
            printf("\n");
        }
    }
    while (error);
    free(xml);

    // Check for errors.
    CamUnit *faulty_unit = NULL;
    while ((faulty_unit = cam_unit_chain_all_units_stream_init(self->chain)))
    {
        fprintf (stderr, "Unit [%s] is not streaming.  Waiting...\n",
                cam_unit_get_name(faulty_unit));
        sleep(5);
    }

    // TODO remove this after the camunits bug has been fixed.
    GList * units = cam_unit_chain_get_units(self->chain);
    if(units) {
        CamUnit * input_unit = CAM_UNIT(units->data);
        const char *id = cam_unit_get_id(input_unit);
        if(!strncmp(id, "input.dc1394", strlen("input.dc1394"))) {
            cam_unit_set_control_int(input_unit, "packet-size", 1000);
        }
    }
    g_list_free(units);

    // Attach chain to glib main loop
    cam_unit_chain_attach_glib(self->chain, G_PRIORITY_DEFAULT, NULL);
    //g_signal_connect(G_OBJECT(self->chain), "frame-ready",
    //        G_CALLBACK(on_frame_ready), self);

    printf ("Running ...\n");

    return 0;
}

void state_t_destroy (state_t *self)
{
    printf ("exiting...\n");

    if (self->chain) {
        // close the camunits chain
        CamUnit *unit = cam_unit_chain_all_units_stream_shutdown (self->chain);
        if (unit) {
            fprintf (stderr, "failed to shutdown chain properly at unit %s\n", cam_unit_get_name (unit));
        }
        g_object_unref (self->chain);
    }

}

int main(int argc, char *argv[])
{
    g_type_init ();

    g_thread_init (NULL);

    srand (time (NULL));

    // initialize application state and zero out memory
    g_self = (state_t*) calloc( 1, sizeof(state_t) );

    state_t *self = g_self;

    // run the main loop
    getopt_t *gopt = getopt_create();

    getopt_add_bool  (gopt, 'h',   "help",    0, "Show this help");
    getopt_add_bool  (gopt, 'v',   "verbose",    0,     "Be verbose");
    getopt_add_string (gopt, ' ', "xml", "", "Camunit XML file");

    if (!getopt_parse(gopt, argc, argv, 1) || getopt_get_bool(gopt,"help")) {
        printf("Usage: %s [options]\n\n", argv[0]);
        getopt_do_usage(gopt);
        return 0;
    }

    self->verbose = getopt_get_bool(gopt, "verbose");
    self->lcm = lcm_create(NULL);
    if (!self->lcm)
        return 1;

    self->chain = NULL;

    if (add_camera_xml (self, getopt_get_string (gopt, "xml")) != 0)
        return 1;

    GMainLoop *mainloop = g_main_loop_new(NULL, FALSE);

    if (mainloop)
    {
        bot_signal_pipe_glib_quit_on_kill (mainloop);
        g_main_loop_run(mainloop);    
        g_main_loop_unref(mainloop);
    }
    
    // run main loop
    state_t_destroy (self);

    return 0;
}
