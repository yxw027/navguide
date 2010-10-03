#include <stdio.h>
#include <stdlib.h>

#include <bot/bot_core.h>

#include "mathutil.h"
#include "navconf.h"

#define CONF_FILENAME CONFIG_DIR "/navguide.cfg"

BotConf *
navconf_parse_default (void)
{
    BotConf * c;
    FILE * f = fopen (CONF_FILENAME, "r");
    if (!f) {
        fprintf (stderr, "Error: failed to open config file:\n%s\n",
                CONF_FILENAME);
        return NULL;
    }
    c = bot_conf_parse_file (f, CONF_FILENAME);
    fclose (f);
    return c;
}

int
navconf_get_vehicle_footprint (BotConf *cfg, double fp[8])
{
    const char *key_fl = "calibration.vehicle_bounds.front_left";
    const char *key_fr = "calibration.vehicle_bounds.front_right";
    const char *key_bl = "calibration.vehicle_bounds.rear_left";
    const char *key_br = "calibration.vehicle_bounds.rear_right";

    if ((bot_conf_get_array_len (cfg, key_fl) != 2) ||
        (bot_conf_get_array_len (cfg, key_fr) != 2) ||
        (bot_conf_get_array_len (cfg, key_bl) != 2) ||
        (bot_conf_get_array_len (cfg, key_br) != 2)) {
        fprintf (stderr, "ERROR! invalid calibration.vehicle_bounds!\n");
        return -1;
    }

    bot_conf_get_double_array (cfg, key_fl, fp, 2);
    bot_conf_get_double_array (cfg, key_fr, fp+2, 2);
    bot_conf_get_double_array (cfg, key_br, fp+4, 2);
    bot_conf_get_double_array (cfg, key_bl, fp+6, 2);
    return 0;
}

//int 
//arconf_get_rndf_absolute_path (BotConf * config, char *buf, int buf_size)
//{
//    char rndf_path[256];
//    char *rndf_file;
//    if (bot_conf_get_str (config, "rndf", &rndf_file) < 0)
//        return -1;
//    snprintf (rndf_path, sizeof (rndf_path), CONFIG_DIR"/%s", rndf_file);
//
//    if (strlen (rndf_path) > buf_size)
//        return -1;
//    strcpy (buf, rndf_path);
//    return 0;
//}
//
static int
get_filename (BotConf *cfg, char *buf, int buf_size, const char *key, 
        const char *prefix)
{
    char *fname;
    char full_fname[4096];
    if (bot_conf_get_str (cfg, key, &fname) < 0)
        return -1;
    snprintf (full_fname, sizeof (full_fname)-1, "%s%s", prefix, fname);
    if (strlen (full_fname) >= (unsigned int)buf_size) return -1;
    strcpy (buf, full_fname);
    return 0;
}

int 
navconf_get_gis_shapefile_name (BotConf *cfg, char *buf, int buf_size)
{
    return get_filename(cfg, buf, buf_size, "gis.shapefile", CONFIG_DIR"/../");
}

int 
navconf_get_lanetruth_filename(BotConf *cfg, char *buf, int buf_size)
{
    return get_filename(cfg, buf, buf_size, "lanetruth.filename", 
            CONFIG_DIR"/../");
}

int 
navconf_get_lanetruth_corrections_filename(BotConf *cfg, char *buf, int buf_size)
{
    return get_filename(cfg, buf, buf_size, "lanetruth.corrections", 
            CONFIG_DIR"/../");
}

int navconf_get_gis_roi (BotConf *cfg, double *lat_min, double *lon_min, 
        double *lat_max, double *lon_max)
{
    double d[4];
    if (bot_conf_get_double_array (cfg, "gis.roi", d, 4) < 0) return -1;
    if (d[0] > d[2]) {
        *lat_min = d[2]; *lat_max = d[0];
    } else {
        *lat_min = d[0]; *lat_max = d[2];
    }

    if (d[1] > d[3]) {
        *lon_min = d[3]; *lon_max = d[1];
    } else {
        *lon_min = d[1]; *lon_max = d[3];
    }
    return 0;
}


//int 
//navconf_get_gis_ortho_imagery_dir (BotConf *cfg, char *buf, int buf_size)
//{
//    char *fname;
//    char full_fname[4096];
//    if (bot_conf_get_str (cfg, "gis.ortho_imagery_dir", &fname) < 0)
//        return -1;
//    snprintf (full_fname, sizeof (full_fname)-1, CONFIG_DIR"/../%s", fname);
//
//    if (strlen (full_fname) >= buf_size) return -1;
//    strcpy (buf, full_fname);
//    return 0;
//}

int 
navconf_get_quat (BotConf *cfg, const char *name, double quat[4])
{
    assert (cfg);

    char key[256];
    sprintf(key, "calibration.%s.orientation", name);
    if (bot_conf_has_key(cfg, key)) {
        int sz = bot_conf_get_double_array(cfg, key, quat, 4);
        assert(sz==4);
        return 0;
    }

    sprintf(key, "calibration.%s.rpy", name);
    if (bot_conf_has_key(cfg, key)) {
        double rpy[3];
        int sz = bot_conf_get_double_array(cfg, key, rpy, 3);
        assert(sz == 3);
        for (int i = 0; i < 3; i++)
            rpy[i] = to_radians(rpy[i]);
        bot_roll_pitch_yaw_to_quat(rpy, quat);
        return 0;
    }

    sprintf(key, "calibration.%s.angleaxis", name);
    if (bot_conf_has_key(cfg, key)) {
        double aa[4];
        int sz = bot_conf_get_double_array(cfg, key, aa, 4);
        assert(sz==4);

        double theta = aa[0];
        double s = sin(theta/2);

        quat[0] = cos(theta/2);
        for (int i = 1; i < 4; i++)
            quat[i] = aa[i] * s;
        return 0;
    }
    return -1;
}

static int 
navconf_get_pos(BotConf *cfg, const char *name, double pos[3])
{
    char key[256];
    sprintf(key, "calibration.%s.position", name);
    if (bot_conf_has_key(cfg, key)) {
        int sz = bot_conf_get_double_array(cfg, key, pos, 3);
        assert(sz==3);
        return 0;
    } 
    return -1;
}

int 
navconf_get_matrix(BotConf *cfg, const char *name, double m[16])
{
    double quat[4];
    double pos[3];
    if (navconf_get_quat(cfg, name, quat))
        return -1;
    if (navconf_get_pos(cfg, name, pos))
        return -1;
    bot_quat_pos_to_matrix(quat, pos, m);
    return 0;
}

int 
navconf_sensor_to_local_with_pose(BotConf *cfg, const char *name, double m[16], 
        const botlcm_pose_t *p)
{
    double body_to_local[16];

    bot_quat_pos_to_matrix(p->orientation, p->pos, body_to_local);

    double sensor_to_calibration[16];

    if (navconf_get_matrix(cfg, name, sensor_to_calibration))
        return -1;

    char key[128];
    sprintf(key,"calibration.%s.relative_to", name);

    char *calib_frame = bot_conf_get_str_or_fail(cfg, key);
    if (!strcmp(calib_frame, "body")) {

        bot_matrix_multiply_4x4_4x4(body_to_local, sensor_to_calibration, m);

    } else {
        double calibration_to_body[16];
        
        if (navconf_get_matrix(cfg, calib_frame, calibration_to_body))
            return -1;

        double sensor_to_body[16];
        bot_matrix_multiply_4x4_4x4(calibration_to_body, sensor_to_calibration, 
                                sensor_to_body);
        bot_matrix_multiply_4x4_4x4(body_to_local, sensor_to_body, m);
    }

    return 0;
}

static int
get_str (BotConf *cfg, const char *key, char *result, int result_size)
{
    char *val = NULL;
    int status = bot_conf_get_str (cfg, key, &val);
    if (0 == status && (unsigned int)result_size > strlen (val)) {
        strcpy (result, val);
        status = 0;
    } else {
        status = -1;
    }
    return status;
}

int 
navconf_get_camera_thumbnail_channel (BotConf *cfg,
        const char *camera_name, char *result, int result_size)
{
    char key[1024];
    snprintf (key, sizeof (key), "cameras.%s.thumbnail_channel", camera_name);
    return get_str (cfg, key, result, result_size);
}

int navconf_get_nsensors (BotConf *cfg, int *result)
{
    char *platform;
    bot_conf_get_str (cfg, "platform", &platform);

    char key[1024];
    sprintf (key, "cameras.%s.nsensors", platform);

    if (!bot_conf_get_int (cfg, key, result))
        return -1;
    return 0;
}

int
navconf_get_channel_index (char **channels, const char *channel, int *result)
{
    int idx=0;
    while (channels[idx]) {
        if (strcmp (channel, channels[idx]) == 0) {
            *result = idx;
            return 0;
        }
        idx++;
    }

    return -1;
}

/* ================ cameras ============== */
static int _navconf_get_quat (BotConf *config, const char *name, double quat[4]);

char**
navconf_get_all_camera_names (BotConf *config)
{
    return bot_conf_get_subkeys (config, "cameras");
}

int 
navconf_get_camera_calibration_config_prefix (BotConf *config, 
                            const char *cam_name, char *result, int result_size)
{
    snprintf (result, result_size, "calibration.cameras.%s", cam_name);
    return bot_conf_has_key (config, result) ? 0 : -1;
}

CamTrans* 
navconf_get_new_camtrans (BotConf *config, const char *cam_name)
{
    char prefix[1024];
    int status = navconf_get_camera_calibration_config_prefix (config, cam_name, 
                                                       prefix, sizeof (prefix));
    if (0 != status) return NULL;

    char key[2048];
    double width;
    sprintf(key, "%s.width", prefix);
    if (0 != bot_conf_get_double(config, key, &width)) return NULL;

    double height;
    sprintf(key, "%s.height", prefix);
    if (0 != bot_conf_get_double(config, key, &height)) return NULL;

    double pinhole_params[5];
    snprintf(key, sizeof (key), "%s.pinhole", prefix);
    if (5 != bot_conf_get_double_array(config, key, pinhole_params, 5))
        return NULL;
    double fx = pinhole_params[0];
    double fy = pinhole_params[1];
    double cx = pinhole_params[3];
    double cy = pinhole_params[4];
    double skew = pinhole_params[2];

    double position[3];
    sprintf(key, "%s.position", prefix);
    if (3 != bot_conf_get_double_array(config, key, position, 3)) return NULL;

    sprintf (key, "cameras.%s", cam_name);
    double orientation[4];
    if (0 != _navconf_get_quat (config, key, orientation)) return NULL;

    double dist_center[2];
    sprintf(key, "%s.distortion_center", prefix);
    if (2 != bot_conf_get_double_array(config, key, dist_center, 2)) return NULL;

    double distortion_param;
    sprintf(key, "%s.distortion_params", prefix);
    if (1 != bot_conf_get_double_array(config, key, &distortion_param, 1))
        return NULL;

    return camtrans_new_spherical (cam_name, width, height, fx, fy, cx, cy, skew,
                                   dist_center[0], dist_center[1],
                                   distortion_param);
}

/* ================ planar lidar ============== */
char** 
navconf_get_all_planar_lidar_names (BotConf *config)
{
    return bot_conf_get_subkeys (config, "planar_lidars");
}

static int 
_navconf_get_quat (BotConf *config, const char *name, double quat[4])
{
    assert (config);

    char key[256];
    sprintf(key, "calibration.%s.orientation", name);
    if (bot_conf_has_key(config, key)) {
        int sz = bot_conf_get_double_array(config, key, quat, 4);
        assert(sz==4);
        return 0;
    }

    sprintf(key, "calibration.%s.rpy", name);
    if (bot_conf_has_key(config, key)) {
        double rpy[3];
        int sz = bot_conf_get_double_array(config, key, rpy, 3);
        assert(sz == 3);
        for (int i = 0; i < 3; i++)
            rpy[i] = to_radians(rpy[i]);
        bot_roll_pitch_yaw_to_quat(rpy, quat);
        return 0;
    }

    sprintf(key, "calibration.%s.angleaxis", name);
    if (bot_conf_has_key(config, key)) {
        double aa[4];
        int sz = bot_conf_get_double_array(config, key, aa, 4);
        assert(sz==4);

        double theta = aa[0];
        double s = sin(theta/2);

        quat[0] = cos(theta/2);
        for (int i = 1; i < 4; i++)
            quat[i] = aa[i] * s;
        return 0;
    }
    return -1;
}

int
navconf_cam_get_name_from_uid (BotConf *cfg, int64_t uid, char *result, 
        int result_size)
{
    int status = -1;

    memset (result, 0, result_size);
    char **cam_names = bot_conf_get_subkeys (cfg, "cameras");

    for (int i=0; cam_names && cam_names[i]; i++) {
        char *uid_key = 
            g_strdup_printf ("cameras.%s.uid", cam_names[i]);
        char *cam_uid_str = NULL;
        int key_status = bot_conf_get_str (cfg, uid_key, &cam_uid_str);
        free (uid_key);

        if (0 == key_status) {
            int64_t cam_uid = strtoll (cam_uid_str, NULL, 16);
            if (cam_uid == uid) {
                if ((unsigned int)result_size > strlen (cam_names[i])) {
                    strcpy (result, cam_names[i]);
                    status = 0;
                }
                break;
            }
        }
    }
    g_strfreev (cam_names);

    return status;
}

int
navconf_cam_get_name_from_lcm_channel (BotConf *cfg, const char *channel, 
        char *result, int result_size)
{
    int status = -1;

    memset (result, 0, result_size);
    char **cam_names = bot_conf_get_subkeys (cfg, "cameras");

    for (int i=0; cam_names && cam_names[i]; i++) {
        char *thumb_key = 
            g_strdup_printf ("cameras.%s.thumbnail_channel", cam_names[i]);
        char *cam_thumb_str = NULL;
        int key_status = bot_conf_get_str (cfg, thumb_key, &cam_thumb_str);
        free (thumb_key);

        if ((0 == key_status) && (0 == strcmp (channel, cam_thumb_str))) {
            strcpy (result, cam_names[i]);
            status = 0;
            break;
        }

        char *full_key = 
            g_strdup_printf ("cameras.%s.full_frame_channel", cam_names[i]);
        char *cam_full_str = NULL;
        key_status = bot_conf_get_str (cfg, full_key, &cam_full_str);
        free (full_key);

        if ((0 == key_status) && (0 == strcmp (channel, cam_full_str))) {
            strcpy (result, cam_names[i]);
            status = 0;
            break;
        }
    }
    g_strfreev (cam_names);

    return status;
}
