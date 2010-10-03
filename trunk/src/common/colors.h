#ifndef __COLORS_H__
#define __COLORS_H__

/* From the GL library */
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

// should probably be called pixels
static const float RED[3] = {1.0,0.0,0.0};
static const float PINK[3] = {1.0,0.0,1.0};
static const float YELLOW[3] = {1.0,1.0,0.0};
static const float ORANGE[3] = {1.0,0.5,0.0};
static const float GREEN[3] = {0.0,1.0,0.0};
static const float BLUEGREEN[3] = {0.0,1.0,1.0};
static const float BLUE[3] = {0.0,0.0,1.0};
static const float REDGREEN[3]={1.0,1.0,0.0};
static const float REDBLUE[3]={1.0,0.0,1.0};
static const float DULLRED[3]={0.5,0.0,0.0};
static const float DULLBLUE[3]={0.0,0.0,0.5};
static const float WHITE[3]={1.0,1.0,1.0};
static const float DULLWHITE[3]={0.2,0.2,0.2};
static const float GREY[3]={0.2,0.2,0.2};
static const float BLACK[3]={0.0,0.0,0.0};
static const float REDGREEN2[3]={1.0,0.5,0.0};
static const float GRAY[3]={0.2,0.2,0.2};
static const float LIGHTBLUE[3]={0.0/255.0,25/255.0,50/255.0};
static const float PURPLE[3]={1.0, 0.14, 0.6667};



static const void* fonts[7] = {
GLUT_BITMAP_9_BY_15, 
GLUT_BITMAP_8_BY_13,
GLUT_BITMAP_TIMES_ROMAN_10, 
GLUT_BITMAP_TIMES_ROMAN_24, 
GLUT_BITMAP_HELVETICA_10,
GLUT_BITMAP_HELVETICA_12 ,
GLUT_BITMAP_HELVETICA_18 
};

static const int NCOLORS = 10;
static const float _colors[NCOLORS][3] = {
  {1.0, 1.0, 1.0},	// 0: WHITE
  {1.0, 0.0, 0.0},	// 1: RED
  {0.0, 1.0, 0.0},	// 2: GREEN
  {0.0, 0.0, 1.0},	// 3: BLUE
  {1.0, 0.0, 1.0},	// 4: PINK
  {0.0, 1.0, 1.0},	// 5: EMERAUDE
  {1.0, 0.5, 0.0},	// 6: ORANGE
  {1.0, 0.14, 0.6667},	// 7: PURPLE
  {0.2, 0.2, 0.2},	// 8: GRAY
  {0.0, 0.0, 0.0},	// 9: BLACK
};

typedef const float* Color;

#endif
