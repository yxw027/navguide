#ifndef _EVALUATION_DRAW_H__
#define _EVALUATION_DRAW_H__

#include "main.h"
#include <camunits/pixels.h>
#include <common/gltool.h>

#define NAME_POINT1 0
#define NAME_POINT2 1
#define NAME_MATCH 2

gboolean check_gl_error (const char *t);
void start_picking (int cursorX, int cursorY, int w, int h);
int  stop_picking (GLuint *type, GLuint *name);

void draw_frame (int ww, int wh, botlcm_image_t **img1, botlcm_image_t **img2, int nimg, navlcm_feature_list_t *features1, navlcm_feature_list_t *features2, navlcm_feature_match_set_t *matches, double user_scale, double *user_trans, gboolean feature_finder_enabled, gboolean select, navlcm_feature_t *highlight_f, navlcm_feature_match_t *highlight_m, navlcm_feature_t *match_f1, navlcm_feature_t *match_f2, navlcm_feature_match_t *select_m, navlcm_feature_match_t *match, int feature_type, double precision, double recall);


#endif

