#include "simcam.h"

model_t sim_read_model (const char *filename)
{
    model_t model;
    model.quad = NULL;
    model.nel = 0;
    model.tex = NULL;

    char fullname[256];
    sprintf (fullname, "%s/sim_img/%s", DATA_DIR, filename);

    FILE *fp = fopen (fullname, "r");
    if (!fp) {
        dbg (DBG_ERROR, "Failed to open model file %s", fullname);
        return model;
    }

    while (!feof (fp)) {
        
        double q[12];

        int ok = 1;

        // read quads
        for (int i=0;i<4;i++) {
            double x,y,z;
            if (fscanf (fp, "%lf%lf%lf", &x, &y, &z)!=3) {
                ok = 0;
                break;
            }
            q[3*i] = x;
            q[3*i+1] = y;
            q[3*i+2] = z;
        }

        if (!ok)
            break;

        // save to structure
        model.quad = (double**)realloc (model.quad, (model.nel+1)*sizeof(double*));
        model.tex = (GLUtilTexture **)realloc (model.tex, (model.nel+1)*sizeof(GLUtilTexture *));

        model.quad[model.nel] = (double*)malloc(12*sizeof(double));
        for (int i=0;i<12;i++)
            model.quad[model.nel][i] = q[i];

        // read filename
        char imgname[256];
        fscanf (fp, "%s", imgname);

        int width, height, nchannels;
        /*
        ImgColor *img = NULL;load_png (imgname, &width, &height, &nchannels);
        if (!img) {
            dbg (DBG_ERROR, "Failed to load image %s", imgname);
            continue;
        } else {
            dbg (DBG_INFO, "Loaded image %s (%d x %d)", imgname, width, height);
        }
        
        // load into texture
        GLUtilTexture *tex = glutil_texture_new (width, height, 3*width*height);
        glutil_texture_upload (tex, GL_RGB, GL_UNSIGNED_BYTE, 3*width, img->IplImage());
        
        // save to structure
        model.tex[model.nel] = tex;
        model.nel++;

        // free image
        delete (img);
        */
    }

    dbg (DBG_INFO, "Read %d model elements.", model.nel);

    fclose (fp);

    return model;
}
            
void
setup_3d ( double *eye, double *unit, double *up )

{
    //dbg(DBG_INFO,"Setting up 3D viewer...\n");
 
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float ratio = 1.0*SIM_WIDTH/SIM_HEIGHT;
    gluPerspective(SIM_FOV, ratio, SIM_ZNEAR, SIM_ZFAR);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(eye[0],eye[1],eye[2],
              eye[0]+unit[0],eye[1]+unit[1],\
              eye[2]+unit[2],
              up[0],up[1],up[2]);
}

void sim_render (model_t model, int tex_on, int wireframe)
{
    if (wireframe) {
        glColor3f(1,0,0);
        for (int i=0;i<model.nel;i++) {
            double *p = model.quad[i];
            glBegin (GL_LINES);
            glVertex3f (p[0], p[1], p[2]);
            glVertex3f (p[3], p[4], p[5]);
            glVertex3f (p[3], p[4], p[5]);
            glVertex3f (p[6], p[7], p[8]);
            glVertex3f (p[6], p[7], p[8]);
            glVertex3f (p[9], p[10], p[11]);
            glVertex3f (p[9], p[10], p[11]);
            glVertex3f (p[0], p[1], p[2]);
            glEnd();
        }

        return;
    }

    if (tex_on) {
        for (int i=0;i<model.nel;i++) {
            glutil_texture_draw_3d_pt (model.tex[i], model.quad[i]);
        }
    } else {
        glColor3f(1.0,0.0,0.0);
        for (int i=0;i<model.nel;i++) {
            double *p = model.quad[i];
            glBegin (GL_QUADS);
            glVertex3f (p[0], p[1], p[2]);
            glVertex3f (p[3], p[4], p[5]);
            glVertex3f (p[6], p[7], p[8]);
            glVertex3f (p[9], p[10], p[11]);
            glEnd();
        }
    }

}


GLfloat *sim_read_pixels (model_t model, double *eye, double *unit, double *up)
{
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glRenderMode (GL_RENDER);
 
    // setup
    setup_3d (eye, unit, up);

    // render
    sim_render (model, SIM_TEX_ON, 0);

    // read pixels
    GLfloat *pixels = (GLfloat*)malloc(3*SIM_WIDTH*SIM_HEIGHT*sizeof(GLfloat));

    glReadPixels(0,0, SIM_WIDTH, SIM_HEIGHT, GL_DEPTH_COMPONENT,GL_FLOAT,(void*)pixels);
    
    return pixels;
}

void sim_model_add_face (model_t *model, double a[3], double b[3], double c[3], double d[3])
{
    model->quad = (double**)realloc (model->quad, (model->nel+1)*sizeof(double));
    double *data = (double*)malloc(12*sizeof(double));
    data[0] = a[0]; data[1] = a[1]; data[2] = a[2];
    data[3] = b[0]; data[4] = b[1]; data[5] = b[2];
    data[6] = c[0]; data[7] = c[1]; data[8] = c[2];
    data[9] = d[0]; data[10] = d[1]; data[11] = d[2];
    
    model->quad[model->nel] = data;
    model->nel++;
}

void sim_model_add_cube (model_t *model, double x, double y, double z, double w)
{
    double q[8][3];
    
    int count = 0;

    // generate points

    for (int i=0;i<2;i++) {
        
        double z1 = i == 0 ? z - w/2 : z + w/2;
        
        for (int j=0;j<2;j++) {
            
            double y1 = j == 0 ? y - w/2 : y + w/2;
            
            for (int k = 0; k<2;k++) {
                
                double x1 = k == 0 ? x - w/2 : x + w/2;
                
                q[count][0] = x1;
                q[count][1] = y1;
                q[count][2] = z1;

                count++;
            }
        }
    }
    
    
    // add cube
    sim_model_add_face (model, q[0], q[1], q[5], q[4]);
    sim_model_add_face (model, q[1], q[3], q[7], q[5]);
    sim_model_add_face (model, q[2], q[6], q[7], q[3]);
    sim_model_add_face (model, q[0], q[4], q[6], q[2]);
    sim_model_add_face (model, q[5], q[7], q[6], q[4]);
    sim_model_add_face (model, q[0], q[1], q[3], q[2]);

    dbg (DBG_INFO, "added one cube to the model");
}

void sim_assign_and_save (model_t *model, const char *filename, const char **names, int nnames)
{
    char fullname[256];
    sprintf (fullname, "%s/sim_img/%s", DATA_DIR, filename);

    FILE *fp = fopen (fullname, "w");
    if (!fp) {
        dbg (DBG_ERROR, "Failed to open file %s in w mode", fullname);
        return;
    }

    int num = 0;

    for (int i=0;i<model->nel;i++) {
        
        // write quad
        for (int j=0;j<12;j++) {
            fprintf (fp, "%.4f ", model->quad[i][j]);
        }
        fprintf (fp, "\n");

        fprintf (fp, "%s/sim_img/%s\n", DATA_DIR, names[num]);

        // pick a face randomly
        num = (num+1)%nnames;
        
    }

    fclose (fp);

    dbg (DBG_INFO, "Saved model in %s", fullname);
}

void sim_generate_model (model_t *model)
{
    sim_model_add_cube (model, 0.0, 0.0, 0.0, 200.0);
    sim_model_add_cube (model, -40.0, -40.0, -40.0, 20.0);
    sim_model_add_cube (model, -40.0, 40.0, -40.0, 20.0);
    sim_model_add_cube (model, 0.0, 40.0, 0.0, 30.0);
    sim_model_add_cube (model, 40.0, 40.0, 20.0, 40.0);

}
 
void sim_generate_model2 (model_t *model)
{
    sim_model_add_cube (model, 0, 0, 0, 3000);
    sim_model_add_cube (model, 400, -400, 400, 300);
    sim_model_add_cube (model, 300, 400, -400, 350);
    sim_model_add_cube (model, -200, 400, 0, 355);
    sim_model_add_cube (model, 200, 400, -200, 400);
}
  
