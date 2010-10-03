#include "util.h"

/* compute image display ratio
 */
double util_image_ratio (int width, int height, int img_width, int img_height, gboolean compact, gboolean fit_to_window, int desired_width, 
        int nsensors, int border_width)
{
    if (!fit_to_window)
        return 1.0 * desired_width / img_width;

    if (!compact) {
        int dc = (width-2*border_width)/(nsensors);
        int dr = (height-2*border_width);
        int tw = img_width;
        int th = img_height;
        return fmin (1.0*dc/tw, 1.0*dr/th);
    } else {
        int dc = (width-2*border_width)/2;
        int dr = (height-2*border_width)/(nsensors/2);
        int tw = img_width;
        int th = img_height;
        return fmin (1.0*dc/tw, 1.0*dr/th);
    }
    return 1.0;
}

/* convert a point (col, row) from image coordinate to screen coordinate
 * c0, r0: the top left corner of the drawing area (typically, 10, 10)
 * compact_layout: a flag
 * img_width, img_height: the width and height of the image
 * ratio: the scaling ratio
*/
void util_image_to_screen (double col, double row, int sensorid, int c0, int r0, int img_width, int img_height, double ratio, gboolean compact_layout, double *ic0, double *ir0)
{
    assert (sensorid != -1);

    if (!compact_layout) {
        // a row of images
        if (ic0) *ic0 = c0 + sensorid * img_width * ratio;
        if (ir0) *ir0 = r0;
    } else {
        // a matrix of images, 2 per row
        switch (sensorid) {
            case 0:
                if (ic0) *ic0 = c0 + ratio * img_width;
                if (ir0) *ir0 = r0 + ratio * img_height;
                break;
            case 1:
                if (ic0) *ic0 = c0;
                if (ir0) *ir0 = r0;
                break;
            case 2:
                if (ic0) *ic0 = c0 + ratio * img_width;
                if (ir0) *ir0 = r0;
                break;
            case 3:
                if (ic0) *ic0 = c0;
                if (ir0) *ir0 = r0 + ratio * img_height;
                break;
            default:
                break;
        }
    }
    if (ic0) *ic0 = *ic0 + col * ratio;
    if (ir0) *ir0 = *ir0 + row * ratio;
}

static GLUtilTexture *g_texture = NULL;

GLUtilTexture *util_load_texture_from_file (char *filename)
{
    IplImage *img = cvLoadImage (filename);
    if (!img)
        return NULL;

    GLUtilTexture *texture = glutil_texture_new (img->width, img->height, img->imageSize);

    if (img->nChannels == 1) {
        glutil_texture_upload (texture, GL_LUMINANCE, GL_UNSIGNED_BYTE, img->width * img->nChannels, img->imageData);
    }
    else if (img->nChannels == 3) {
        glutil_texture_upload (texture, GL_RGB, GL_UNSIGNED_BYTE, img->widthStep, img->imageData);
    }
    else {
        fprintf (stderr, "unexpected nChannels = %d\n", img->nChannels);
    }

    printf ("Loaded %d x %d x %d texture from %s.\n", img->width, img->height, img->nChannels, filename);

    cvReleaseImage (&img);

    return texture;
}

void util_draw_image (botlcm_image_t *img, int idx, int window_width, int window_height, gboolean compact,
        gboolean fit_to_window, int desired_width, int nsensors, int c0, int r0, int border_width)
{
    double ratio = util_image_ratio (window_width, window_height, img->width, img->height, 
            compact, fit_to_window, desired_width, nsensors, border_width);
    
    double dw = ratio * img->width;
    double dh = ratio * img->height;

    double irow, icol;
    util_image_to_screen (c0, r0, idx, border_width, border_width, img->width, img->height, ratio, 
            compact, &icol, &irow);

    if (!g_texture) 
        g_texture = glutil_texture_new (img->width, img->height, img->row_stride * img->height);

    glutil_texture_upload (g_texture, GL_LUMINANCE, GL_UNSIGNED_BYTE, img->row_stride, img->data);

    glutil_texture_draw_pt (g_texture, icol, irow, icol, 
            irow+dh, icol+dw, irow+dh, icol+dw, irow);

}

void util_draw_flow (navlcm_flow_t *flow, int window_width, int window_height, int image_width, int image_height, double flow_scale,
        gboolean compact, gboolean fit_to_window, int desired_width, int nsensors, int c0, int r0, int border_width, int line_width)
{
    if (image_width == 0 || image_height == 0)
        return;

    double ratio = util_image_ratio (window_width, window_height, image_width, image_height, 
            compact, fit_to_window, desired_width, nsensors, border_width);

    for (int i=0;i<flow->num;i++) {
        double src[2], dst[2];
        util_image_to_screen (c0 + flow->x[i] * image_width, r0 + flow->y[i] * image_height, flow->sensor[i], border_width, border_width, 
                image_width, image_height, ratio, compact, &src[0], &src[1]);
        util_image_to_screen (c0 + (flow->x[i] + flow->flowx[i]*flow_scale) * image_width, r0 + (flow->y[i] + flow->flowy[i]*flow_scale) * image_height, flow->sensor[i], border_width, 
                border_width, image_width, image_height, ratio, compact, &dst[0], &dst[1]);

        if (flow->status[i]) {
            glColor3f(1,0,0);
            double arrow_length = bot_vector_dist_2d (src, dst);
            double head_width = 6;
            double head_length = head_width*1.5;
            double body_width = 2;
            gboolean fill = TRUE;

            glPushMatrix ();
            glTranslatef ((src[0]+dst[0])/2, (src[1]+dst[1])/2, .0);
            glRotatef(to_degrees (atan2(dst[1]-src[1], dst[0]-src[0])), 0, 0, 1);
            bot_gl_draw_arrow_2d (arrow_length, head_width, head_length, body_width, fill);
            glPopMatrix ();

        } else {
            glColor3f(0,0,1);
            glPointSize (2*line_width);
            glBegin (GL_POINTS);
            glVertex2f (src[0], src[1]);
            glEnd();
        }
    }

}
    void
util_draw_disk (double x, double y, double r_in, double r_out)
{
    double xyz[3] = {x, y, .0};
    GLUquadric* q = gluNewQuadric ();

    glPushMatrix();
    glTranslatef(xyz[0], xyz[1], xyz[2]);
    gluDisk(q, r_in, r_out, 90, 1);
    glPopMatrix();

    gluDeleteQuadric (q);
}

    void
util_draw_circle (double x, double y, double r)
{
    glPushMatrix ();
    glTranslatef (x, y, .0);
    bot_gl_draw_circle (r);
    glPopMatrix ();
}

void util_draw_features (navlcm_feature_list_t *s, int window_width, int window_height, gboolean compact,
        gboolean fit_to_window, int desired_width, int nsensors, int c0, int r0, int border_width, int pointsize, 
        gboolean sensor_color, gboolean sensor_color_box, gboolean show_scale)
{
    double ratio = util_image_ratio (window_width, window_height, s->width, s->height, 
            compact, fit_to_window, desired_width, nsensors, border_width);

    double maxscale = .0;
    glPointSize(pointsize);
    glBegin (GL_POINTS);

    for (int i=0;i<s->num;i++) {

        navlcm_feature_t *f = s->el + i;

        double irow, icol;
        util_image_to_screen (c0 + f->col, r0 + f->row, f->sensorid, border_width, border_width, s->width, s->height, ratio, 
                compact, &icol, &irow);

        maxscale = fmax (maxscale, f->scale);

        if (sensor_color && f->uid == 0)
            continue;

        if (sensor_color)
            glColor3f (_colors[f->uid % NCOLORS][0],_colors[f->uid % NCOLORS][1],_colors[f->uid % NCOLORS][2]);
        else
            glColor3f (1,1,0);


        glVertex2f (icol, irow);

    }

    glEnd ();

    // draw color box around each image
    if (sensor_color_box) {
        for (int i=0;i<nsensors;i++) {
            double icoltl, irowtl, icolbr, irowbr;
            glColor3f (_colors[(i+1) % NCOLORS][0],_colors[(i+1) % NCOLORS][1],_colors[(i+1) % NCOLORS][2]);
            util_image_to_screen (c0, r0, i, border_width, border_width, s->width, s->height, ratio, 
                    compact, &icoltl, &irowtl);
            util_image_to_screen (c0 + s->width-1, r0 + s->height-1, i, border_width, border_width, s->width, s->height, ratio, 
                    compact, &icolbr, &irowbr);
            glLineWidth(2);
            glBegin (GL_LINE_LOOP);
            glVertex2f(icoltl, irowtl);
            glVertex2f(icoltl, irowbr);
            glVertex2f(icolbr, irowbr);
            glVertex2f(icolbr, irowtl);
            glEnd();

        }
    }

    if (show_scale) {
        glLineWidth(1);
        for (int i=0;i<s->num;i++) {

            navlcm_feature_t *f = s->el + i;

            double irow, icol;
            util_image_to_screen (c0 + f->col, r0 + f->row, f->sensorid, border_width, border_width, s->width, s->height, ratio, 
                    compact, &icol, &irow);

            int color = 0;
            if (maxscale > 1E-3) {
                color = MIN (int(NCOLORS*f->scale / maxscale), NCOLORS-1);
                glColor3f (_colors[color % NCOLORS][0],_colors[color % NCOLORS][1],_colors[color % NCOLORS][2]);
                double pixel_scale = f->scale * 1.414 * 3 * (4 + 1) / 2.0;  // from SIFT code
                util_draw_circle (icol, irow, f->scale); 
            }
        }
    }

}
