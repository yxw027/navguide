#ifndef _GLTOOL_H__
#define _GLTOOL_H__

/* From the standard C library */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>

/* From the GL library */
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include "colors.h"
//#include "math/quat.h"
#include "mathutil.h"

/* From common */
#include <common/dbg.h>

#define SELECT_BUFSIZE 512

void glColor( Color c );
void DrawAxis( int size );
void gl_clear_window( Color color );
void setup_2d (int w, int h, double left, double right, double bottom, double top);
void setup_2d (gboolean clear, Color clear_color, int w, int h);
int  StopPicking( GLuint *buffer );
void display_msg (double x, double y, int w, int h, const float *colors, int font, const char *string, ...);
void draw_sphere (double* center, double radius, const float color[3], int slices, int stacks);
void draw_arrow (int c, int r, double radius, double angle);
void mono_to_red_blue (double val, double maxval, float *cr, float *cg, float *cb);

void glutil_draw_3d_quad (double *p);
int
draw_ellipse (double x0, double y0, double s11, double s12, double s22, int pointsize, int steps, const float *color,
              int c0, int r0, float size);
void
draw_ellipse (double x, double y, double orient, double lenx, double leny, const float *color, int pointsize);
void draw_point (double x1, double y1, const float *color, int full, int size, int linewidth);
void draw_box (double x1, double y1, double x2, double y2, const float *color, int full, int linewidth);



int gl_intersect_ray_plane( double *p, double *u, double *q, double *n, double *result );
void gl_project ( double *xyz, double *x, double *y);
int gl_raycast_to_plane ( double winx, double winy, double *eye, double *target, double *xyz );


#endif
