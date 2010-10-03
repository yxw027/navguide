#include "draw.h"

#define BUFSIZE 2048
GLuint g_selectBuf[BUFSIZE];

void process_hits (GLint hits, GLuint buffer[], GLuint *type, GLuint *name);

gboolean check_gl_error (const char *t)
{
    GLenum err = glGetError ();
    switch (err) {
        case GL_NO_ERROR:
            break;
        case GL_INVALID_ENUM:
            dbg (DBG_ERROR, "%s INVALID_ENUM", t);
            break;
        case GL_INVALID_VALUE:
            dbg (DBG_ERROR, "%s INVALID_VALUE", t);
            break;
        case GL_INVALID_OPERATION:
            dbg (DBG_ERROR, "%s INVALID_OP.", t);
            break;
        case GL_STACK_OVERFLOW:
            dbg (DBG_ERROR, "%s STACK_OVERFLOW", t);
            break;
        case GL_STACK_UNDERFLOW:
            dbg (DBG_ERROR, "%s STACK_UNDERFLOW", t);
            break;
        case GL_OUT_OF_MEMORY:
            dbg (DBG_ERROR, "%s OUT_OF_MEMORY", t);
            break;
        default:
            dbg (DBG_ERROR, "%s UNKNOWN ERROR", t);
            break;
    }

    if (err != GL_NO_ERROR)
        return false;

    return true;
}

void set_background_color (state_t *self)
{
    if (!self->gladexml)
        return;

    Color bgcolor = BLACK;
    char *color = gtk_combo_box_get_active_text (GTK_COMBO_BOX (glade_xml_get_widget(self->gladexml, "background_color")));
    
    if (!color) {
        bgcolor = BLACK; 
    } else {
        if (strcmp (color, "Black") == 0)
            bgcolor = BLACK;
        if (strcmp (color, "White") == 0)
            bgcolor = WHITE;
    }
    glClearColor(bgcolor[0], bgcolor[1], bgcolor[2], 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return;

}

void start_picking (int cursorX, int cursorY, int w, int h)
{
    GLint viewport[4];
    glSelectBuffer(BUFSIZE,g_selectBuf);
    glRenderMode(GL_SELECT);
    int size = 10;

    // setup the 2D view
    glViewport(0, 0, w, h);        // reset the viewport to new dimensions 
    glMatrixMode(GL_PROJECTION);
    glPushMatrix ();
    glLoadIdentity();
    glGetIntegerv(GL_VIEWPORT,viewport);
    gluPickMatrix(cursorX,viewport[3]-cursorY, size, size, viewport);
    gluOrtho2D(0, w, h, 0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glInitNames ();

    glEnable (GL_DEPTH_TEST);
}

int stop_picking (GLuint *type, GLuint *name)
{
    int hits;
        
    // restoring the original projection matrix
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glFlush();

    // returning to normal rendering mode
    hits = glRenderMode(GL_RENDER);

    // if there are hits process them
    if (hits > 0) {
        process_hits(hits,g_selectBuf, type, name);
        return 0;
    }

    return -1;
}

void process_hits (GLint hits, GLuint buffer[], GLuint *type, GLuint *name)
{
    GLint i;
    unsigned int j;
    GLuint names=0, *ptr=NULL, minZ=0,*ptrNames=NULL, numberOfNames=0;

    if (hits == -1)
        return;

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
    ptr = ptrNames;

    *type = *ptr; ptr++;
    *name = *ptr;
}

void draw_images (botlcm_image_t **img, int nimg, double ratio, double *user_trans)
{
    for (int i=0;i<nimg;i++) {
        botlcm_image_t *im = img[i];
        if (!im) continue;
        gboolean greyscale = img[i]->pixelformat == CAM_PIXEL_FORMAT_GRAY;

        GLUtilTexture *tex = glutil_texture_new (im->width, im->height, im->row_stride * im->height);
        if (!tex) continue;
        glutil_texture_upload (tex, greyscale ? GL_LUMINANCE : GL_RGB, GL_UNSIGNED_BYTE, im->row_stride, im->data);
        int xmin = i * im->width * ratio + user_trans[0];
        int xmax = (i+1) * im->width * ratio + user_trans[0];
        int ymin = 0 + user_trans[1];
        int ymax = im->height*ratio + user_trans[1];
        glutil_texture_draw_pt (tex, xmin, ymin, xmin, ymax, xmax, ymax, xmax, ymin);
        glutil_texture_free (tex);
    }
}

/* draw selected feature as a green box
 */
void draw_select_feature (navlcm_feature_t *f, int fwidth, const float color[3], int boxsize, double ratio, double *user_trans)
{
    if (!f) return;

    double x = (f->col + fwidth*f->sensorid)*ratio + user_trans[0];
    double y = f->row * ratio + user_trans[1];

    glColor3f (color[0], color[1], color[2]);
    glLineWidth (2);

    glBegin (GL_LINE_LOOP);
    glVertex2f (x - boxsize/2, y - boxsize/2);
    glVertex2f (x + boxsize/2, y - boxsize/2);
    glVertex2f (x + boxsize/2, y + boxsize/2);
    glVertex2f (x - boxsize/2, y + boxsize/2);
    glEnd ();

}

/* draw features
*/
void draw_features (navlcm_feature_list_t *features, const float color[3], int pointsize, double ratio, double *user_trans, gboolean select, GLuint name, navlcm_feature_t *highlight_f, gboolean feature_finder_enabled)
{
    if (!features) return;

    glColor3f (color[0], color[1], color[2]);

    if (select)
        glPushName (name);

    for (int i=0;i<features->num;i++) {
        navlcm_feature_t *f = features->el + i;
        double x = (f->col + features->width*f->sensorid)*ratio + user_trans[0];
        double y = f->row * ratio + user_trans[1];
        if (select) 
            glPushName (i);
        glPointSize (pointsize);
        if (!select && f == highlight_f)
            glPointSize (2*pointsize);
        if (feature_finder_enabled)
            glPointSize (5*pointsize);
        glBegin (GL_POINTS);
        glVertex2f (x, y);
        glEnd ();
        if (select) 
            glPopName ();
    }

    if (select)
        glPopName ();
}

/* draw feature match ends
 */
void draw_match_ends (navlcm_feature_match_t *m, const float color[3], int pointsize, int iwidth, int iheight, double ratio, double *trans1, double *trans2)
{
    glColor3f (color[0], color[1], color[2]);
    glPointSize (pointsize);
    double x1 = (m->src.col + iwidth*m->src.sensorid)*ratio+trans1[0];
    double y1 = m->src.row * ratio + trans1[1];
    double x2 = (m->dst[0].col + iwidth*m->dst[0].sensorid)*ratio + trans2[0];
    double y2 = m->dst[0].row * ratio + trans2[1];
    glBegin (GL_POINTS);
    glVertex2f (x1, y1);
    glVertex2f (x2, y2);
    glEnd ();

}
void draw_match (navlcm_feature_match_t *m, const float color[3], int linewidth, int iwidth, int iheight, double ratio, double *trans1, double *trans2)
{
    glColor3f (color[0], color[1], color[2]);
    glLineWidth (linewidth);
    double x1 = (m->src.col + iwidth*m->src.sensorid)*ratio+trans1[0];
    double y1 = m->src.row * ratio + trans1[1];
    double x2 = (m->dst[0].col + iwidth*m->dst[0].sensorid)*ratio + trans2[0];
    double y2 = m->dst[0].row * ratio + trans2[1];
    glBegin (GL_LINES);
    glVertex2f (x1, y1);
    glVertex2f (x2, y2);
    glEnd ();

}

/* draw feature matches 
*/
void draw_matches (navlcm_feature_match_set_t *matches, const float color[3], int linewidth, int iwidth, int iheight, double ratio, double *trans1, double *trans2, gboolean select, navlcm_feature_match_t *highlight_m, navlcm_feature_match_t *select_m)
{
    if (!matches) return;

    glLineWidth (linewidth);

    if (select)
        glPushName (NAME_MATCH);

    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *m = matches->el + i;
        double x1 = (m->src.col + iwidth*m->src.sensorid)*ratio+trans1[0];
        double y1 = m->src.row * ratio + trans1[1];
        double x2 = (m->dst[0].col + iwidth*m->dst[0].sensorid)*ratio + trans2[0];
        double y2 = m->dst[0].row * ratio + trans2[1];
        if (select) 
            glPushName (i);
        glLineWidth (linewidth);
        if (!select && (match_equal (m, highlight_m) || match_equal (m, select_m)))
            glLineWidth (2*linewidth);
        if (match_equal (m, select_m))
            glColor3f (0,1,0);//green
        else
            glColor3f (color[0], color[1], color[2]);
        glBegin (GL_LINES);
        glVertex2f (x1, y1);
        glVertex2f (x2, y2);
        glEnd ();
        if (select)
            glPopName ();
    }

    if (select)
        glPopName ();
}

/* draw image sub-region
 */
void draw_img_subregion (botlcm_image_t **img, int nimg, int col, int row, int sensorid, float scale, float orientation, int feature_type, int wcol, int wrow)
{
    if (sensorid < 0 || sensorid > 4) {
        printf ("mmm... sensorid = %d for feature at pos %d %d (orientation: %.3f)\n", sensorid, col, row, orientation);
    }

    
    /* SURF scale is different */
    if (feature_type == NAVLCM_FEATURES_PARAM_T_SURF64 || feature_type == NAVLCM_FEATURES_PARAM_T_SURF128)
        scale *= 8;

    /* SIFT scale is different */
    if (feature_type == NAVLCM_FEATURES_PARAM_T_SIFT || feature_type == NAVLCM_FEATURES_PARAM_T_SIFT2)
       scale *= 1.414 * 3 * (4 + 1) / 2.0;
    
    /* FAST scale is different */
    if (feature_type == NAVLCM_FEATURES_PARAM_T_FAST)
        scale = 15;

    assert (0 <= sensorid && sensorid <= 4);

    botlcm_image_t *image = img[sensorid];

    assert (image);

    int size = 100;
    unsigned char *data = (unsigned char*)calloc(3*size*size,sizeof(unsigned char));

    if (sensorid == 0)
        wcol += 500;


    // fill pixels
    for (int c=0;c<size;c++) {
        for (int r=0;r<size;r++) {
            int cp = col + (int)((1.0 * c - size/2)/(size/2) * scale);
            int rp = row + (int)((1.0 * r - size/2)/(size/2) * scale);
            if (0 <= cp && cp < image->width && 0 <= rp && rp < image->height) {
                data[3*(r*size+c)] = image->data[rp*image->width + cp];
                data[3*(r*size+c)+1] = image->data[rp*image->width + cp];
                data[3*(r*size+c)+2] = image->data[rp*image->width + cp];
            }
        }
    }


    // draw orientation as a line
    for (int r=0;r<size;r++) {
        int cp = size/2 + (int)(1.0 * r - size/2) * cos (orientation);
        int rp = size/2 + (int)(1.0 * r - size/2) * sin (orientation);
        if (0 <= cp && cp < size && 0 <= rp && rp < size) {
            data[3*(rp*size+cp)] = 255;
            data[3*(rp*size+cp)+1] = 255;
            data[3*(rp*size+cp)+2] = 0;
        }
    }


    // draw red cross at the center
    for (int r=0;r<20;r++) {
        data[3*(size/2*size+size/2+r-10)] = 255;
        data[3*(size/2*size+size/2+r-10)+1] = 0;
        data[3*(size/2*size+size/2+r-10)+2] = 0;
    }
    for (int r=0;r<20;r++) {
        data[3*((size/2+r-10)*size+size/2)] = 255;
        data[3*((size/2+r-10)*size+size/2)+1] = 0;
        data[3*((size/2+r-10)*size+size/2)+2] = 0;
    }

    GLUtilTexture *tex = glutil_texture_new (size, size, 3*size*size);
    if (tex) {
        int xmin = wcol;
        int xmax = xmin+size-1;
        int ymin = wrow;
        int ymax = ymin+size-1;

        // on a red background
        glColor3f (1,0,0);
        glBegin (GL_QUADS);
        glVertex2f (xmin-4, ymin-4);
        glVertex2f (xmin-4, ymax+4);
        glVertex2f (xmax+4, ymax+4);
        glVertex2f (xmax+4, ymin-4);
        glEnd ();

        glutil_texture_upload (tex, GL_RGB, GL_UNSIGNED_BYTE, 3*size, data);
        glutil_texture_draw_pt (tex, xmin, ymin, xmin, ymax, xmax, ymax, xmax, ymin);
        glutil_texture_free (tex);
    }

    free (data);
}

/* <ww, wh> is the size of the OpenGL window
*/
void draw_frame (int ww, int wh, botlcm_image_t **img1, botlcm_image_t **img2, int nimg, navlcm_feature_list_t *features1, navlcm_feature_list_t *features2, navlcm_feature_match_set_t *matches, double user_scale, double *user_trans, gboolean feature_finder_enabled, gboolean select, navlcm_feature_t *highlight_f, navlcm_feature_match_t *highlight_m, navlcm_feature_t *match_f1, navlcm_feature_t *match_f2, navlcm_feature_match_t *select_m, navlcm_feature_match_t *match, int feature_type, double precision, double recall)
{
    if (!img1[0]) return;

    // determine scale factor
    float window_scale = .95*fmin (.25*ww/img1[0]->width, 1.0*wh/img1[0]->height);
    float ratio = user_scale* window_scale;
    double trans1[2] = {user_trans[0], user_trans[1]};
    double trans2[2] = {user_trans[0], user_trans[1] + ratio*img1[0]->height};

    if (!select) {
        // draw images
        draw_images (img1, nimg, ratio, trans1);
        draw_images (img2, nimg, ratio, trans2);
    }

    // this is the current match to be (or not to be) added to ground-truth
    if (match) {
        draw_match (match, GREEN, 2, img1[0]->width, img1[0]->height, ratio, trans1, trans2);
        draw_match_ends (match, RED, 4, img1[0]->width, img1[0]->height, ratio, trans1, trans2);
    }

    // draw features
    assert (check_gl_error ("1"));
    draw_features (features1, RED, 4, ratio, trans1, select, NAME_POINT1, highlight_f, feature_finder_enabled);
    assert (check_gl_error ("1"));
    draw_features (features2, RED, 4, ratio, trans2, select, NAME_POINT2, highlight_f, feature_finder_enabled);
    assert (check_gl_error ("1"));

    // draw matches
    draw_matches (matches, YELLOW, 2, img1[0]->width, img1[0]->height, ratio, trans1, trans2, select, highlight_m, select_m);

    // draw selected items
    if (features1 && features2) {
        draw_select_feature (match_f1, features1->width, GREEN, 12, ratio, trans1);
        draw_select_feature (match_f2, features2->width, GREEN, 12, ratio, trans2);
    }

    // draw feature thumbnails
    if (!select && select_m) {
        draw_img_subregion (img1, nimg, select_m->src.col, select_m->src.row, select_m->src.sensorid, select_m->src.scale, select_m->src.ori, feature_type, 0, 0);
        draw_img_subregion (img2, nimg, select_m->dst[0].col, select_m->dst[0].row, select_m->dst[0].sensorid, select_m->dst[0].scale, select_m->dst[0].ori, feature_type, 0, wh/2);
    } else if (!select && highlight_m) {
        draw_img_subregion (img1, nimg, highlight_m->src.col, highlight_m->src.row, highlight_m->src.sensorid, highlight_m->src.scale, highlight_m->src.ori, feature_type, 0, 0);
        draw_img_subregion (img2, nimg, highlight_m->dst[0].col, highlight_m->dst[0].row, highlight_m->dst[0].sensorid, highlight_m->dst[0].scale, highlight_m->dst[0].ori, feature_type, 0, wh/2);
    } else {
        if (!select && highlight_f) {
            draw_img_subregion (img1, nimg, highlight_f->col, highlight_f->row, highlight_f->sensorid, highlight_f->scale, highlight_f->ori, feature_type, 0, 0);
            draw_img_subregion (img2, nimg, highlight_f->col, highlight_f->row, highlight_f->sensorid, highlight_f->scale, highlight_f->ori, feature_type, 0, wh/2);
        }
        if (!select && match_f1)
            draw_img_subregion (img1, nimg, match_f1->col, match_f1->row, match_f1->sensorid, match_f1->scale, match_f1->ori, feature_type, 0, 0);
        if (!select && match_f2)
            draw_img_subregion (img2, nimg, match_f2->col, match_f2->row, match_f2->sensorid, match_f2->scale, match_f2->ori, feature_type, 0, wh/2);
    }

    assert (check_gl_error ("1"));

    if (!select) {
        // display text info
        if (matches)
            display_msg (10, wh-20, ww, wh, RED, 0, "# matches: %d", matches->num);
        if (features1 && features2)
            display_msg (200, wh-20, ww, wh, RED, 0, "# features: %d/%d", features1->num, features2->num);
        display_msg (500, wh-20, ww, wh, RED, 0, "precision: %.2f%% recall: %.2f %%", 100.0*precision, 100.0*recall);
    }
}


