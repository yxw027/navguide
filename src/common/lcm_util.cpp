#include "lcm_util.h"

void publish_phone_command (lcm_t *lcm, double theta)
{
    navlcm_phone_command_t msg;
    msg.theta = theta;
    navlcm_phone_command_t_publish (lcm, "PHONE_COMMAND", &msg);
}

void publish_audio_param (lcm_t *lcm, int code, const char *name, double val)
{
    navlcm_audio_param_t ap;
    ap.code = code;
    ap.val = val;
    ap.name = name ? strdup (name) : strdup ("");

    navlcm_audio_param_t_publish (lcm, "AUDIO_PARAM", &ap);
}

void publish_nav_order (lcm_t *lcm, double angle, double progress, 
        int node, int code, double error)
{
    navlcm_nav_order_t nav;
    nav.angle = angle;
    nav.progress = progress;
    nav.node = node;
    nav.code = code;
    nav.error = error;

    navlcm_nav_order_t_publish (lcm, "NAV_ORDER", &nav);

}

void publish_phone_msg (lcm_t *lcm, const char *string, ...) 
{
    if (strlen(string) < 128) {

        char buf[256];

        va_list arg_ptr;

        va_start (arg_ptr,string);

        vsprintf(buf,string,arg_ptr);

        va_end (arg_ptr);

        navlcm_phone_print_t msg;
        msg.text = buf;
        navlcm_phone_print_t_publish (lcm, "PHONE_PRINT", &msg);

    }
}

void publish_phone_param_in (lcm_t *lcm, int code, const char *name)
{
    navlcm_phone_param_t pp;
    pp.code = code;
    pp.name = strdup (name);

    navlcm_phone_param_t_publish (lcm, "PHONE_PARAM_IN", &pp);
}

void publish_phone_param_out (lcm_t *lcm, int code, const char *name)
{
    navlcm_phone_param_t pp;
    pp.code = code;
    pp.name = strdup (name);

    navlcm_phone_param_t_publish (lcm, "PHONE_PARAM_OUT", &pp);
}

void publish_features_param (lcm_t *lcm, int code)
{
    navlcm_features_param_t sp;
    sp.code = code;

    navlcm_features_param_t_publish (lcm, "FEATURES_PARAM", &sp);
}   

// free feature list
//
void free_feature_list (navlcm_feature_list_t *f)
{
    if (!f || f->num == 0)
        return;

    free (f->el);
    f->num = 0;
    f->el = NULL;
}

navlcm_system_info_t *navlcm_system_info_t_create ()
{
    navlcm_system_info_t* sysinfo = (navlcm_system_info_t*)malloc(sizeof(navlcm_system_info_t));

    return sysinfo;
}

void navlcm_system_info_t_print (navlcm_system_info_t *s)
{
    printf ("Temp critical: %d C\n", s->temp_critical);
    printf ("Temp CPU: %d C\n", s->temp_cpu);
    printf ("Discharge rate: %d mA\n", s->battery_discharge_rate);
    printf ("Design capacity: %d mA\n", s->battery_design_capacity_mA);
    printf ("Remaining capacity: %d mA\n", s->battery_remaining_mA);
    //printf ("cpu_user: %d\n", s->cpu_user);
    //printf ("cpu_user_low: %d\n", s->cpu_user_low);
    //printf ("cpu_system: %d\n", s->cpu_system);
    //printf ("cpu_idle: %d\n", s->cpu_idle);

    printf ("memtotal: %ld\n", s->memtotal);
    printf ("memfree: %ld\n", s->memfree);
    printf ("swaptotal: %ld\n", s->swaptotal);
    printf ("swapfree: %ld\n", s->swapfree);

    printf ("cpu usage: %.1f %%\n", 100.0 * s->cpu_usage);
    printf ("mem usage: %.1f %%\n", 100.0 * s->mem_usage);
    printf ("swap usage: %.1f %%\n", 100.0 * s->swap_usage);

    printf ("Disk space remaining: %.1f GB\n", s->disk_space_remaining_gb);

    printf ("Sound Y/N: %d\n", s->sound_enabled);
}
// create
//
// create an empty feature
//
navlcm_feature_t* navlcm_feature_t_create ()
{
    navlcm_feature_t *f = (navlcm_feature_t*)malloc(sizeof(navlcm_feature_t));

    f->col = 0.0;
    f->row = 0.0;
    f->ori = 0.0;
    f->scale = 0;
    f->sensorid = 0;
    f->size = 0;
    f->index = 0;
    f->utime = 0;
    f->uid = 0;
    f->data = NULL;

    return f;
}

navlcm_feature_t* navlcm_feature_t_create (int sensorid, double col, double row,
        int index, double scale)
{
    navlcm_feature_t *f = (navlcm_feature_t*)malloc(sizeof(navlcm_feature_t));

    f->col = col;
    f->row = row;
    f->scale = scale;
    f->sensorid = sensorid;
    f->ori = 0.0;
    f->size = 0;
    f->index = 0;
    f->utime = 0;
    f->uid = 0;
    f->data = NULL;

    return f;
}

void navlcm_feature_t_clear (navlcm_feature_t* ft)
{
    free (ft->data);
}

void navlcm_feature_match_t_clear (navlcm_feature_match_t *m)
{
    free (m->src.data);
    for (int i=0;i<m->num;i++)
        free (m->dst[i].data);
}

void navlcm_feature_t_print (navlcm_feature_t* ft)
{
    assert (ft);

    printf ("[%d] [%ld] [%.2f, %.2f, %.2f, %.2f] %d: %.3f, %.3f, %.3f, ...\n",
            ft->index, ft->utime, ft->col, ft->row, ft->scale, ft->ori, ft->size,
            ft->data[0], ft->data[1], ft->data[2]);
    for (int i=0;i<ft->size;i++) printf ("%.3f ", ft->data[i]);
    printf ("\n");
    fflush (stdout);
}

navlcm_feature_t *navlcm_feature_t_find_by_index (navlcm_feature_list_t *features, int index)
{
    for (int i=0;i<features->num;i++) {
        if (features->el[i].index == index)
            return features->el + i;
    }
    return NULL;
}

navlcm_feature_list_t* navlcm_feature_list_t_create ()
{
    return navlcm_feature_list_t_create (0, 0, 0, 0, 0);
}

navlcm_feature_list_t* navlcm_feature_list_t_create (int width, int height, int sensorid, int64_t utime, int desc_size)
{
    navlcm_feature_list_t *ls = 
        (navlcm_feature_list_t*)malloc(sizeof(navlcm_feature_list_t));

    ls->width = width;
    ls->height = height;
    ls->sensorid = sensorid;
    ls->utime= utime;
    ls->desc_size = desc_size;
    ls->feature_type = 0;
    ls->num = 0;
    ls->el = NULL;

    return ls;
}

    navlcm_feature_list_t* navlcm_feature_list_t_append 
(navlcm_feature_list_t* set, navlcm_feature_t *ft)
{
    assert (set);

    set->el = (navlcm_feature_t*)realloc 
        (set->el, (set->num+1)*sizeof(navlcm_feature_t));
    navlcm_feature_t *copy = navlcm_feature_t_copy (ft);
    memcpy (set->el + set->num, copy, sizeof(navlcm_feature_t));
    free (copy);

    set->num++;

    return set;

}

// search
gboolean navlcm_feature_list_t_search (navlcm_feature_list_t *fs, navlcm_feature_t *f)
{
    if (!fs) return false;

    for (int i=0;i<fs->num;i++) {
        if (f == fs->el + i)
            return true;
    }
    return false;
}

gboolean navlcm_feature_match_set_t_search (navlcm_feature_match_set_t *ms, navlcm_feature_match_t *m)
{
    if (!ms) return false;
    for (int i=0;i<ms->num;i++) {
        if (m == ms->el + i)
            return true;
    }
    return false;
}

gboolean navlcm_feature_match_t_equal_copy (navlcm_feature_match_t *m1, navlcm_feature_match_t *m2)
{
    if (m1->src.index == m2->src.index && (m1->num>0 && m2->num>0) && \
            m1->dst[0].index == m2->dst[0].index)
        return true;
    return false;
}

gboolean navlcm_feature_match_set_t_search_copy (navlcm_feature_match_set_t *ms, navlcm_feature_match_t *m)
{
    if (!ms) return false;
    for (int i=0;i<ms->num;i++) {
        navlcm_feature_match_t *k = ms->el + i;
        if (navlcm_feature_match_t_equal_copy (m, k))
            return true;
    }
    return false;
}

gboolean navlcm_feature_match_set_t_remove_copy (navlcm_feature_match_set_t *ms, navlcm_feature_match_t *m)
{
    if (!navlcm_feature_match_set_t_search_copy (ms, m)) return false;

    // last match in the list
    if (ms->num == 0) {
        navlcm_feature_match_t_destroy (ms->el);
        ms->el = NULL;
        ms->num = 0;
        return true;
    }

    navlcm_feature_match_t *set = (navlcm_feature_match_t*)malloc((ms->num-1)*sizeof(navlcm_feature_match_t));
    int count=0;
    for (int i=0;i<ms->num;i++) {
        if (!navlcm_feature_match_t_equal_copy (ms->el + i, m)) {
            memcpy (set + count, ms->el+i, sizeof(navlcm_feature_match_t));
            count++;
        } 
    }
    assert (count == ms->num-1);

    navlcm_feature_match_t_clear (m);
    free (ms->el);
    ms->el = set;
    ms->num = count;

    return TRUE;

}
gboolean navlcm_feature_match_set_t_remove (navlcm_feature_match_set_t *ms, navlcm_feature_match_t *m)
{
    if (!navlcm_feature_match_set_t_search (ms, m)) return false;

    // last match in the list
    if (ms->num == 0) {
        navlcm_feature_match_t_destroy (ms->el);
        ms->el = NULL;
        ms->num = 0;
        return true;
    }

    navlcm_feature_match_t *set = (navlcm_feature_match_t*)malloc((ms->num-1)*sizeof(navlcm_feature_match_t));
    int count=0;
    for (int i=0;i<ms->num;i++) {
        if (ms->el + i != m) {
            memcpy (set + count, ms->el+i, sizeof(navlcm_feature_match_t));
            count++;
        } 
    }
    assert (count == ms->num-1);

    navlcm_feature_match_t_clear (m);
    free (ms->el);
    ms->el = set;
    ms->num = count;

    return TRUE;

}

gboolean navlcm_feature_list_t_remove (navlcm_feature_list_t *fs, navlcm_feature_t *f)
{
    if (!navlcm_feature_list_t_search (fs, f)) return FALSE;

    // last keypoint in the list
    if (fs->num == 1) {
        navlcm_feature_t_destroy (fs->el);
        fs->el = NULL;
        fs->num = 0;
        return TRUE;
    }

    navlcm_feature_t *set = (navlcm_feature_t*)malloc((fs->num-1)*sizeof(navlcm_feature_t));
    int count=0;
    for (int i=0;i<fs->num;i++) {
        if (fs->el + i != f) {
            memcpy (set + count, fs->el+i, sizeof(navlcm_feature_t));
            count++;
        } 
    }
    assert (count == fs->num-1);

    navlcm_feature_t_clear (f);
    free (fs->el);
    fs->el = set;
    fs->num = count;

    return TRUE;
}

// insert head
    void navlcm_feature_list_t_insert_head 
(navlcm_feature_list_t* set, navlcm_feature_t *ft)
{
    set->el = (navlcm_feature_t*)realloc
        (set->el, (set->num+1)*sizeof(navlcm_feature_t));

    for (int i=set->num-1;i>=0;i--) 
        memcpy (set->el + i + 1, set->el + i, sizeof(navlcm_feature_t));

    navlcm_feature_t *copy = navlcm_feature_t_copy (ft);
    memcpy (set->el, copy, sizeof(navlcm_feature_t));
    free (copy);

    set->num++;
}

    navlcm_feature_list_t* navlcm_feature_list_t_insert_tail
(navlcm_feature_list_t* set, navlcm_feature_t *ft)
{
    set->el = (navlcm_feature_t*)realloc
        (set->el, (set->num+1)*sizeof(navlcm_feature_t));

    set->el[set->num] = *ft;
    set->num++;
    return set;
}


// append
    navlcm_feature_list_t*
navlcm_feature_list_t_append (navlcm_feature_list_t *set1, 
        navlcm_feature_list_t *set2)
{
    if (!set1) return set2;
    if (!set2) return set1;

    int num = set1->num + set2->num;

    set1->el = (navlcm_feature_t*)realloc
        (set1->el, num*sizeof(navlcm_feature_t));

    int count = set1->num;

    // insert elements from set2 into set1
    //
    for (int i=0;i<set2->num;i++) {

        navlcm_feature_t *copy = navlcm_feature_t_copy (set2->el + i);
        memcpy (set1->el + count, copy, sizeof(navlcm_feature_t));
        free (copy);

        count++;
    }

    set1->num = num;

    // renumber features
    //
    for (int i=0;i<num;i++)
        set1->el[i].index = i;

    return set1;
}

    navlcm_feature_match_set_t *navlcm_feature_match_set_t_append 
(navlcm_feature_match_set_t *set1,
 navlcm_feature_match_set_t *set2)
{
    int set1num = set1 ? set1->num : 0;
    int set2num = set2 ? set2->num : 0;

    navlcm_feature_match_set_t *set = 
        (navlcm_feature_match_set_t *)malloc
        (sizeof(navlcm_feature_match_set_t));
    set->num = 0;
    set->el = NULL;

    for (int i=0;i<set1num;i++) {
        set->el = (navlcm_feature_match_t *)realloc 
            (set->el, (set->num+1)*sizeof(navlcm_feature_match_t));

        navlcm_feature_match_t *copy = 
            navlcm_feature_match_t_copy (set1->el + i);

        memcpy (set->el + set->num, copy, sizeof (navlcm_feature_match_t));

        free (copy);

        set->num++;
    }

    for (int i=0;i<set2num;i++) {
        set->el = (navlcm_feature_match_t *)realloc 
            (set->el, (set->num+1)*sizeof(navlcm_feature_match_t));

        navlcm_feature_match_t *copy = 
            navlcm_feature_match_t_copy (set2->el + i);

        memcpy (set->el + set->num, copy, sizeof (navlcm_feature_match_t));

        free (copy);

        set->num++;
    }

    return set;
}

    navlcm_feature_match_set_t*
navlcm_feature_match_set_t_create ()
{
    navlcm_feature_match_set_t *set =
        (navlcm_feature_match_set_t *)malloc(sizeof
                (navlcm_feature_match_set_t));
    set->el = NULL;
    set->num = 0;
    return set;
}

// create a feature match with zero matches
//
    navlcm_feature_match_t* navlcm_feature_match_t_create 
(navlcm_feature_t *key)
{
    navlcm_feature_match_t *match = (navlcm_feature_match_t*)malloc
        (sizeof(navlcm_feature_match_t));

    navlcm_feature_t *copy = navlcm_feature_t_copy (key);
    memcpy (&match->src, copy, sizeof(navlcm_feature_t));
    free (copy);

    match->dst = NULL;
    match->dist = NULL;
    match->num = 0;

    return match;
}

// create a track from two features
//
    navlcm_track_t*
navlcm_track_t_create (navlcm_feature_t *key, int width, int height,
        int ttl, int64_t uid)
{
    navlcm_track_t* track = (navlcm_track_t*)malloc
        (sizeof(navlcm_track_t));

    track->ttl = ttl;
    track->uid = uid;
    track->averror = 0.0;
    track->time_start = 0;
    track->time_end = 0;
    track->ft.num = 0;
    track->ft.utime = 0;
    track->ft.sensorid = 0;
    track->ft.width = width;
    track->ft.height = height;
    track->ft.el = NULL; 
    if (key)
        navlcm_track_t_insert_head (track, key);

    return track;
}

    navlcm_track_t*
navlcm_track_t_append (navlcm_track_t *track, navlcm_feature_t *ft)
{
    assert (track && ft);

    navlcm_feature_list_t_append (&track->ft, ft);
    return track;
}

    navlcm_track_t*
navlcm_track_t_insert_head (navlcm_track_t *track, navlcm_feature_t *ft)
{
    assert (track && ft);

    navlcm_feature_list_t_insert_head (&track->ft, ft);

    return track;
}

    navlcm_track_t*
navlcm_track_t_insert_tail (navlcm_track_t *track, navlcm_feature_t *ft)
{
    track->ft = *(navlcm_feature_list_t_insert_tail (&track->ft, ft));
    return track;
}

    navlcm_track_set_t* 
navlcm_track_set_t_append (navlcm_track_set_t *set, navlcm_track_t *tr)
{
    set->el = (navlcm_track_t*)realloc 
        (set->el, (set->num+1)*sizeof(navlcm_track_t));

    navlcm_track_t *copy = navlcm_track_t_copy (tr);
    memcpy (set->el + set->num, copy, sizeof(navlcm_track_t));
    free (copy);
    set->num=set->num+1;

    return set;
}

    navlcm_track_t*
navlcm_track_t_concat (navlcm_track_t *src, navlcm_track_t *sup)
{
    assert (src->time_start >= sup->time_end);

    src->time_start = sup->time_start;
    src->averror = (src->averror + sup->averror)/2;

    navlcm_feature_t *tl = src->ft.el + src->ft.num-1;

    while (sup->time_end < src->time_start) {
        src = navlcm_track_t_insert_tail (src, tl);
        src->time_start--;
    }
    for (int i=0;i<sup->ft.num;i++) {
        src = navlcm_track_t_insert_tail (src, sup->ft.el + i);
    }
    return src;
}

    navlcm_track_t*
navlcm_track_t_erase (navlcm_track_t* t)
{
    t->ttl = 0;
    t->time_start = t->time_end = 0;
    return t;
}

// insert a match in a match structure
//
    navlcm_feature_match_t*
navlcm_feature_match_t_insert (navlcm_feature_match_t *match,
        navlcm_feature_t *key, double dist)
{
    match->dst = (navlcm_feature_t*)realloc 
        (match->dst, (match->num+1)*sizeof(navlcm_feature_t));

    match->dst[match->num] = *key;

    match->dist = (double*)realloc (match->dist, (match->num+1)*(sizeof(double)));
    match->dist[match->num] = dist;

    match->num++;

    return match;

}

    void
navlcm_feature_match_set_t_print (navlcm_feature_match_set_t *ms)
{
    for (int i=0;i<ms->num;i++)
        navlcm_feature_match_t_print (ms->el + i);
}

    void
navlcm_feature_match_t_print (navlcm_feature_match_t *m)
{
    printf ("match : src[%d,%d] -- ", m->src.sensorid, m->src.index);
    for (int k=0;k<m->num;k++) {
        printf ("[%d,%d,%.3f] ", m->dst[k].sensorid, m->dst[k].index, 
                m->dist[k]);
    }
    printf ("\n"); fflush(stdout);
}

// insert a match in a set
//
void navlcm_feature_match_set_t_insert (
        navlcm_feature_match_set_t *set, 
        navlcm_feature_match_t *match)
{
    if (!set) return;

    set->el = (navlcm_feature_match_t*)realloc (
            set->el, (set->num+1)*sizeof(navlcm_feature_match_t));

    set->el[set->num] = *match;

    set->num++;
}

int navlcm_feature_match_set_t_read (navlcm_feature_match_set_t *data, FILE *fp)
{
    if (!fp || feof(fp)) return -1;

    int size=0;
    fread (&size, sizeof(int), 1, fp);
    if (size == 0)
        return -1;
    unsigned char *buffer = (unsigned char*)malloc(size);
    int status = fread (buffer, size, 1, fp);
    navlcm_feature_match_set_t_decode (buffer, 0, size, data);
    free (buffer);
    return status == 1 ? 0 : -1;

}
// assume fp has been open in "wb" mode
//
int navlcm_feature_match_set_t_write (const navlcm_feature_match_set_t *data, FILE *fp)
{
    if (!fp)        return -1;
    int size = 0;

    if (!data) {
        fwrite (&size, sizeof(int), 1, fp);
        return 0;
    }
    
    size = navlcm_feature_match_set_t_encoded_size (data);
    unsigned char *buffer = (unsigned char*)malloc(size);
    navlcm_feature_match_set_t_encode (buffer, 0, size, data);
    fwrite (&size, sizeof(int), 1, fp);
    int status = fwrite (buffer, size, 1, fp);
    free (buffer);
    return status == 1 ? 0 : -1;
}
// assume fp has been open in "wb" mode
//

int navlcm_feature_t_equal (navlcm_feature_t *f1, navlcm_feature_t *f2)
{
    if (f1->sensorid != f2->sensorid)   return 0;
    if (fabs(f1->col - f2->col) > 1E-6) return 0;
    if (fabs(f1->row - f2->row) > 1E-6) return 0;

    // compare descriptors
    for (int i=0;i<f1->size;i++) {
        if (f1->data[i] != f2->data[i])
            return 0;
    }

    return 1;
}

int navlcm_track_t_write (const navlcm_track_t *track, FILE *fp)
{
    if (!fp) return -1;
    int size = 0;

    if (!track) {
        fwrite (&size, sizeof(int), 1, fp);
        return 0;
    }

    size = navlcm_track_t_encoded_size (track);
    unsigned char *buffer = (unsigned char*)malloc(size);
    navlcm_track_t_encode (buffer, 0, size, track);
    fwrite (&size, sizeof(int), 1, fp);
    int status = fwrite (buffer, size, 1, fp);
    free (buffer);
    return status == 1 ? 0 : -1;
}

int navlcm_track_t_read (navlcm_track_t *track, FILE *fp)
{
    if (!fp || feof(fp)) return -1;

    int size=0;
    fread (&size, sizeof(int), 1, fp);
    if (size==0) 
        return -1;

    unsigned char *buffer = (unsigned char*)malloc(size);
    int status = fread (buffer, size, 1, fp);
    navlcm_track_t_decode (buffer, 0, size, track);
    free (buffer);

    return status == 1 ? 0 : -1;
}

void navlcm_feature_list_t_clear (navlcm_feature_list_t *ft)
{
    assert (ft);
    ft->width = 0;
    ft->height = 0;
    ft->utime = 0;
    ft->desc_size = -1;
    ft->sensorid = -1;
    ft->num = 0;
    ft->el = NULL;
}

int navlcm_feature_t_read (navlcm_feature_t *ft, FILE *fp)
{
    if (!fp || feof (fp)) return -1;
    
    int size = 0;
    fread (&size, sizeof(int), 1, fp);
    if (size == 0)
        return -1;

    unsigned char *buffer = (unsigned char*)malloc(size);
    int status = fread (buffer, size, 1, fp);
    navlcm_feature_t_decode (buffer, 0, size, ft);
    free (buffer);
    return status == 1 ? 0 : -1;
}

int navlcm_feature_t_write (navlcm_feature_t *ft, FILE *fp)
{
    if (!fp) return -1;
    int size = 0;

    if (!ft) {
        fwrite (&size, sizeof(int), 1, fp);
        return 0;
    }

    size = navlcm_feature_t_encoded_size (ft);
    unsigned char *buffer = (unsigned char*)malloc(size);
    navlcm_feature_t_encode (buffer, 0, size, ft);
    fwrite (&size, sizeof(int), 1, fp);
    int status = fwrite (buffer, size, 1, fp);
    free (buffer);
    return status == 1 ? 0 : -1;
}   

int navlcm_feature_list_t_write (const navlcm_feature_list_t *data, FILE *fp)
{
    if (!fp) return -1;
    int size = 0;

    if (!data) {
        fwrite (&size, sizeof(int), 1, fp);
        return 0;
    }

    size = navlcm_feature_list_t_encoded_size (data);
    unsigned char *buffer = (unsigned char*)malloc(size);
    navlcm_feature_list_t_encode (buffer, 0, size, data);
    fwrite (&size, sizeof(int), 1, fp);
    int status = fwrite (buffer, size, 1, fp);
    free (buffer);
    return status == 1 ? 0 : -1;
}

// assume fp has been open in "rb" mode
//
int navlcm_feature_list_t_read (navlcm_feature_list_t *data, FILE *fp)
{
    if (!fp || feof(fp) || !data) return -1;

    int size = 0;
    fread (&size, sizeof(int), 1, fp);
    if (size == 0)
        return -1;
    unsigned char *buffer = (unsigned char*)malloc(size);
    int status = fread (buffer, size, 1, fp);
    navlcm_feature_list_t_decode (buffer, 0, size, data);
    free (buffer);
    return status == 1 ? 0 : -1;
}

int botlcm_pose_t_write (const botlcm_pose_t *data, FILE *fp)
{
    if (!fp) return -1;
    int size = 0;

    if (!data) {
        fwrite (&size, sizeof(int), 1, fp);
        return 0;
    }

    size = botlcm_pose_t_encoded_size (data);
    unsigned char *buffer = (unsigned char*)malloc(size);
    botlcm_pose_t_encode (buffer, 0, size, data);
    fwrite (&size, sizeof(int), 1, fp);
    int status = fwrite (buffer, size, 1, fp);
    free (buffer);
    return status == 1 ? 0 : -1;
}

// assume fp has been open in "rb" mode
//
int botlcm_pose_t_read (botlcm_pose_t *data, FILE *fp)
{
    if (!fp || feof(fp) || !data) return -1;

    int size = 0;
    fread (&size, sizeof(int), 1, fp);
    if (size == 0)
        return -1;
    unsigned char *buffer = (unsigned char*)malloc(size);
    int status = fread (buffer, size, 1, fp);
    botlcm_pose_t_decode (buffer, 0, size, data);
    free (buffer);
    return status == 1 ? 0 : -1;
}

navlcm_gps_to_local_t *navlcm_gps_to_local_t_new (int64_t utime)
{
    navlcm_gps_to_local_t *g = (navlcm_gps_to_local_t*)malloc(sizeof(navlcm_gps_to_local_t));
    g->utime = utime;
    for (int i=0;i<3;i++)
        g->local[i] = .0;
    for (int i=0;i<4;i++)
        g->lat_lon_el_theta[i] = .0;
    for (int i=0;i<4;i++) 
        for (int j=0;j<4;j++)
            g->gps_cov[i][j] = .0;

    return g;
}

int navlcm_gps_to_local_t_write (const navlcm_gps_to_local_t *data, FILE *fp)
{
    if (!fp) return -1;

    int size=0;

    if (!data) {
        fwrite (&size, sizeof(int), 1, fp);
        return 0;
    }

    size = navlcm_gps_to_local_t_encoded_size (data);
    unsigned char *buffer = (unsigned char*)malloc(size);
    navlcm_gps_to_local_t_encode (buffer, 0, size, data);
    fwrite (&size, sizeof(int), 1, fp);
    int status = fwrite (buffer, size, 1, fp);
    free (buffer);
    return status == 1 ? 0 : -1;
}

// assume fp has been open in "rb" mode
//
int navlcm_gps_to_local_t_read (navlcm_gps_to_local_t *data, FILE *fp)
{
    if (!fp || feof(fp)) return -1;

    int size = 0;
    fread (&size, sizeof(int), 1, fp);
    if (size == 0)
        return -1;
    unsigned char *buffer = (unsigned char*)malloc(size);
    int status = fread (buffer, size, 1, fp);
    navlcm_gps_to_local_t_decode (buffer, 0, size, data);
    free (buffer);
    return status == 1 ? 0 : -1;
}

int botlcm_image_t_print (const botlcm_image_t *data)
{
    if (!data) return -1;

    printf ("%d x %d (%d metadata el.)\n", data->width, data->height, data->nmetadata);
    for (int i=0;i<data->nmetadata;i++) {
        printf ("metadata[%d]  key: %s\n", i, data->metadata[i].key);
        for (int j=0;j<data->metadata[i].n;j++) {
            printf ("val[%d] = %c\n", j, data->metadata[i].value[j]);
        }
    }

    return 0;
}

int botlcm_image_t_write (const botlcm_image_t *data, FILE *fp)
{
    if (!fp) return -1;
    int size = 0;

    if (!data) {
        fwrite (&size, sizeof(int), 1, fp);
        return 0;
    }

    size = botlcm_image_t_encoded_size (data);
    unsigned char *buffer = (unsigned char*)malloc(size);
    botlcm_image_t_encode (buffer, 0, size, data);
    fwrite (&size, sizeof(int), 1, fp);
    int status = fwrite (buffer, size, 1, fp);
    free (buffer);
    return status == 1 ? 0 : -1;
}

// assume fp has been open in "rb" mode
//
int botlcm_image_t_read (botlcm_image_t *data, FILE *fp)
{
    if (!fp || feof(fp)) return -1;

    int size = 0;
    fread (&size, sizeof(int), 1, fp);
    if (size == 0)
        return -1;
    unsigned char *buffer = (unsigned char*)malloc(size);
    int status = fread (buffer, size, 1, fp);
    botlcm_image_t_decode (buffer, 0, size, data);
    free (buffer);
    return status == 1 ? 0 : -1;
}

// random ////////////////////////////////////////////////////////////
//
navlcm_feature_t navlcm_feature_t_random (int width, int height, int size, int sensorid)
{
    navlcm_feature_t ft;

    ft.scale = 1.0;
    ft.col = 1.0*rand()/RAND_MAX*width;
    ft.row = 1.0*rand()/RAND_MAX*height;
    ft.size = size;
    ft.sensorid = sensorid;
    ft.uid = 0;
    ft.data = (float*)malloc(ft.size*sizeof(float));
    for (int i=0;i<size;i++) {
        ft.data[i] = 1.0*rand()/RAND_MAX;
    }

    return ft;
}



botlcm_image_t *botlcm_image_t_create (int width, int height, int stride, int size, 
        const unsigned char *data, int pixelformat, int64_t utime)
{
    botlcm_image_t *img = (botlcm_image_t*)malloc(sizeof(botlcm_image_t));

    img->width = width;
    img->height = height;
    img->row_stride = stride;
    img->utime = utime;
    img->pixelformat = pixelformat;
    img->size = size;
    img->data = (unsigned char*)malloc(size);
    memcpy (img->data, data, size);
    img->nmetadata = 0;
    img->metadata = NULL;
    return img;
}

void botlcm_image_t_clear (botlcm_image_t *img)
{
    assert (img);
    img->width = 0;
    img->height = 0;
    img->row_stride = 0;
    img->utime = 0;
    img->pixelformat = 0;
    img->size = 0;
    img->data = NULL;
    img->nmetadata = 0;
    img->metadata = NULL;
}

void botlcm_pose_t_clear (botlcm_pose_t *p)
{
    p->utime=0;
    for (int k=0;k<3;k++) {
        p->pos[k]=0;
        p->vel[k]=0;
        p->rotation_rate[k]=0;
        p->accel[k]=0.0;
    }
    p->orientation[0] = 1.0;
    for (int k=1;k<4;k++) {
        p->orientation[k]=0;
    }
}

void navlcm_gps_to_local_t_clear (navlcm_gps_to_local_t *g)
{
    g->utime=0;
    for (int k=0;k<3;k++) {
        g->local[k]=0;
    }
    for (int k=0;k<4;k++) {
        g->lat_lon_el_theta[k]=0;
        for (int kk=0;kk<4;kk++) {
            g->gps_cov[k][kk]=0;
        }
    }
}

/* assumes that data is stored in reverse chronos order in the queue
 * (that is, you used push_head to store the data there)
 */
navlcm_feature_list_t * find_feature_list_by_utime (GQueue *data, int64_t utime)
{
    if (!data || g_queue_is_empty (data)) {
        return NULL;
    }

    GList *iter = g_queue_peek_head_link (data);
    while (iter) {
        navlcm_feature_list_t *p = (navlcm_feature_list_t*)iter->data;
        if (p->utime <= utime) {
            navlcm_feature_list_t *q = NULL;
            if (iter->prev)
                q = (navlcm_feature_list_t*)(iter->prev->data);
            if (q && q->utime - utime < utime - p->utime)
                return q;
            return p;
        }
        iter = iter->next;
    }

    return NULL;
}

/* assumes that data is stored in reverse chronos order in the queue
 * (that is, you used push_head to store the data there)
 */
navlcm_imu_t * find_imu_by_utime (GQueue *data, int64_t utime)
{
    if (!data || g_queue_is_empty (data)) {
        return NULL;
    }

    GList *iter = g_queue_peek_head_link (data);
    while (iter) {
        navlcm_imu_t *p = (navlcm_imu_t*)iter->data;
        if (p->utime <= utime) {
            navlcm_imu_t *q = NULL;
            if (iter->prev)
                q = (navlcm_imu_t*)(iter->prev->data);
            if (q && q->utime - utime < utime - p->utime)
                return q;
            return p;
        }
        iter = iter->next;
    }

    return NULL;
}

/* assumes that data is stored in reverse chronos order in the queue
 * (that is, you used push_head to store the data there)
 */
botlcm_pose_t * find_pose_by_utime (GQueue *data, int64_t utime)
{
    if (!data || g_queue_is_empty (data)) {
        return NULL;
    }

    GList *iter = g_queue_peek_head_link (data);
    while (iter) {
        botlcm_pose_t *p = (botlcm_pose_t*)iter->data;
        if (p->utime <= utime) {
            botlcm_pose_t *q = NULL;
            if (iter->prev) {
                q = (botlcm_pose_t*)(iter->prev->data);
                assert (p->utime <= q->utime);
                if (q->utime - utime < utime - p->utime)
                    return q;
                return p;
            }
        }
        iter = iter->next;
    }

    return NULL;
}

/* assumes that data is stored in reverse chronos order in the queue
 * (that is, you used push_head to store the data there)
 */
nav_applanix_data_t *find_applanix_data (GQueue *data, int64_t utime)
{
    if (!data || g_queue_is_empty (data)) {
        return NULL;
    }

    GList *iter = g_queue_peek_head_link (data);
    while (iter) {
        nav_applanix_data_t *d = (nav_applanix_data_t*)iter->data;
        if (d->utime <= utime) {
            return d;
        }
        iter = iter->next;
    }

    return NULL;
}

botlcm_image_t * find_image_by_utime (GQueue *data, int64_t utime)
{
    if (!data || g_queue_is_empty (data) || utime == 0) {
        return NULL;
    }

    GList *iter = g_queue_peek_head_link (data);
    while (iter) {
        botlcm_image_t *p = (botlcm_image_t*)iter->data;
        if (p->utime <= utime) {
            botlcm_image_t *q = NULL;
            if (iter->prev)
                q = (botlcm_image_t*)(iter->prev->data);
            if (q && q->utime - utime < utime - p->utime)
                return q;
            return p;
        }
        iter = iter->next;
    }

    return NULL;
}

botlcm_image_t *botlcm_image_t_find_by_utime (GQueue *data, int64_t utime)
{
    int64_t best_dt = 0;
    int64_t prev_best_dt = 0;
    botlcm_image_t *best_el = NULL;

    if (g_queue_is_empty (data))
        return NULL;

    // check bounds
    int64_t utime1 = ((botlcm_image_t*)g_queue_peek_head (data))->utime;
    int64_t utime2 = ((botlcm_image_t*)g_queue_peek_tail (data))->utime;
    if (utime2 < utime1) {
        int64_t tmp = utime1;
        utime1 = utime2;
        utime2 = tmp;
    }

    if (utime < utime1 || utime2 < utime)
        return NULL;

    for (GList *iter=g_queue_peek_head_link (data);iter;iter=iter->next) {
        botlcm_image_t *im = (botlcm_image_t*)iter->data;
        int64_t dt = utime < im->utime ? im->utime - utime : utime - im->utime;
        prev_best_dt = best_dt;
        if (!best_el || dt < best_dt) {
            best_dt = dt;
            best_el = im;
        }
        if (prev_best_dt == best_dt) // we stopped making progress
            break;
    }

    return best_el;
}

botlcm_image_t *navlcm_image_old_to_botlcm_image (const navlcm_image_old_t *im)
{
    botlcm_image_t *img = (botlcm_image_t*)calloc(1, sizeof(botlcm_image_t));

    img->utime = im->utime;
    img->width = im->width;
    img->height = im->height;
    img->row_stride = im->stride;
    img->pixelformat = im->pixelformat;
    img->size = im->size;
    img->data = (unsigned char*)malloc(im->size);
    memcpy (img->data, im->image, im->size);
    
    // metadata dictionary
    img->nmetadata = 0;
    img->metadata = NULL;

    return img;
}

