#include "gltool.h"

float _circle_points[][2] = {
    { 1.000000, 0.000000 }, { 0.999848, 0.017452 }, { 0.999391, 0.034899 },
    { 0.998630, 0.052336 }, { 0.997564, 0.069756 }, { 0.996195, 0.087156 },
    { 0.994522, 0.104528 }, { 0.992546, 0.121869 }, { 0.990268, 0.139173 },
    { 0.987688, 0.156434 }, { 0.984808, 0.173648 }, { 0.981627, 0.190809 },
    { 0.978148, 0.207912 }, { 0.974370, 0.224951 }, { 0.970296, 0.241922 },
    { 0.965926, 0.258819 }, { 0.961262, 0.275637 }, { 0.956305, 0.292372 },
    { 0.951057, 0.309017 }, { 0.945519, 0.325568 }, { 0.939693, 0.342020 },
    { 0.933580, 0.358368 }, { 0.927184, 0.374607 }, { 0.920505, 0.390731 },
    { 0.913545, 0.406737 }, { 0.906308, 0.422618 }, { 0.898794, 0.438371 },
    { 0.891007, 0.453990 }, { 0.882948, 0.469472 }, { 0.874620, 0.484810 },
    { 0.866025, 0.500000 }, { 0.857167, 0.515038 }, { 0.848048, 0.529919 },
    { 0.838671, 0.544639 }, { 0.829038, 0.559193 }, { 0.819152, 0.573576 },
    { 0.809017, 0.587785 }, { 0.798636, 0.601815 }, { 0.788011, 0.615661 },
    { 0.777146, 0.629320 }, { 0.766044, 0.642788 }, { 0.754710, 0.656059 },
    { 0.743145, 0.669131 }, { 0.731354, 0.681998 }, { 0.719340, 0.694658 },
    { 0.707107, 0.707107 }, { 0.694658, 0.719340 }, { 0.681998, 0.731354 },
    { 0.669131, 0.743145 }, { 0.656059, 0.754710 }, { 0.642788, 0.766044 },
    { 0.629320, 0.777146 }, { 0.615661, 0.788011 }, { 0.601815, 0.798636 },
    { 0.587785, 0.809017 }, { 0.573576, 0.819152 }, { 0.559193, 0.829038 },
    { 0.544639, 0.838671 }, { 0.529919, 0.848048 }, { 0.515038, 0.857167 },
    { 0.500000, 0.866025 }, { 0.484810, 0.874620 }, { 0.469472, 0.882948 },
    { 0.453990, 0.891007 }, { 0.438371, 0.898794 }, { 0.422618, 0.906308 },
    { 0.406737, 0.913545 }, { 0.390731, 0.920505 }, { 0.374607, 0.927184 },
    { 0.358368, 0.933580 }, { 0.342020, 0.939693 }, { 0.325568, 0.945519 },
    { 0.309017, 0.951057 }, { 0.292372, 0.956305 }, { 0.275637, 0.961262 },
    { 0.258819, 0.965926 }, { 0.241922, 0.970296 }, { 0.224951, 0.974370 },
    { 0.207912, 0.978148 }, { 0.190809, 0.981627 }, { 0.173648, 0.984808 },
    { 0.156434, 0.987688 }, { 0.139173, 0.990268 }, { 0.121869, 0.992546 },
    { 0.104528, 0.994522 }, { 0.087156, 0.996195 }, { 0.069756, 0.997564 },
    { 0.052336, 0.998630 }, { 0.034899, 0.999391 }, { 0.017452, 0.999848 },
    { 0.000000, 1.000000 }, { -0.017452, 0.999848 }, { -0.034899, 0.999391 },
    { -0.052336, 0.998630 }, { -0.069756, 0.997564 }, { -0.087156, 0.996195 },
    { -0.104528, 0.994522 }, { -0.121869, 0.992546 }, { -0.139173, 0.990268 },
    { -0.156434, 0.987688 }, { -0.173648, 0.984808 }, { -0.190809, 0.981627 },
    { -0.207912, 0.978148 }, { -0.224951, 0.974370 }, { -0.241922, 0.970296 },
    { -0.258819, 0.965926 }, { -0.275637, 0.961262 }, { -0.292372, 0.956305 },
    { -0.309017, 0.951057 }, { -0.325568, 0.945519 }, { -0.342020, 0.939693 },
    { -0.358368, 0.933580 }, { -0.374607, 0.927184 }, { -0.390731, 0.920505 },
    { -0.406737, 0.913545 }, { -0.422618, 0.906308 }, { -0.438371, 0.898794 },
    { -0.453990, 0.891007 }, { -0.469472, 0.882948 }, { -0.484810, 0.874620 },
    { -0.500000, 0.866025 }, { -0.515038, 0.857167 }, { -0.529919, 0.848048 },
    { -0.544639, 0.838671 }, { -0.559193, 0.829038 }, { -0.573576, 0.819152 },
    { -0.587785, 0.809017 }, { -0.601815, 0.798636 }, { -0.615661, 0.788011 },
    { -0.629320, 0.777146 }, { -0.642788, 0.766044 }, { -0.656059, 0.754710 },
    { -0.669131, 0.743145 }, { -0.681998, 0.731354 }, { -0.694658, 0.719340 },
    { -0.707107, 0.707107 }, { -0.719340, 0.694658 }, { -0.731354, 0.681998 },
    { -0.743145, 0.669131 }, { -0.754710, 0.656059 }, { -0.766044, 0.642788 },
    { -0.777146, 0.629320 }, { -0.788011, 0.615661 }, { -0.798636, 0.601815 },
    { -0.809017, 0.587785 }, { -0.819152, 0.573576 }, { -0.829038, 0.559193 },
    { -0.838671, 0.544639 }, { -0.848048, 0.529919 }, { -0.857167, 0.515038 },
    { -0.866025, 0.500000 }, { -0.874620, 0.484810 }, { -0.882948, 0.469472 },
    { -0.891007, 0.453990 }, { -0.898794, 0.438371 }, { -0.906308, 0.422618 },
    { -0.913545, 0.406737 }, { -0.920505, 0.390731 }, { -0.927184, 0.374607 },
    { -0.933580, 0.358368 }, { -0.939693, 0.342020 }, { -0.945519, 0.325568 },
    { -0.951057, 0.309017 }, { -0.956305, 0.292372 }, { -0.961262, 0.275637 },
    { -0.965926, 0.258819 }, { -0.970296, 0.241922 }, { -0.974370, 0.224951 },
    { -0.978148, 0.207912 }, { -0.981627, 0.190809 }, { -0.984808, 0.173648 },
    { -0.987688, 0.156434 }, { -0.990268, 0.139173 }, { -0.992546, 0.121869 },
    { -0.994522, 0.104528 }, { -0.996195, 0.087156 }, { -0.997564, 0.069756 },
    { -0.998630, 0.052336 }, { -0.999391, 0.034899 }, { -0.999848, 0.017452 },
    { -1.000000, 0.000000 }, { -0.999848, -0.017452 }, { -0.999391, -0.034899 },
    { -0.998630,-0.052336 }, { -0.997564, -0.069756 }, { -0.996195, -0.087156 },
    { -0.994522,-0.104528 }, { -0.992546, -0.121869 }, { -0.990268, -0.139173 },
    { -0.987688,-0.156434 }, { -0.984808, -0.173648 }, { -0.981627, -0.190809 },
    { -0.978148,-0.207912 }, { -0.974370, -0.224951 }, { -0.970296, -0.241922 },
    { -0.965926,-0.258819 }, { -0.961262, -0.275637 }, { -0.956305, -0.292372 },
    { -0.951057,-0.309017 }, { -0.945519, -0.325568 }, { -0.939693, -0.342020 },
    { -0.933580,-0.358368 }, { -0.927184, -0.374607 }, { -0.920505, -0.390731 },
    { -0.913545,-0.406737 }, { -0.906308, -0.422618 }, { -0.898794, -0.438371 },
    { -0.891007,-0.453990 }, { -0.882948, -0.469472 }, { -0.874620, -0.484810 },
    { -0.866025,-0.500000 }, { -0.857167, -0.515038 }, { -0.848048, -0.529919 },
    { -0.838671,-0.544639 }, { -0.829038, -0.559193 }, { -0.819152, -0.573576 },
    { -0.809017,-0.587785 }, { -0.798636, -0.601815 }, { -0.788011, -0.615661 },
    { -0.777146,-0.629320 }, { -0.766044, -0.642788 }, { -0.754710, -0.656059 },
    { -0.743145,-0.669131 }, { -0.731354, -0.681998 }, { -0.719340, -0.694658 },
    { -0.707107,-0.707107 }, { -0.694658, -0.719340 }, { -0.681998, -0.731354 },
    { -0.669131,-0.743145 }, { -0.656059, -0.754710 }, { -0.642788, -0.766044 },
    { -0.629320,-0.777146 }, { -0.615661, -0.788011 }, { -0.601815, -0.798636 },
    { -0.587785,-0.809017 }, { -0.573576, -0.819152 }, { -0.559193, -0.829038 },
    { -0.544639,-0.838671 }, { -0.529919, -0.848048 }, { -0.515038, -0.857167 },
    { -0.500000,-0.866025 }, { -0.484810, -0.874620 }, { -0.469472, -0.882948 },
    { -0.453990,-0.891007 }, { -0.438371, -0.898794 }, { -0.422618, -0.906308 },
    { -0.406737,-0.913545 }, { -0.390731, -0.920505 }, { -0.374607, -0.927184 },
    { -0.358368,-0.933580 }, { -0.342020, -0.939693 }, { -0.325568, -0.945519 },
    { -0.309017,-0.951057 }, { -0.292372, -0.956305 }, { -0.275637, -0.961262 },
    { -0.258819,-0.965926 }, { -0.241922, -0.970296 }, { -0.224951, -0.974370 },
    { -0.207912,-0.978148 }, { -0.190809, -0.981627 }, { -0.173648, -0.984808 },
    { -0.156434,-0.987688 }, { -0.139173, -0.990268 }, { -0.121869, -0.992546 },
    { -0.104528,-0.994522 }, { -0.087156, -0.996195 }, { -0.069756, -0.997564 },
    { -0.052336,-0.998630 }, { -0.034899, -0.999391 }, { -0.017452, -0.999848 },
    { -0.000000, -1.000000 }, { 0.017452, -0.999848 }, { 0.034899, -0.999391 },
    { 0.052336, -0.998630 }, { 0.069756, -0.997564 }, { 0.087156, -0.996195 },
    { 0.104528, -0.994522 }, { 0.121869, -0.992546 }, { 0.139173, -0.990268 },
    { 0.156434, -0.987688 }, { 0.173648, -0.984808 }, { 0.190809, -0.981627 },
    { 0.207912, -0.978148 }, { 0.224951, -0.974370 }, { 0.241922, -0.970296 },
    { 0.258819, -0.965926 }, { 0.275637, -0.961262 }, { 0.292372, -0.956305 },
    { 0.309017, -0.951057 }, { 0.325568, -0.945519 }, { 0.342020, -0.939693 },
    { 0.358368, -0.933580 }, { 0.374607, -0.927184 }, { 0.390731, -0.920505 },
    { 0.406737, -0.913545 }, { 0.422618, -0.906308 }, { 0.438371, -0.898794 },
    { 0.453990, -0.891007 }, { 0.469472, -0.882948 }, { 0.484810, -0.874620 },
    { 0.500000, -0.866025 }, { 0.515038, -0.857167 }, { 0.529919, -0.848048 },
    { 0.544639, -0.838671 }, { 0.559193, -0.829038 }, { 0.573576, -0.819152 },
    { 0.587785, -0.809017 }, { 0.601815, -0.798636 }, { 0.615661, -0.788011 },
    { 0.629320, -0.777146 }, { 0.642788, -0.766044 }, { 0.656059, -0.754710 },
    { 0.669131, -0.743145 }, { 0.681998, -0.731354 }, { 0.694658, -0.719340 },
    { 0.707107, -0.707107 }, { 0.719340, -0.694658 }, { 0.731354, -0.681998 },
    { 0.743145, -0.669131 }, { 0.754710, -0.656059 }, { 0.766044, -0.642788 },
    { 0.777146, -0.629320 }, { 0.788011, -0.615661 }, { 0.798636, -0.601815 },
    { 0.809017, -0.587785 }, { 0.819152, -0.573576 }, { 0.829038, -0.559193 },
    { 0.838671, -0.544639 }, { 0.848048, -0.529919 }, { 0.857167, -0.515038 },
    { 0.866025, -0.500000 }, { 0.874620, -0.484810 }, { 0.882948, -0.469472 },
    { 0.891007, -0.453990 }, { 0.898794, -0.438371 }, { 0.906308, -0.422618 },
    { 0.913545, -0.406737 }, { 0.920505, -0.390731 }, { 0.927184, -0.374607 },
    { 0.933580, -0.358368 }, { 0.939693, -0.342020 }, { 0.945519, -0.325568 },
    { 0.951057, -0.309017 }, { 0.956305, -0.292372 }, { 0.961262, -0.275637 },
    { 0.965926, -0.258819 }, { 0.970296, -0.241922 }, { 0.974370, -0.224951 },
    { 0.978148, -0.207912 }, { 0.981627, -0.190809 }, { 0.984808, -0.173648 },
    { 0.987688, -0.156434 }, { 0.990268, -0.139173 }, { 0.992546, -0.121869 },
    { 0.994522, -0.104528 }, { 0.996195, -0.087156 }, { 0.997564, -0.069756 },
    { 0.998630, -0.052336 }, { 0.999391, -0.034899 }, { 0.999848, -0.017452 },
};

void display_msg (double x, double y, int width, int height, char *string,
                  const float *colors, int font) 
{
    int i;
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0,width,height,0,-1.0,1.0);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix ();
    glLoadIdentity();
    
    glColor3fv(colors);
    glRasterPos2f(x, y);

    for (i = 0; i < ((int) strlen(string)); i++) {
        glutBitmapCharacter((void*)fonts[font], string[i]);
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    
}

void display_msg (double x, double y, int width, int height,
                  const float *colors, int font, const char *string, ...) 
{
    if (strlen(string) < 128) {
        
        char buf[256];
        
        va_list arg_ptr;
        
        va_start (arg_ptr,string);
        
        vsprintf(buf,string,arg_ptr);
        
        va_end (arg_ptr);
        
        display_msg(x,y, width, height, buf, colors, font);
    }
}

void draw_sphere (double *center, double radius, const float color[3], int slices, int stacks)
{
    GLUquadricObj* pQuadric = gluNewQuadric();
    glPushMatrix();
    glTranslatef(center[0],center[1],center[2]);
    glColor(color);
    gluSphere(pQuadric,radius,slices,stacks);
    glPopMatrix();
}

    void
draw_ellipse (double x, double y, double orient, double lenx, double leny, const float *color, int pointsize)
{
    glLineWidth (pointsize);
    glColor (color);
    glBegin (GL_LINE_LOOP);

    int decimate = 4;
    for (unsigned int i=0; i<sizeof (_circle_points) / (2 * sizeof (float)); i+=decimate) {
        double a = lenx * _circle_points[i][0];
        double b = leny * _circle_points[i][1];
        glVertex2f (x + a * cos(orient) + b*sin(orient), y + -a*sin(orient) + b*cos(orient));
    }

    glEnd ();
}

void draw_point (double x1, double y1, const float *color, int full, int size, int linewidth)
{
    glColor3f (color[0], color[1], color[2]);
    double x2 = x1 + size/2; double y2 = y1 + size/2;
    x1 -= size/2; y1 -= size/2;

    draw_box (x1, y1, x2, y2, color, full, linewidth);
}


void draw_box (double x1, double y1, double x2, double y2, const float *color, int full, int linewidth)
{
    glColor3f (color[0], color[1], color[2]);

    if (full) {
        glBegin (GL_QUADS);
        glVertex2f (x1, y1); glVertex2f (x2, y1);
        glVertex2f (x2, y2); glVertex2f (x1, y2);
        glEnd ();
    } else {
        glLineWidth(linewidth);
        glBegin (GL_LINES);
        glVertex2f (x1, y1); glVertex2f (x2, y1);
        glVertex2f (x2, y1); glVertex2f (x2, y2);
        glVertex2f (x2, y2); glVertex2f (x1, y2);
        glVertex2f (x1, y2); glVertex2f (x1, y1);
        glEnd();
    }
}

// draw an ellipse centered on (x0, y0) which parameters are S11, S12, S22 of x' inv(S) x = 1
// 
// the ellipse is drawn as a set of points (steps specifies the resolution in the x direction)
//
    int
draw_ellipse (double x0, double y0, double s11, double s12, double s22, int pointsize, int steps, const float *color,
        int c0, int r0, float ratio)
{
    // invert the S matrix
    double delta = s11*s22-s12*s12;
    if (fabs (delta) < NAVGUIDE_EPS) {
        dbg (DBG_ERROR, "[ellipse] delta = %.4f < 1e-6", delta);
        return -1;
    }
    double a = s22 / delta;
    double b = -s12 / delta;
    double c = b;
    double d = s11 / delta;

    // compute xmin and xmax
    delta = 4*a*d-(b+c)*(b+c);
    if (fabs(delta) < NAVGUIDE_EPS) {
        dbg (DBG_ERROR, "[ellipse] delta 2 = %.4f < 1e-6", delta);
        return -1;
    }
    delta = 4*d / delta;
    if (delta < NAVGUIDE_EPS) {
        dbg (DBG_ERROR, "[ellipse] delta = %.4f < 0", delta);
        return -1;
    }

    double xmin = -sqrt (delta);
    double xmax = sqrt (delta);

    glPointSize (pointsize);
    glColor3f (color[0], color[1], color[2]);
    glBegin (GL_POINTS);

    // draw ellipse center
    glVertex2f (c0+y0*ratio, r0+x0*ratio);

    for (int i=0;i<steps;i++) {

        double x = xmin + i * 1.0 / steps * (xmax-xmin);

        delta = (b+c)*(b+c)*x*x-4*d*(a*x*x-1);

        if (delta < NAVGUIDE_EPS)
            continue;

        double y1 = (-(b+c)*x-sqrt(delta))/(2*d);
        double y2 = (-(b+c)*x+sqrt(delta))/(2*d);

        glVertex2f (c0+(y0 + y1)*ratio, r0+(x + x0)*ratio);
        glVertex2f (c0+(y0 + y2)*ratio, r0+(x + x0)*ratio);
    }

    glEnd ();

    return 0;
}

void
glColor( Color c ) {
    glColor3d(c[0], c[1], c[2] );
}

void gl_clear_window( Color color )
{
    glClearColor(color[0], color[1], color[2], 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

    void 
DrawAxis( int size )
{
    glLineWidth(2);

    // z-axis
    glColor(RED);
    glBegin(GL_LINES);
    glVertex3f(0,0,0);
    glVertex3f(0,0,size);
    glEnd();

    // x-axis
    glColor(GREEN);
    glBegin(GL_LINES);
    glVertex3f(0,0,0);
    glVertex3f(size,0,0);
    glEnd();

    // y-axis
    glColor(BLUE);
    glBegin(GL_LINES);
    glVertex3f(0,0,0);
    glVertex3f(0,size,0);
    glEnd();


}


void glutil_draw_3d_quad (double *p)
{
    glBegin (GL_QUADS);
    glVertex3f (p[0], p[1], p[2]);
    glVertex3f (p[3], p[4], p[5]);
    glVertex3f (p[6], p[7], p[8]);
    glVertex3f (p[9], p[10], p[11]);
    glEnd();
}

int StopPicking( GLuint *buffer )
{
    unsigned int hits;

    int output = -1;

    // restoring the original projection matrix
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glFlush();

    // returning to normal rendering mode
    hits = glRenderMode(GL_RENDER);

    // if there are hits process them
    if (hits != 0) {
        unsigned int i, j;
        GLuint names, *ptr, minZ,*ptrNames, numberOfNames;

        //printf ("hits = %d\n", hits);
        ptr = (GLuint *) buffer;
        minZ = 0xffffffff;
        for (i = 0; i < hits; i++) {	
            names = *ptr;
            ptr++;
            if (*ptr < minZ) {
                numberOfNames = names;
                minZ = *ptr;
                ptrNames = ptr+2;
            }

            ptr += names+2;
        }
        //printf ("The closest hit names are ");
        ptr = ptrNames;
        for (j = 0; j < numberOfNames; j++,ptr++) {
            output = *ptr;
            //printf ("%d ", *ptr);
        }
        //printf ("\n");
    }

    return output;
}

// draw an arrow
//
void draw_arrow (int c, int r, double radius, double angle)
{
    double uc = -sin(angle);
    double ur = cos(angle);

    double c1 = c + radius * uc / 10.0;
    double r1 = r + radius * ur / 10.0;
    double c2 = c - radius * uc / 10.0;
    double r2 = r - radius * ur / 10.0;
    double c3 = c + radius * cos(angle);
    double r3 = r + radius * sin(angle);

    glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
    glBegin (GL_TRIANGLES);
    glVertex2f(c1,r1);
    glVertex2f(c3,r3);
    glVertex2f(c2,r2);
    glEnd();
}

// convert a single data to RGB color 
// positive is red, negative is blue
//
void mono_to_red_blue (double val, double maxval, float *cr, float *cg, float *cb)
{
    if (maxval < 1E-6)
        maxval = 1.0;

    if (val>0) {
        *cr = val / maxval;
        *cg = 0.0;
        *cb = 0.0;
    } else {
        *cr = 0.0;
        *cg = 0.0;
        *cb = - val / maxval;
    }        
}

void setup_2d (int w, int h, double left, double right, double bottom, double top)
{
    // setup the 2D view
    glViewport(0, 0, w, h);        // reset the viewport to new dimensions 
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(left, right, bottom, top);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void setup_2d (gboolean clear, Color clear_color, int w, int h)
{
    // clear screen
    if (clear)
        gl_clear_window (clear_color);

    // setup 2D
    setup_2d (w, h, 0, w, h, 0);
}

/* intersect ray (p,u) with plane (q,n) assuming that u and n are two normal vectors and
   p and q are two 3d points */
int gl_intersect_ray_plane( double *p, double *u, double *q, double *n, double *result )
{
    double lambda1 = u[0]*n[0] + u[1]*n[1] + u[2]*n[2];

    if ( fabs(lambda1) < 1E-6 )
        return -1;

    double lambda2 = (q[0]-p[0])*n[0] + (q[1]-p[1])*n[1] + (q[2]-p[2])*n[2];

    result[0] = p[0] + lambda2 / lambda1 * u[0];
    result[1] = p[1] + lambda2 / lambda1 * u[1];
    result[2] = p[2] + lambda2 / lambda1 * u[2];

    return 0;
}
/* project a 3D point xyz on the screen (x,y) */
void gl_project ( double *xyz, double *x, double *y)
{
    double z;

    GLdouble modelMatrix[16];
    glGetDoublev( GL_MODELVIEW_MATRIX, modelMatrix);

    GLdouble projMatrix[16];
    glGetDoublev( GL_PROJECTION_MATRIX, projMatrix);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    //printf("projecting %f %f %f\n", xyz[0], xyz[1], xyz[2]);
    gluProject( xyz[0], xyz[1], xyz[2], modelMatrix, projMatrix, viewport, x, y, &z );

    //*x /= z;
    //*y /= z;
}

/* raycast a 3D ray from the viewer point and intersect it with the plane perpendicular to target */

int gl_raycast_to_plane ( double winx, double winy, double *eye, double *target, double *xyz )
{
    GLdouble modelMatrix[16];
    glGetDoublev( GL_MODELVIEW_MATRIX, modelMatrix);

    GLdouble projMatrix[16];
    glGetDoublev( GL_PROJECTION_MATRIX, projMatrix);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    double x,y,z;

    gluUnProject( winx, winy, 1.0, modelMatrix, projMatrix, viewport, &x, &y, &z );

    double u[3]; // unit ray vector
    u[0] = x - eye[0];
    u[1] = y - eye[1];
    u[2] = z - eye[2];

    double lu = sqrt(u[0]*u[0] + u[1]*u[1] + u[2]*u[2]);
    if ( lu < 1E-6 )
        return -1;

    u[0] /= lu;
    u[1] /= lu;
    u[2] /= lu;

    double n[3];
    n[0] = 0.0;
    n[1] = 0.0;
    n[2] = 1.0;

    // intersect the ray with the map
    int ok = gl_intersect_ray_plane( eye, u, target, n, xyz );

    return ok;
}

