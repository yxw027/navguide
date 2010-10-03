#ifndef __ARCONF_H__
#define __ARCONF_H__

#include <bot/bot_core.h>
#include <camunits/cam.h>
#include "camtrans.h"

#ifdef __cplusplus
extern "C" {
#endif

BotConf * navconf_parse_default (void);

int navconf_get_vehicle_footprint (BotConf *cfg, double fp[8]);

int navconf_get_matrix(BotConf *cfg, const char *name, double m[16]);

int navconf_sensor_to_local_with_pose(BotConf *cfg, const char *name, 
        double m[16], const botlcm_pose_t *p);

int navconf_get_quat (BotConf * config, const char *name, double quat[4]);

int navconf_get_nsensors (BotConf *cfg, int *result);
int navconf_get_channel_index (char **channels, const char *channel, int *result);

int navconf_get_camera_thumbnail_channel (BotConf *cfg,
        const char *camera_name, char *result, int result_size);

char** navconf_get_all_camera_names (BotConf *config);
int navconf_get_camera_calibration_config_prefix (BotConf *config, 
                           const char *cam_name, char *result, int result_size);
CamTrans* navconf_get_new_camtrans (BotConf *config, const char *cam_name);

char** navconf_get_all_planar_lidar_names (BotConf *config);

int navconf_cam_get_name_from_uid (BotConf *cfg, int64_t uid, char *result, 
        int result_size);

int navconf_cam_get_name_from_lcm_channel (BotConf *cfg, const char *channel, 
        char *result, int result_size);

int 
navconf_get_camera_thumbnail_channel (BotConf *cfg,
        const char *camera_name, char *result, int result_size);
#ifdef __cplusplus
}
#endif

#endif
