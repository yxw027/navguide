#include <stdio.h>

#include <glib.h>

#include <lcm/lcm.h>
#include <bot/lcmtypes/lcmtypes.h>
#include <lcmtypes/navlcm.h>

#include "atrans.h"
#include "navconf.h"

struct _ATrans 
{
    BotCTrans * ctrans;
    lcm_t *lcm;
    BotConf *config;

    GMutex * mutex;

    botlcm_pose_t_subscription_t * pose_subscription;
    botlcm_pose_t last_pose;
    int have_last_pose;

    BotCTransLink *body_to_local_link;
};

static void
on_pose(const lcm_recv_buf_t *rbuf, const char *channel, 
        const botlcm_pose_t *pose, void *user_data);

static void
setup_sensor_trans(ATrans *atrans, BotConf *config, 
        const char *sensor_name, const char *calib_key)
{
    char key[2048];

    BotTrans rbtrans;
    sprintf(key, "calibration.%s.position", calib_key);
    if (3 != bot_conf_get_double_array(config, key, rbtrans.trans_vec, 3)) {
        fprintf(stderr, "%s:  error setting up %s (%s)\n"
                "Couldn't get position\n", __FILE__, 
                sensor_name, calib_key);
        return;
    }

    if (0 != navconf_get_quat (config, calib_key, rbtrans.rot_quat)) {
        fprintf(stderr, "%s:  error setting up %s (%s)\n"
                "Couldn't get orientation\n", __FILE__, 
                sensor_name, calib_key);
        return;
    }

    bot_ctrans_add_frame(atrans->ctrans, sensor_name);

    char *ref_frame;
    sprintf(key, "calibration.%s.relative_to", calib_key);
    if (0 != bot_conf_get_str(config, key, &ref_frame)) {
        fprintf(stderr, "%s:  error setting up %s (%s)\n"
                "Couldn't get reference frame\n", __FILE__, 
                sensor_name, calib_key);
        return;
    }

    BotCTransLink * link = bot_ctrans_link_frames(atrans->ctrans, 
            sensor_name, ref_frame, 1);
    if(link) {
        bot_ctrans_link_update(link, &rbtrans, 0);
    } else {
        fprintf(stderr, "%s:  error setting up %s (%s)\n"
                "Couldn't link with reference frame %s\n", __FILE__, 
                sensor_name, calib_key, ref_frame);
    }



    return;
}

ATrans * 
atrans_new(lcm_t *lcm, BotConf *config)
{
    ATrans *atrans = g_slice_new0(ATrans);

    atrans->lcm = lcm;
    atrans->config = config;
    atrans->mutex = g_mutex_new();

    // setup the coordinate frame graph
    atrans->ctrans = bot_ctrans_new();

    memset(&atrans->last_pose, 0, sizeof(botlcm_pose_t));
    atrans->have_last_pose = 0;

    // setup the body to local transformation
    bot_ctrans_add_frame(atrans->ctrans, "body");
    bot_ctrans_add_frame(atrans->ctrans, "local");
    atrans->body_to_local_link = bot_ctrans_link_frames(atrans->ctrans, 
            "body", "local", 1000);

    // setup the camera coordinate frames (extrinsics)
    char **camnames = navconf_get_all_camera_names(config);
    if (camnames)
    {
        for(int cind=0; camnames[cind]; cind++) {
            const char *cam_name = camnames[cind];
            char calib_key[1024];
            sprintf(calib_key, "cameras.%s", cam_name);
            setup_sensor_trans(atrans, config, cam_name, calib_key);
        }
        g_strfreev(camnames);
    }
    else
    {
        fprintf(stderr, "["__FILE__":%d] Error: Could not"
                " get camera names.\n", __LINE__);
    }

    // setup the planar lidar (SICK, hokuyo) coordinate frames
    char **planar_lidar_names = navconf_get_all_planar_lidar_names(config);
    if (planar_lidar_names)
    {
        for(int pind=0; planar_lidar_names[pind]; pind++) {
            char calib_key[1024];
            sprintf(calib_key, "planar_lidars.%s", planar_lidar_names[pind]);
            setup_sensor_trans(atrans, config, planar_lidar_names[pind], calib_key);
        }
        g_strfreev(planar_lidar_names);
    }
    else
    {
        fprintf(stderr, "["__FILE__":%d] Error: Could not"
                " get lidar names.\n", __LINE__);
    }

    if(lcm) {
        atrans->pose_subscription = botlcm_pose_t_subscribe(lcm,
                "POSE", on_pose, atrans);
    }

    return atrans;
}

void 
atrans_destroy(ATrans * atrans)
{
    if(atrans->pose_subscription)
        botlcm_pose_t_unsubscribe(atrans->lcm, atrans->pose_subscription);

    g_mutex_free(atrans->mutex);

    bot_ctrans_destroy(atrans->ctrans);
    g_slice_free(ATrans, atrans);
}

int 
atrans_get_trans_with_utime(ATrans *atrans, const char *from_frame,
        const char *to_frame, int64_t utime, BotTrans *result)
{
    g_mutex_lock(atrans->mutex);
    int status = bot_ctrans_get_trans(atrans->ctrans, from_frame, 
            to_frame, utime, result);
    g_mutex_unlock(atrans->mutex);
    return status;
}

int 
atrans_get_trans(ATrans *atrans, const char *from_frame,
        const char *to_frame, BotTrans *result)
{
    g_mutex_lock(atrans->mutex);
    int status = bot_ctrans_get_trans_latest(atrans->ctrans, from_frame, 
            to_frame, result);
    g_mutex_unlock(atrans->mutex);
    return status;
}

int 
atrans_get_trans_mat_3x4(ATrans *atrans, const char *from_frame,
        const char *to_frame, double mat[12])
{
    BotTrans bt;
    if(!atrans_get_trans(atrans, from_frame, to_frame, &bt))
        return 0;
    bot_trans_get_mat_3x4(&bt, mat);
    return 1;
}

int 
atrans_get_trans_mat_3x4_with_utime(ATrans *atrans, 
        const char *from_frame, const char *to_frame, int64_t utime, 
        double mat[12])
{
    BotTrans bt;
    if(!atrans_get_trans_with_utime(atrans, from_frame, to_frame, utime, &bt))
        return 0;
    bot_trans_get_mat_3x4(&bt, mat);
    return 1;
}

int
atrans_get_trans_mat_4x4(ATrans *atrans, const char *from_frame,
        const char *to_frame, double mat[16])
{
    BotTrans bt;
    if(!atrans_get_trans(atrans, from_frame, to_frame, &bt))
        return 0;
    bot_trans_get_mat_4x4(&bt, mat);
    return 1;
}

int 
atrans_get_trans_mat_4x4_with_utime(ATrans *atrans, 
        const char *from_frame, const char *to_frame, int64_t utime, 
        double mat[16])
{
    BotTrans bt;
    if(!atrans_get_trans_with_utime(atrans, from_frame, to_frame, utime, &bt))
        return 0;
    bot_trans_get_mat_4x4(&bt, mat);
    return 1;
}

int 
atrans_have_trans(ATrans *atrans, const char *from_frame, const char *to_frame)
{
    g_mutex_lock(atrans->mutex);
    int status = bot_ctrans_have_trans(atrans->ctrans, from_frame, to_frame);
    g_mutex_unlock(atrans->mutex);
    return status;
}

int 
atrans_transform_vec(ATrans *atrans, const char *from_frame,
        const char *to_frame, const double src[3], double dst[3])
{
    BotTrans rbtrans;
    if(!atrans_get_trans(atrans, from_frame, to_frame, &rbtrans))
        return 0;
    bot_trans_apply_vec(&rbtrans, src, dst);
    return 1;
}

int 
atrans_rotate_vec(ATrans *atrans, const char *from_frame,
        const char *to_frame, const double src[3], double dst[3])
{
    BotTrans rbtrans;
    if(!atrans_get_trans(atrans, from_frame, to_frame, &rbtrans))
        return 0;
    bot_trans_rotate_vec(&rbtrans, src, dst);
    return 1;
}

int 
atrans_get_n_trans(ATrans *atrans, const char *from_frame,
        const char *to_frame, int nth_from_latest)
{
    BotCTransLink *link = bot_ctrans_get_link(atrans->ctrans, from_frame, 
            to_frame);
    if(!link)
        return 0;
    return bot_ctrans_link_get_n_trans(link);
}

/**
 * Returns: 1 on success, 0 on failure
 */
int 
atrans_get_nth_trans(ATrans *atrans, const char *from_frame,
        const char *to_frame, int nth_from_latest, 
        BotTrans *btrans, int64_t *timestamp)
{
    BotCTransLink *link = bot_ctrans_get_link(atrans->ctrans, from_frame, 
            to_frame);
    if(!link)
        return 0;
    int status = bot_ctrans_link_get_nth_trans(link, nth_from_latest, btrans, 
            timestamp);
    if(status && btrans && 
       0 != strcmp(to_frame, bot_ctrans_link_get_to_frame(link))) {
        bot_trans_invert(btrans);
    }
    return status;
}

// convenience function to get the vehicle position in the local frame.
int 
atrans_vehicle_pos_local(ATrans *atrans, double pos[3])
{
    double pos_body[3] = { 0, 0, 0 };
    return atrans_transform_vec(atrans, "body", "local", pos_body, pos);
}

static void
on_pose(const lcm_recv_buf_t *rbuf, const char *channel, 
        const botlcm_pose_t *pose, void *user_data)
{
    ATrans * atrans = (ATrans*)user_data;
    g_mutex_lock(atrans->mutex);
    BotTrans body_to_local;
    memcpy(body_to_local.trans_vec, pose->pos, 3*sizeof(double));
    memcpy(body_to_local.rot_quat, pose->orientation, 4*sizeof(double));
    bot_ctrans_link_update(atrans->body_to_local_link, &body_to_local, 
            pose->utime);
//    if(pose->utime < atrans->last_pose.utime)
//        atrans->have_gps_to_local = 0;
    memcpy(&atrans->last_pose, pose, sizeof(botlcm_pose_t));
    atrans->have_last_pose = 1;
    g_mutex_unlock(atrans->mutex);
}

int 
atrans_get_local_pose(ATrans *atrans, botlcm_pose_t *pose)
{
    if(!atrans->have_last_pose)
        return 0;
    memcpy(pose, &atrans->last_pose, sizeof(botlcm_pose_t));
    return 1;
}

int
atrans_get_local_pos(ATrans *atrans, double pos[3])
{
    BotTrans bt;
    if (!atrans_get_trans(atrans, "body", "local", &bt))
        return 0;

    // Get position.
    memcpy(pos, bt.trans_vec, 3 * sizeof(double));
    return 1;
}

int
atrans_get_local_pos_heading(ATrans *atrans, double pos[3], double *heading)
{
    BotTrans bt;
    if (!atrans_get_trans(atrans, "body", "local", &bt))
        return 0;

    // Get position.
    memcpy(pos, bt.trans_vec, 3 * sizeof(double));

    // Get heading.
    double rpy[3];
    bot_quat_to_roll_pitch_yaw(bt.rot_quat, rpy);
    *heading = rpy[2];

    return 1;
}
