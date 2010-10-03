#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include <glib.h>

#include <camunits/cam.h>
#include <lcm/lcm.h>

#include <lcmtypes/camlcm_image_t.h>

#define PUBLISH_INTERVAL_S 0.5
#define PUBLISH_INTERVAL_USEC (PUBLISH_INTERVAL_S * 1000000)

typedef struct _state_t {
    GMainLoop *mainloop;
    char filename[4096];

    int64_t next_publish_utime;
    lcm_t *lcm;
} state_t;

static int64_t _timestamp_now()
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return (int64_t) tv.tv_sec * 1000000 + tv.tv_usec;
}

static void
print_usage_and_inputs (const char *progname, CamUnitManager *manager)
{
    fprintf (stderr, "usage: %s <input_id>\n\n", progname);
    fprintf (stderr, "Available inputs:\n\n"); 
    GList *udlist = cam_unit_manager_list_package (manager, "input", TRUE);
    for (GList *uditer=udlist; uditer; uditer=uditer->next) {
        CamUnitDescription *udesc = CAM_UNIT_DESCRIPTION(uditer->data);
        printf("  %s  (%s)\n", udesc->unit_id, udesc->name);
    }
    g_list_free(udlist);
}

static void
on_frame_ready (CamUnitChain *chain, CamUnit *unit, 
        const CamFrameBuffer *buf, void *user_data)
{
    state_t *s = (state_t*) user_data;

    const CamUnitFormat *fmt = cam_unit_get_output_format (unit);

    int64_t now = _timestamp_now ();
    if (now > s->next_publish_utime) {
        camlcm_image_t msg;
        memset (&msg, 0, sizeof (msg));
        msg.width = fmt->width;
        msg.height = fmt->height;
        msg.row_stride = fmt->row_stride;
        msg.data = buf->data;
        msg.size = buf->bytesused;
        msg.pixelformat = fmt->pixelformat;

        camlcm_image_t_publish (s->lcm, "PHONE_THUMB", &msg);

        s->next_publish_utime = now + (int64_t) PUBLISH_INTERVAL_USEC;

        printf ("publish\n");
    }
}

int main(int argc, char **argv)
{
    g_type_init();

    CamUnit *resize_unit = NULL;
    CamUnit *faulty_unit = NULL;
    CamUnitChain * chain = NULL;
    const char *input_id = NULL;

    state_t s;
    memset (&s, 0, sizeof (s));
    sprintf (s.filename, "snapshot-output.ppm");

    // create the GLib mainloop
    s.mainloop = g_main_loop_new (NULL, FALSE);

    s.lcm = lcm_create ("udpm://?transmit_only=true");
    if (!s.lcm) {
        fprintf (stderr, "Couldn't create LCM\n");
        goto failed;
    }

    // create the image processing chain
    chain = cam_unit_chain_new ();

    // abort if no input unit was specified
    if (argc < 2) {
        print_usage_and_inputs (argv[0], chain->manager);
        goto failed;
    }
    input_id = argv[1];

    // instantiate the input unit
    if (! cam_unit_chain_add_unit_by_id (chain, input_id)) {
        fprintf (stderr, "Oh no!  Couldn't create input unit [%s].\n\n", 
                input_id);
        print_usage_and_inputs (argv[0], chain->manager);
        goto failed;
    }

    // create a unit to convert the input data to 8-bit RGB
    cam_unit_chain_add_unit_by_id (chain, "convert.to_rgb8");

    resize_unit = cam_unit_chain_add_unit_by_id (chain, 
            "filter.ipp.resize");
//    cam_unit_set_control_boolean (resize_unit, "aspect-lock", FALSE);
    cam_unit_set_control_int (resize_unit, "width", 64);
//    cam_unit_set_control_int (resize_unit, "height", 64);

    // start the image processing chain
    faulty_unit = cam_unit_chain_all_units_stream_init (chain);

    // did everything start up correctly?
    if (faulty_unit) {
        fprintf (stderr, "Unit [%s] is not streaming, aborting...\n",
                cam_unit_get_name (faulty_unit));
        goto failed;
    }

    // attach the chain to the glib event loop.
    cam_unit_chain_attach_glib (chain, 1000, NULL);

    // subscribe to be notified when an image has made its way through the
    // chain
    g_signal_connect (G_OBJECT (chain), "frame-ready",
            G_CALLBACK (on_frame_ready), &s);

    s.next_publish_utime = _timestamp_now ();

    // run 
    g_main_loop_run (s.mainloop);

    // cleanup
    g_main_loop_unref (s.mainloop);
    cam_unit_chain_all_units_stream_shutdown (chain);
    g_object_unref (chain);
    return 0;

failed:
    g_main_loop_unref (s.mainloop);
    cam_unit_chain_all_units_stream_shutdown (chain);
    g_object_unref (chain);
    return 1;
}
