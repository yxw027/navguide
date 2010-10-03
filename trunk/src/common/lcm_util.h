#ifndef _LCM_UTIL_H__
#define _LCM_UTIL_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>
#include <signal.h>
#include <math.h>
#include <poll.h>
#include <fcntl.h>

/* From LCM */
#include <lcm/lcm.h>
#include <lcmtypes/navlcm.h>

/* from common */
#include <common/ioutils.h>
#include <common/mathutil.h>
#include <common/codes.h>
#include <common/applanix.h>
#include <bot/bot_core.h>

#define MAGIC ((int32_t) 0xEDA1DA01L)

// publish
//
void publish_phone_command (lcm_t *lcm, double theta);

void publish_audio_param (lcm_t *lcm, int code, const char *name, double val);
void publish_mission_param (lcm_t *lcm, int code, const char *name);
void publish_mission_param (lcm_t *lcm, int code, const char *name, int64_t utime);
void publish_imu_param (lcm_t *lcm, int code, const char *filename, double rate);
void publish_viewer_param (lcm_t *lcm, int code, const char *filename);
void publish_phone_param_in (lcm_t *lcm, int code, const char *name);
void publish_phone_param_out (lcm_t *lcm, int code, const char *name);
void publish_features_param (lcm_t *lcm, int code);
void publish_log_info_t (lcm_t *lcm, const char *channel, int code, char *name, char *fullname, char *desc);

void publish_nav_order (lcm_t *lcm, double angle, double progress, 
                        int node, int code, double error);
void publish_phone_msg (lcm_t *lcm, const char *string, ...) ;
void publish_grabber_param (lcm_t *lcm, int code,
                            int frame_rate, double scale_factor, 
                            int packet_size);

void publish_class_param (lcm_t *lcm, int code, const char *text);
void publish_logplayer_param (lcm_t *lcm, int code, const char *filename, double speed);
void free_feature_list (navlcm_feature_list_t *f);

// create
//
navlcm_feature_t *navlcm_feature_t_create ();
navlcm_feature_t* navlcm_feature_t_create (int sensorid, double col, double row,
                                             int index, double scale);
void navlcm_feature_t_print (navlcm_feature_t* ft);
navlcm_feature_t *navlcm_feature_t_find_by_index (navlcm_feature_list_t *features, int index);
int navlcm_feature_t_equal (navlcm_feature_t *f1, navlcm_feature_t *f2);

navlcm_track_t*
navlcm_track_t_create (navlcm_feature_t *key, int width, int height,
                         int ttl, int64_t uid);
navlcm_track_t*
navlcm_track_t_insert_head (navlcm_track_t *track, navlcm_feature_t *ft);
navlcm_track_t*
navlcm_track_t_append (navlcm_track_t *track, navlcm_feature_t *ft);

navlcm_track_t*
navlcm_track_t_concat (navlcm_track_t *src, navlcm_track_t *sup);

void botlcm_pose_t_clear (botlcm_pose_t *p);
navlcm_gps_to_local_t *navlcm_gps_to_local_t_new (int64_t utime);
void navlcm_gps_to_local_t_clear (navlcm_gps_to_local_t *g);
void navlcm_feature_list_t_clear (navlcm_feature_list_t *ft);
navlcm_feature_list_t* navlcm_feature_list_t_create ();
navlcm_feature_list_t* navlcm_feature_list_t_create 
                (int width, int height, int sensorid, int64_t timestamp, int dist_mode);
gboolean navlcm_feature_list_t_search (navlcm_feature_list_t *fs, navlcm_feature_t *f);
gboolean navlcm_feature_match_set_t_search_copy (navlcm_feature_match_set_t *ms, navlcm_feature_match_t *m);
gboolean navlcm_feature_match_set_t_remove (navlcm_feature_match_set_t *ms, navlcm_feature_match_t *m);
gboolean navlcm_feature_match_set_t_remove_copy (navlcm_feature_match_set_t *ms, navlcm_feature_match_t *m);
gboolean navlcm_feature_list_t_remove (navlcm_feature_list_t *fs, navlcm_feature_t *f);
void navlcm_feature_list_t_insert_head 
(navlcm_feature_list_t* set, navlcm_feature_t *ft);
navlcm_feature_list_t* navlcm_feature_list_t_insert_tail
(navlcm_feature_list_t* set, navlcm_feature_t *ft);
navlcm_feature_list_t* navlcm_feature_list_t_insert_tail
(navlcm_feature_list_t* set, navlcm_feature_t *ft);
navlcm_feature_list_t* navlcm_feature_list_t_append 
(navlcm_feature_list_t* set, navlcm_feature_t *ft);
navlcm_feature_list_t navlcm_feature_list_t_create (int n);
navlcm_feature_match_set_t*
navlcm_feature_match_set_t_create ();

navlcm_track_t*
navlcm_track_t_erase (navlcm_track_t* t);

navlcm_track_set_t* 
navlcm_track_set_t_append (navlcm_track_set_t *set, navlcm_track_t *tr);
navlcm_feature_list_t* 
navlcm_feature_list_t_append (navlcm_feature_list_t *set1, 
                                navlcm_feature_list_t *set2);
navlcm_feature_match_set_t *navlcm_feature_match_set_t_append 
(navlcm_feature_match_set_t*set1,
 navlcm_feature_match_set_t *set2);
navlcm_feature_match_t* navlcm_feature_match_t_create 
(navlcm_feature_t *key);
navlcm_feature_match_t* 
navlcm_feature_match_t_insert (navlcm_feature_match_t* match,
                                 navlcm_feature_t *key, double dist);
void navlcm_feature_match_t_print (navlcm_feature_match_t *m);
void navlcm_feature_match_set_t_print (navlcm_feature_match_set_t *ms);


void navlcm_feature_match_set_t_insert (
                navlcm_feature_match_set_t *set, 
                navlcm_feature_match_t *match);

// read, write, init
//
int navlcm_track_t_write (const navlcm_track_t *track, FILE *fp);
int navlcm_feature_t_write (navlcm_feature_t *ft, FILE *fp);
int navlcm_feature_list_t_write (const navlcm_feature_list_t *data, FILE *fp);
int navlcm_gps_to_local_t_write (const navlcm_gps_to_local_t *data, FILE *fp);
int botlcm_pose_t_write (const botlcm_pose_t *data, FILE *fp);

int navlcm_track_t_read (navlcm_track_t *track, FILE *fp);
int navlcm_feature_t_read (navlcm_feature_t *ft, FILE *fp);
int navlcm_feature_list_t_read (navlcm_feature_list_t *data, FILE *fp);
int navlcm_gps_to_local_t_read (navlcm_gps_to_local_t *data, FILE *fp);
int botlcm_pose_t_read (botlcm_pose_t *data, FILE *fp);

int navlcm_feature_match_set_t_read (navlcm_feature_match_set_t *data, FILE *fp);
int navlcm_feature_match_set_t_write (const navlcm_feature_match_set_t *data, FILE *fp);

int botlcm_image_t_read (botlcm_image_t *data, FILE *fp);
int botlcm_image_t_write (const botlcm_image_t *data, FILE *fp);

int botlcm_image_t_print (const botlcm_image_t *data);

// random
navlcm_feature_t navlcm_feature_t_random (int width, int height, int size, int sensorid);

botlcm_image_t *botlcm_image_t_create (int width, int height, int stride, int size, 
       const unsigned char *data, int pixelformat, int64_t utime);
void botlcm_image_t_clear (botlcm_image_t *img);

// navlcm_gate_t
GQueue* navlcm_gates_read_from_file (FILE *fp);
int navlcm_gates_write_to_file (GQueue *gs, FILE *fp);
void navlcm_gates_destroy (GQueue *gs);
gint navlcm_gate_comp (gconstpointer a, gconstpointer b);

navlcm_feature_list_t * find_feature_list_by_utime (GQueue *data, int64_t utime);
navlcm_imu_t * find_imu_by_utime (GQueue *data, int64_t utime);
botlcm_pose_t * find_pose_by_utime (GQueue *data, int64_t utime);
nav_applanix_data_t *find_applanix_data (GQueue *data, int64_t utime);
botlcm_image_t * find_image_by_utime (GQueue *data, int64_t utime);
navlcm_system_info_t *navlcm_system_info_t_create ();
void navlcm_system_info_t_print (navlcm_system_info_t *s);
botlcm_image_t *botlcm_image_t_find_by_utime (GQueue *data, int64_t utime);

botlcm_image_t *navlcm_image_old_to_botlcm_image (const navlcm_image_old_t *im);

#endif

