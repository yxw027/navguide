#ifndef _viewer_util_h__
#define _viewer_util_h__

#include <stdio.h>
#include <stdlib.h>
#include <bot/bot_core.h>
#include <bot/viewer/viewer.h>
#include <bot/gl/gl_util.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <common/texture.h>
#include <common/mathutil.h>

#include <lcmtypes/navlcm_feature_list_t.h>
#include <lcmtypes/navlcm_flow_t.h>

#include <common/colors.h>

double util_image_ratio (int width, int height, int img_width, int img_height, gboolean compact, gboolean fit_to_window, int nsensors, int border_width);
void util_image_to_screen (double col, double row, int sensorid, int c0, int r0, int img_width, int img_height, double ratio, gboolean compact_layout, double *ic0, double *ir0);
void util_draw_image (botlcm_image_t *img, int idx, int window_width, int window_height, gboolean compact,
        gboolean fit_to_window, int desired_width, int nsensors, int c0, int r0, int border_width);
void util_draw_flow (navlcm_flow_t *flow, int window_width, int window_height, int image_width, int image_height, double flow_scale,
        gboolean compact, gboolean fit_to_window, int desired_width, int nsensors, int c0, int r0, int border_width, int line_width);
void util_draw_features (navlcm_feature_list_t *s, int window_width, int window_height, gboolean compact,
        gboolean fit_to_window, int desired_width, int nsensors, int c0, int r0, int border_width, int pointsize, gboolean sensor_color, gboolean sensor_color_box, gboolean show_scale);
void util_draw_circle (double x, double y, double r);
void util_draw_disk (double x, double y, double r_in, double r_out);
GLUtilTexture *util_load_texture_from_file (char *filename);

#endif

