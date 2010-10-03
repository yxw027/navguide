#ifndef _SIMCAM_H__
#define _SIMCAM_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <common/texture.h>
#include <common/dbg.h>

#include <image/image.h>
#include <jpegcodec/pngload.h>

/* From the GL library */
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#define SIM_WIDTH 376
#define SIM_HEIGHT 240
#define SIM_FOV 90.0
#define SIM_ZNEAR 1.0
#define SIM_ZFAR 10000.0
#define SIM_TEX_ON 1

#define N_SIM_IMG_NAMES 32
static const char* SIM_IMG_NAMES[N_SIM_IMG_NAMES] = {
    "0.png", "1.png", "2.png", "3.png", "4.png", "5.png", "6.png", "7.png", "8.png", "9.png", 
    "10.png","11.png", "12.png", "13.png", "14.png", "15.png", "16.png", "17.png", "18.png", "19.png", 
    "20.png","21.png", "22.png", "23.png", "24.png", "25.png", "26.png", "27.png", "28.png", "29.png", 
    "30.png", "31.png"
};

typedef struct model_t { double **quad; GLUtilTexture **tex; int nel;};

void setup_3d ( double *eye, double *unit, double *up );
model_t sim_read_model (const char *filename);
void sim_render (model_t model, int tex_on, int wireframe);
GLfloat *sim_read_pixels (model_t model, double *eye, double *unit, double *up);
void sim_assign_and_save (model_t *model, const char *filename, const char **names, int nnames);
void sim_model_add_cube (model_t *model, double x, double y, double z, double w);
void sim_generate_model (model_t *model);
void sim_generate_model2 (model_t *model);

#endif
