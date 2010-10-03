#ifndef _GLUTIL_H__
#define _GLUTIL_H__

/* From the standard C library */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <math.h>

/* From the GL library */
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include "common/colors.h"
#include "model.h"
#include "math/quat.h"
#include "jpegcodec/imgload.h"
#include "image/image.h"
#include "camera/camera.h"
#include "common/log.h"
#include "common/gltool.h"
#include "common/timer.h"

/* From the VL library */
#include "vl/VLf.h"


/* a structure for a 3D viewer */

class Viewer3dParam {
 public:
    float rho;//,theta,phi;
    //float roll; // rotation along the z-axis of the camera
    float zNear, zFar;
    float fov;
    Vec3f eye, target, unit, up, lat;
    int _mode; // 0 for outside view, 1 for inside view

    Viewer3dParam();
    void Print();
    void Print(FILE *fp);
    void Read(FILE *fp);

    void Reset();

    void Update();
    void Zoom (float r);
    void Yaw(float r);
    void Pitch(float r);
    void Roll(float r);

    Quaternion GetRotation();
    Vec3f GetTranslation();
    void SetPose( Quaternion q, Vec3f t );
    void Translation( Vec3f f);
    void Rotation( Quaternion q );
    int Mode( int mode);
    inline int Mode() { return _mode; }

    inline CamExtr toCamExtr() 
        { return CamExtr( GetRotation(), GetTranslation() ); };

    //void RotatePhi( float r);
    //void RotateTheta( float r );

    //void ApplyRoll();
};


void DrawModel( Model *model, int textureEnabled );
void DrawModelFeedbackMode( Model *model, int width, int height, \
                            Viewer3dParam *par, vector<int> &vertexIndices );


// OpenGL setup
void Setup3d ( Viewer3dParam *par);
void Setup3dSelect (Viewer3dParam *par, GLuint *buffer, int cursorX, int cursorY );

void GlLighting( int on, Vec3f p );
void GlScreenshot( int w, int h, const char *filename );

// projecting on a camera and drawing in 2D
void DrawModel2D( Model *model, CamExtr *extr, CamIntr *intr, 
                  int textureEnabled);
void Draw3DPoints2D( vector<Vec3f> &points, Color color, int size, int select,
                     CamExtr *extr, CamIntr *intr );

void DrawCamera( CamExtr *extr, float size, Color color );

#endif
