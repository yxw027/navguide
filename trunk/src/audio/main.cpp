#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <poll.h>
#include <netdb.h>
#include <unistd.h>
#include <inttypes.h>
#include <getopt.h>
#include <signal.h>
#include <common/udp_util.h>
#include <gst/gst.h>
#include <stdbool.h>

#include <GL/gl.h>
#include <lcm/lcm.h>
#include <common/dbg.h>
#include <common/config.h>
#include <common/glib_util.h>
#include <common/codes.h>

#include <lcmtypes/navlcm_audio_param_t.h>

typedef enum { 
    LEFT=0,RIGHT, SLIGHT_LEFT, SLIGHT_RIGHT, STRAIGHT_AHEAD, UTURN, NEW_NODE, N_EVENTS
} app_events_enum_t;

const char *app_event_name[N_EVENTS] = {"LEFT","RIGHT","SLIGHT_LEFT", "SLIGHT_RIGHT", "STRAIGHT_AHEAD", "UTURN", "NEW_NODE"};

typedef struct {
  char *uri;
  char *say;
} fx_event_t;

typedef struct {
    char *device;
    int verbose;
    lcm_t *lcm;
    int64_t last_utime;
    Config *cfg;
    
    int nevents;
    fx_event_t *event;
    GstElement *pipeline;
    GstElement *audiosink;
    GstBus *bus;

} app_t;


static gboolean
bus_call (GstBus *bus,
          GstMessage *msg,
          gpointer user_data)
{
    app_t *app = (app_t *) user_data;
    switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_EOS:
    {
        g_message ("End-of-stream");
        //g_main_loop_quit (app->main_loop);
	gst_element_set_state (GST_ELEMENT (app->pipeline), GST_STATE_NULL);

        break;
    }
    case GST_MESSAGE_ERROR:
    {
        gchar *debug;
        GError *err;
        
        gst_message_parse_error (msg, &err, &debug);
        g_free (debug);
        
        g_error ("%s", err->message);
        g_error_free (err);
        
        break;
    }
    default:
        break;
    }
    
    return true;
}

void
play_uri (app_t *app, gchar *uri)
{
  gst_element_set_state (GST_ELEMENT (app->pipeline), GST_STATE_NULL);
  if (uri) {
    g_object_set (G_OBJECT (app->pipeline), "uri", uri, NULL);
  }
  gst_element_set_state (GST_ELEMENT (app->pipeline), GST_STATE_PLAYING);
}


void
say_this (app_t *app, gchar *text)
{
	gst_element_set_state (GST_ELEMENT (app->pipeline), GST_STATE_NULL);

    char cmd[1024];     
    sprintf(cmd,"echo \"%s\" | festival --tts",text);
    if (app->verbose) 
        printf("%s",cmd);
    int status = system(cmd);
}


static void
app_destroy(app_t *app)
{
    if (app) {
        if (app->event)
            free(app->event);
        if (app->pipeline) {
            gst_element_set_state (GST_ELEMENT (app->pipeline), GST_STATE_NULL);
            gst_object_unref (GST_OBJECT (app->pipeline));
        }

        free(app);
    }
}

static void
app_event(app_t *app, app_events_enum_t event)
{
  if (app->event[event].uri) {
      char uri[1024];
      sprintf(uri,"file://%s/sounds/%s",CONFIG_DIR,app->event[event].uri);
      if (app->verbose) {
          fprintf(stdout,"event[%s]: uri=%s",app_event_name[event],uri);
          fflush(stdout);
      }
      play_uri(app,uri);
  }
  else if (app->event[event].say) {
      say_this(app,app->event[event].say);
  }
  else  {
      printf("WARNING: I have no sound/speech for this event!!!\n");
  }

}

static void
on_audio_param_event (const lcm_recv_buf_t *buf, const char *channel, 
        const navlcm_audio_param_t *msg, void *user)
{
    app_t *self = (app_t*)user;

    if (msg->code == AUDIO_PLAY) {
        app_event (self, (app_events_enum_t)atoi (msg->name));
    } else if (msg->code == AUDIO_SAY) {
        say_this (self, msg->name);
    }
}

app_t *
app_create(const char *device)
{
    app_t *self = (app_t*)calloc(1, sizeof(app_t));
    if (!self) {
        dbg (DBG_ERROR,"Error: app_create() failed to allocate self\n");
        return NULL;
    }
    if (device)
        self->device = strdup(device);
    self->lcm = lcm_create (NULL);
    
   self->cfg = config_parse_default ();
    if (!self->cfg) {
        dbg (DBG_ERROR, "Could not reach config file.");
        return NULL;
    }
    
    self->nevents = N_EVENTS;
    self->event = (fx_event_t*)calloc(self->nevents, sizeof(fx_event_t));

    int i=0;
    for (fx_event_t *s = self->event ;s < self->event+self->nevents;s++) {
        char cfg_base_name[256];
        s->uri = NULL;
        s->say = NULL;
        snprintf(cfg_base_name,256,"soundfx.%s",app_event_name[i++]);
        char keystr[256];
        sprintf (keystr, "%s.uri", cfg_base_name);
        if (config_get_str (self->cfg, keystr, &s->uri)) {
        }
        sprintf (keystr, "%s.say", cfg_base_name);
        if (config_get_str (self->cfg, keystr, &s->say)) {
        }
        if (!s->say&&!s->uri) {
            printf("%s: WARNING: I have no sound/speech for this event!!!\n",keystr);
        }
    }    

    // gstreamer stuff
    self->pipeline = gst_element_factory_make ("playbin", "player");
    self->bus = gst_pipeline_get_bus (GST_PIPELINE (self->pipeline));

    self->audiosink = gst_element_factory_make("alsasink", "sink");
    g_object_set (G_OBJECT (self->audiosink), "device", self->device, NULL);
    g_object_set (G_OBJECT (self->pipeline), "audio-sink", self->audiosink, NULL);

    gst_bus_add_watch (self->bus, bus_call, self);
    gst_object_unref (self->bus);

    glib_mainloop_attach_lcm (self->lcm);

    /* subscribe to LCM message channel */
    navlcm_audio_param_t_subscribe (self->lcm, "AUDIO_PARAM", on_audio_param_event, self);
    return self;
}




static gboolean
on_input (GIOChannel * source, GIOCondition cond, gpointer data)
{
    app_t * fx = (app_t *) data;
    int c = getchar();
    int i = c-'0';
    if ((0<=i)&&(i<fx->nevents))
      app_event(fx,(app_events_enum_t)i);
    

    return TRUE;
}

static void usage()
{
    fprintf (stderr, "usage: app [options]\n"
             "\n"
             "  -h, --help             shows this help text and exits\n"
             "  -d, --device           sound device (default: NULL)\n"
             "  -v, --verbose          be verbose\n"
        );
}


int main(int argc, char *argv[])
{
    setlinebuf (stdout);
    gst_init(&argc,&argv);

    const char *optevent = "hd:vt:";
    char c;
    struct option long_opts[] = {
        {"help", no_argument, 0, 'h'},
        {"device", required_argument, 0, 'd'},
        {"verbose", no_argument, 0, 'v'},
        {"test", required_argument, 0, 't'},
        {0, 0, 0, 0}};
    
    const char *device="front:CARD=Intel";
    int verbose=0;
    int test=-1;
    while ((c = getopt_long (argc, argv, optevent, long_opts, 0)) >= 0)
    {
        switch (c) 
        {
        case 'd':
            device = strdup (optarg);
            break;
        case 't':
            test = atoi(optarg);
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
        default:
            usage();
            return 1;
        }
    }

    app_t *fx = app_create(device);
    if (!fx)
        return -1;
    fx->verbose = verbose;


    // test sound effect
    if (test>=0) {
        app_event(fx,(app_events_enum_t)test);
        /* Watch stdin */
        GIOChannel * channel = g_io_channel_unix_new (0);
        g_io_add_watch (channel, G_IO_IN, on_input, fx);
    }

    /* Main Loop */
    GMainLoop *main_loop = g_main_loop_new(NULL, FALSE);
    if (!main_loop) {
        dbg (DBG_ERROR,"Error: Failed to create the main loop\n");
        return -1;
    }

    // connect to kill signal
    signal_pipe_glib_quit_on_kill (main_loop);

    /* sit and wait for messages */
    g_main_loop_run(main_loop);
    
    /* clean */
    app_destroy(fx);
    return 0;
}
