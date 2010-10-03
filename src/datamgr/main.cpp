#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

/* From GLIB */
#include <glib.h>
#include <glib-object.h>

/* From LCM */
#include <lcm/lcm.h>
#include <lcmtypes/navlcm_log_info_t.h>

/* From common */
#include <common/getopt.h>
#include <common/dbg.h>
#include <common/codes.h>

/* From common */
#include <common/glib_util.h>
#include <common/ioutils.h>
#include <common/config_util.h>

#define INDEX_FILE "_index.txt"

struct state_t {

    int verbose;
    GMainLoop *loop;
    lcm_t *lcm;
    config_t *config;
    GHashTable *hash;
};

static state_t *g_self;
gboolean publish_log_info_data (gpointer data);

char* search_index_file (state_t *self, const char *filename)
{
    char *indexfn = g_strconcat (self->config->data_dir, "/", INDEX_FILE, NULL);
    FILE *fp = fopen (indexfn, "r");
    g_free (indexfn);

    if (!fp)
        return strdup ("");

    char line[200];
    while (fgets (line, 199, fp)!=NULL) {
        if (strncmp (line, filename, strlen (filename))==0) {
            char **sp = g_strsplit (line, " ", 0);
            char *r = g_strchomp (g_strjoinv (" ", sp+1));
            fclose (fp);
            g_strfreev (sp);
            return r;
        }
    }

    fclose (fp);
    return strdup ("");
}

gboolean dump_to_index_file (gpointer data)
{
    state_t *self = (state_t*)data;

    char *indexfn = g_strconcat (self->config->data_dir, "/", INDEX_FILE, NULL);
    FILE *fp = fopen (indexfn, "w");
    g_free (indexfn);

    if (!fp)
        return TRUE;

    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init (&iter, self->hash);
    while (g_hash_table_iter_next (&iter, &key, &value)) 
    {
        char *name = (char*)key;
        char *desc = (char*)value;
        fprintf (fp, "%s %s\n", name, desc);
    }

    fclose (fp);
    return TRUE;
}

    static void
on_log_info_set_event (const lcm_recv_buf_t *buf, const char *channel, 
        const navlcm_log_info_t *msg,
        void *user)
{
    state_t *self = (state_t*)user;

    if (msg->code == DATAMGR_SET) {
        // update the desc for a file
        if (self->hash) {
            for (int i=0;i<msg->num;i++) {
                dbg (DBG_INFO, "setting desc for %s to : %s", msg->filename[i], msg->desc[i]);
                g_hash_table_replace (self->hash, strdup(msg->filename[i]), strdup (msg->desc[i]));
            }
        }
        // dump index file
        dump_to_index_file (self);

        // publish log info
        publish_log_info_data (self);
    } 

    if (msg->code == DATAMGR_REQ) {
        // publish log info
        publish_log_info_data (self);
    }

    if (msg->code == DATAMGR_REM) {
        for (int i=0;i<msg->num;i++) {
            char *dstname = g_strconcat ("/home/",getenv("LOGNAME"),"/.Trash/",msg->filename[i],NULL);
            char *srcname = msg->fullname[i];
            if (rename (srcname, dstname) < 0) {
                dbg (DBG_ERROR, "failed to rename %s into %s", srcname, dstname);
            }
            free (dstname);
            g_hash_table_remove (self->hash, msg->filename[i]);
        }

        // dump index file
        dump_to_index_file (self);

        // publish log info
        publish_log_info_data (self);
    }

    if (msg->code == DATAMGR_LST) {
        // do nothing
    }
}

void list_files (state_t *self)
{
    DIR *dir = opendir (self->config->data_dir);
    if (!dir) {
        dbg (DBG_INFO, "warning: data dir %s does not exist.", self->config->data_dir);
        return;
    }

    struct dirent *dt;
    self->hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

    while ((dt = readdir (dir))!=NULL) {
        // must end in .log
        if (strncmp (g_strreverse (dt->d_name), "gol.",4)!=0) continue;
        g_strreverse (dt->d_name);

        char *name = g_strconcat (dt->d_name, NULL);
        char *desc = search_index_file (self, dt->d_name);
        printf ("%s %s\n", dt->d_name, desc);
        g_hash_table_insert (self->hash, name, desc);
    }

    closedir (dir);
}

gboolean publish_log_info_data (gpointer data)
{
    state_t *self = (state_t*)data;
    navlcm_log_info_t in;

    if (self->hash) {
        in.code = DATAMGR_LST;
        in.num = g_hash_table_size (self->hash);
        in.filename = (char**)malloc(in.num*sizeof(char*));
        in.desc = (char**)malloc(in.num*sizeof(char*));
        in.fullname = (char**)malloc(in.num*sizeof(char*));
        in.filesize = (int64_t*)malloc(in.num*sizeof(int64_t));

        GList *keys = g_hash_table_get_keys (self->hash);
        if (keys) {
            GList *sorted = g_list_reverse (g_list_sort (g_list_copy (keys), (GCompareFunc)g_strcmp0));
            int c=0;
            for (GList *iter=g_list_first(sorted);iter;iter=g_list_next(iter)) {
                char *name=(char*)iter->data;
                gpointer data = g_hash_table_lookup (self->hash, name);
                if (data) {
                    char *desc = (char*)data;
                    in.filename[c] = name;
                    in.desc[c] = desc;
                    in.fullname[c] = (char*)malloc(256);
                    struct stat st;
                    sprintf (in.fullname[c], "%s/%s", self->config->data_dir, in.filename[c]);
                    if (stat (in.fullname[c], &st)==0) {
                        in.filesize[c]=st.st_size;
                    } else {
                        dbg (DBG_ERROR, "failed to stat file %s", in.fullname[c]);
                        in.filesize[c]=0;
                    }
                    c++;
                }
            }
            g_list_free (sorted);
        }

        g_list_free (keys);

        if (in.num>0) {
            dbg (DBG_INFO, "publishing log info data with %d files.", in.num);
            navlcm_log_info_t_publish (self->lcm, "LOG_INFO", &in);
        }

        for (int i=0;i<in.num;i++)
            free (in.fullname[i]);
        free (in.filename);
        free (in.desc);
        free (in.fullname);
        free (in.filesize);
    }

    return TRUE;
}

int main(int argc, char *argv[])
{
    g_type_init ();

    dbg_init ();

    // initialize application state and zero out memory
    g_self = (state_t*) calloc( 1, sizeof(state_t) );
    state_t *self = g_self;



    // run the main loop
    self->loop = g_main_loop_new(NULL, FALSE);

    getopt_t *gopt = getopt_create();

    getopt_add_bool  (gopt, 'h',   "help",    0,        "Show this help");
    getopt_add_bool  (gopt, 'v',   "verbose",    0,     "Be verbose");

    if (!getopt_parse(gopt, argc, argv, 1) || getopt_get_bool(gopt,"help")) {
        printf("Usage: %s [options]\n\n", argv[0]);
        getopt_do_usage(gopt);
        return 0;
    }

    self->verbose = getopt_get_bool(gopt, "verbose");

    self->lcm = lcm_create(NULL);
    if (!self->lcm)
        return 1;

    // read config file
    if (!(self->config = read_config_file ())) {
        dbg (DBG_ERROR, "[viewer] failed to read config file.");
        return -1;
    }


    // attach lcm to main loop
    glib_mainloop_attach_lcm (self->lcm);

    // publish cam settings every now and then
    // g_timeout_add_seconds (2, &publish_log_info_data, self);
    // g_timeout_add_seconds (2, &dump_to_index_file, self);

    // listen to tablet event
    navlcm_log_info_t_subscribe (self->lcm, "LOG_INFO", on_log_info_set_event, self);

    list_files (self);

    publish_log_info_data (self);

    // connect to kill signal
    signal_pipe_glib_quit_on_kill (self->loop);

    // run main loop
    g_main_loop_run (self->loop);

    // cleanup
    g_main_loop_unref (self->loop);

    return 0;
}
