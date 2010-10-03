#include "main.h"

static viewmap_t *g_viewmap;

void gl_look_at (double eye[3], double center[3], double up[3])
{
    gluLookAt (eye[0], eye[1], eye[2], center[0], center[1], center[2], up[0], up[1], up[2]);
}

void view3d_print (view3d_t *view)
{
    if (view) {
        printf ("eye: %.3f, %.3f, %.3f\n", view->eye[0], view->eye[1], view->eye[2]);
        printf ("center: %.3f, %.3f, %.3f\n", view->center[0], view->center[1], view->center[2]);
        printf (" up: %.3f, %.3f, %.3f\n", view->up[0], view->up[1], view->up[2]);
    }
}

void viewmap_print (viewmap_t *viewmap)
{
    if (!viewmap) return;

    printf ("***********************\n");
    view3d_print (&viewmap->extr);
    view3d_print (&viewmap->intr);
}

viewmap_t* viewmap_init (int w, int h)
{
    viewmap_t *v = (viewmap_t*)malloc(sizeof(viewmap_t));

    // intrinsic
    v->intr.eye[0] = 0.0;
    v->intr.eye[1] = 0.0;
    v->intr.eye[2] = 0.0;

    v->intr.center[0] = 1.0;
    v->intr.center[1] = 0.0;
    v->intr.center[2] = 0.0;

    v->intr.up[0] = 0.0;
    v->intr.up[1] = 0.0;
    v->intr.up[2] = 1.0;

    v->intr.fovy = 90.0;
    v->intr.znear = 1.0;
    v->intr.zfar = 1000.0;
    v->intr.w = w;
    v->intr.h = h;

    // extrinsic
    v->extr.eye[0] = 16.6;
    v->extr.eye[1] = -37.0;
    v->extr.eye[2] = -29.0;

    v->extr.center[0] = 0.0;
    v->extr.center[1] = 0.0;
    v->extr.center[2] = 0.0;

    v->extr.up[0] = .84;
    v->extr.up[1] = .11;
    v->extr.up[2] = .54;
    vect_norm (v->extr.up, 3);

    v->extr.fovy = 80.0;
    v->extr.znear = 1.0;
    v->extr.zfar = 10000.0;
    v->extr.w = w;
    v->extr.h = h;

    v->mode = MAP_VIEW_EXTR;

    return v;
}

void setup_3d (double eye[3], double center[3], double up[3], double fovy, double znear, double zfar, int w, int h)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double aspect = 1.0 * w / h;
    printf ("CHECKL %.3f %.3f %.3f\n", aspect, znear, zfar);
    gluPerspective(fovy, aspect, znear, zfar);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gl_look_at(eye, center, up);

    glPolygonMode(GL_FRONT,GL_LINE);
    glEnable (GL_DEPTH_TEST);
    glDepthMask (GL_TRUE);
}

double* get_norm_dir (view3d_t *view, int mode)
{
    double *normdir = (double*)malloc(3*sizeof(double));
    normdir[0] = view->center[0] - view->eye[0];
    normdir[1] = view->center[1] - view->eye[1];
    normdir[2] = view->center[2] - view->eye[2];
    double length = sqrt(pow(normdir[0],2) + pow(normdir[1],2) + pow(normdir[2],2));
    if (length > 1E-6) {
        normdir[0] /= length;
        normdir[1] /= length;
        normdir[2] /= length;
    }
    return normdir;
}

double get_center_distance (view3d_t *view, int mode)
{
    double length = 1.0;
    double a,b,c;

    a = view->center[0] - view->eye[0];
    b = view->center[1] - view->eye[1];
    c = view->center[2] - view->eye[2];
    length = sqrt(pow(a,2) + pow(b,2) + pow(c,2));

    return length;
}

void setup_3d (viewmap_t *viewmap)
{
    view3d_t *view = NULL;
    if (viewmap->mode == MAP_VIEW_EXTR)
        view = &viewmap->extr;
    else if (viewmap->mode == MAP_VIEW_INTR)
        view = &viewmap->intr;
    if (!view) return;

    printf ("++++++++++++++++++++++++++++++++++++++++++\n");
    view3d_print(view);
    setup_3d (view->eye, view->center, view->up, view->fovy, view->znear, view->zfar, view->w, view->h);
}

void viewmap_rotate (viewmap_t *viewmap, double dx, double dy, double dz, double sx, double sy, double sz)
{
    double new_dir[3], new_dir2[3];
    double ort[3];
    double new_up[3];

    if (!viewmap) return;

    if (viewmap->mode == MAP_VIEW_INTR) {
        // rotate center along the up vector (dx)
        double length = get_center_distance (&viewmap->intr, viewmap->mode);
        double *normdir = get_norm_dir (&viewmap->intr, viewmap->mode); 
        quat_rot_axis (viewmap->extr.up, sx * dx, normdir, new_dir);
        free (normdir);
        for (int i=0;i<3;i++) viewmap->intr.center[i] = viewmap->intr.eye[i] + length * new_dir[3];
        // rotate up along center x up (dy)
        math_cross_product (new_dir, viewmap->intr.up, ort);
        quat_rot_axis (ort, sy * dy, viewmap->intr.up, new_up);
        for (int i=0;i<3;i++) viewmap->intr.up[i] = new_up[i];
        // rotate up along center (dz)
        quat_rot_axis (new_dir, sz * dz, viewmap->intr.up, new_up);
        for (int i=0;i<3;i++) viewmap->intr.up[i] = new_up[i];
    }

    if (viewmap->mode == MAP_VIEW_EXTR) {
        double length = get_center_distance (&viewmap->extr, viewmap->mode);
        double *normdir = get_norm_dir (&viewmap->extr, viewmap->mode);
        // rotate center along the up vector (dx)
        quat_rot_axis (viewmap->extr.up, sx * dx, normdir, new_dir);
        vect_norm(new_dir, 3);
        free (normdir);
        for (int i=0;i<3;i++) viewmap->extr.eye[i] = viewmap->extr.center[i] - length * new_dir[i];
        // rotate up along center x up (dy)
        math_cross_product (new_dir, viewmap->extr.up, ort);
        vect_norm(ort,3);
        quat_rot_axis (ort, sy * dy, viewmap->extr.up, new_up);
        vect_norm(new_up,3);
        for (int i=0;i<3;i++) viewmap->extr.up[i] = new_up[i];
        quat_rot_axis (ort, sy * dy, new_dir, new_dir2);
        vect_norm(new_dir2,3);
        for (int i=0;i<3;i++) viewmap->extr.eye[i] = viewmap->extr.center[i] - length * new_dir2[i];
        // rotate up along center (dz)
        quat_rot_axis (new_dir, sz * dz, viewmap->extr.up, new_up);
        vect_norm(new_up,3);
        for (int i=0;i<3;i++) viewmap->extr.up[i] = new_up[i];
    }
}

void viewmap_zoom (viewmap_t *viewmap, double val, double sens)
{
    if (!viewmap) return;

    if (viewmap->mode == MAP_VIEW_EXTR) {
        double length = get_center_distance (&viewmap->extr, viewmap->mode);
        double *normdir = get_norm_dir (&viewmap->extr, viewmap->mode);
        length = length * (1.0 + val * sens);
        for (int i=0;i<3;i++) viewmap->extr.eye[i] = viewmap->extr.center[i] - length * normdir[i];
        free (normdir);
    }
}

void viewmap_switch_mode (viewmap_t *viewmap)
{
    if (!viewmap) return;

    if (viewmap->mode == MAP_VIEW_EXTR)
        viewmap->mode = MAP_VIEW_INTR;
    else
        viewmap->mode = MAP_VIEW_EXTR;
}

void draw_3d_axis (double *c, double length)
{
    glLineWidth(5);
    glColor(RED); // x-axis
    glBegin (GL_LINES);
    glVertex3f(c[0],c[1],c[2]);
    glVertex3f(c[0]+length, c[1], c[2]);
    glEnd();
    glColor(GREEN); // y-axis
    glBegin (GL_LINES);
    glVertex3f(c[0],c[1],c[2]);
    glVertex3f(c[0], c[1]+length, c[2]);
    glEnd();
    glColor(BLUE); // z-axis
    glBegin (GL_LINES);
    glVertex3f(c[0],c[1],c[2]);
    glVertex3f(c[0], c[1], c[2]+length);
    glEnd();
}

void viewmap_lighting ()
{
    glLightModelfv( GL_LIGHT_MODEL_AMBIENT, globalAmbient );

    glEnable( GL_LIGHT0 );
    glLightfv(  GL_LIGHT0, GL_DIFFUSE,  lightDiffuseLamp  );
    glLightfv(  GL_LIGHT0, GL_AMBIENT,  lightAmbientLamp  );
    glLightfv(  GL_LIGHT0, GL_SPECULAR, lightDiffuseLamp  );
    glLightfv(  GL_LIGHT0, GL_POSITION, lightPositionLamp );

    glMaterialfv(   GL_FRONT_AND_BACK, GL_AMBIENT,  objAmbientHead  );
    glMaterialfv(   GL_FRONT_AND_BACK, GL_DIFFUSE,  objDiffuseHead  );
    glMaterialfv(   GL_FRONT_AND_BACK, GL_SPECULAR, objSpecularHead );
    glMaterialfv(   GL_FRONT_AND_BACK, GL_EMISSION, objEmissionDuck     );

    glShadeModel( GL_SMOOTH ); 

    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST );

}

void viewmap_draw (viewmap_t *viewmap)
{
    if (!viewmap) return;

    // setup the 3D environment
    setup_3d (viewmap);

    // lighting
    //viewmap_lighting ();

    glGetError ();

    glEnable (GL_DEPTH_TEST);

    GLenum error = glGetError ();
    if (error == GL_NO_ERROR) {
        dbg (DBG_ERROR, "no error.");
    }
    else {
        dbg (DBG_ERROR, "Error!");
    }

    // draw axis
    double axis_center[3] = {0,0,0};
    draw_3d_axis (axis_center, 50.0);

    glColor(YELLOW);

    glBegin (GL_QUADS);
    glVertex3f (-10, -10, -10);
    glVertex3f (100, -10, -10);
    glVertex3f (100, 100, -10);
    glVertex3f (-10, 100, -10);
    glEnd();

    glColor (RED);
    glBegin (GL_QUADS);
    glVertex3f (-10, -10, 10);
    glVertex3f (100, -10, 10);
    glVertex3f (100, 100, 10);
    glVertex3f (-10, 100, 10);
    glEnd();

    // draw a random set of spheres
    /*
    for (int i=0;i<10;i++) {
        double center[3];
        center[0] = 5.0 * sqrt(i) + 30.0;
        center[1] = 10.0 * (i - 5) + 20.0;
        center[2] = 0.0;
        double radius = 5.0;
        int colindex = i % NCOLORS;
        //draw_sphere (center, radius, _colors[colindex], 10, 10);
        Draw3DPoint (Vec3f(center[0],center[1],center[2]), _colors[colindex], radius);
    }
    */
}

void render_scene ()
{
    g_viewmap = viewmap_init (320, 320);

    viewmap_draw (g_viewmap);

    glFlush ();
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_SINGLE | GLUT_RGBA);
    glutInitWindowPosition(100,100);
    glutInitWindowSize(320,320);
    glutCreateWindow("3D Tech- GLUT Tutorial");
    glutDisplayFunc(render_scene);
    glutMainLoop();
    return 0;
}


