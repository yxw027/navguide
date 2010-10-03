#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <lcm/lcm.h>

#include <bot/bot_core.h>
#include <bot/gtk/gtk_util.h>
#include <bot/viewer/viewer.h>
#include <bot/gl/gl_util.h>

#include <common/color_util.h>
#include <common/globals.h>
#include <common/navconf.h>
#include <common/gltool.h>
#include <common/codes.h>
#include <common/mathutil.h>

#include <lcmtypes/navlcm_class_param_t.h>
#include <lcmtypes/navlcm_ui_map_t.h>

#include "util.h"

#include <guidance/dijkstra.h>
#include <guidance/corrmat.h>
#include <guidance/classifier.h>

#define RENDERER_NAME "Guidance"
#define PARAM_SHOW_NODE_IMAGES "Node images"
#define PARAM_SHOW_NODE_FEATURES "Node features"
#define PARAM_SHOW_SIMILARITY_MATRIX "Similarity matrix"
#define PARAM_SHOW_MOTION_TYPE "Motion class."
#define PARAM_SHOW_COMPASS "Compass"
#define PARAM_SHOW_TEXT_INFO "Text info"
#define PARAM_SHOW_EXTRA_NAV "Extra nav."
#define PARAM_SHOW_LOC_PDF "Loc. PDF"
#define PARAM_SHOW_UI_MAP "UI Map"
#define PARAM_SHOW_UI_MAP_LOCAL "UI Map (local)"
#define PARAM_SHOW_CLASS_HISTOGRAMS "Class. histograms"

#define PARAM_NAME_SIM_MATRIX_SIZE "Sim. matrix size"
#define PARAM_NAME_UI_MAP_SIZE "UI Map size"
#define PARAM_NAME_UI_MAP_DECIMATE "Decimate UI Map"
#define PARAM_NAME_CLASS_HISTOGRAMS_SIZE "Histogram size"
#define PARAM_IMAGE_DESIRED_WIDTH "Image width"
#define PARAM_NAME_MAP_ZOOM "UI Map Zoom"

#define RENDERER_GUIDANCE_LOAD_MAP "Load map"
#define RENDERER_GUIDANCE_SET_REFERENCE "Set reference"

typedef struct _RendererGuidance {
    Renderer renderer;

    BotConf *config;
    lcm_t *lcm;
    Viewer *viewer;
    BotGtkParamWidget *pw;
    char *dirname;

    navlcm_class_param_t *navguide_state;
    int future_direction[3];

    dijk_graph_t *dg;
    double *similarity_matrix;
    int similarity_matrix_size;
    GHashTable *class_hist;
    GLUtilTexture **textures;

    navlcm_ui_map_t *ui_map;

} RendererGuidance;

static void my_free( Renderer *renderer )
{
    RendererGuidance *self = (RendererGuidance*) renderer->user;

    free( self );
}

void load_icons (RendererGuidance *self)
{
    int num_textures = MAX (NAVLCM_CLASS_PARAM_T_MOTION_TYPE_FORWARD,
            MAX (NAVLCM_CLASS_PARAM_T_MOTION_TYPE_LEFT,
                MAX (NAVLCM_CLASS_PARAM_T_MOTION_TYPE_RIGHT,
                    MAX (NAVLCM_CLASS_PARAM_T_MOTION_TYPE_UP,
                        NAVLCM_CLASS_PARAM_T_MOTION_TYPE_DOWN))));

    self->textures = (GLUtilTexture**)calloc(num_textures, sizeof(GLUtilTexture*));

    char filename[256];
    sprintf (filename, "%s/img/icon-down-100x100.png", CONFIG_DIR);
    self->textures[NAVLCM_CLASS_PARAM_T_MOTION_TYPE_DOWN] = util_load_texture_from_file (filename);
    sprintf (filename, "%s/img/icon-up-100x100.png", CONFIG_DIR);
    self->textures[NAVLCM_CLASS_PARAM_T_MOTION_TYPE_UP] = util_load_texture_from_file (filename);
    sprintf (filename, "%s/img/icon-left-turn-100x100.png", CONFIG_DIR);
    self->textures[NAVLCM_CLASS_PARAM_T_MOTION_TYPE_LEFT] = util_load_texture_from_file (filename);
    sprintf (filename, "%s/img/icon-right-turn-100x100.png", CONFIG_DIR);
    self->textures[NAVLCM_CLASS_PARAM_T_MOTION_TYPE_RIGHT] = util_load_texture_from_file (filename);
    sprintf (filename, "%s/img/icon-straight-100x100.png", CONFIG_DIR);
    self->textures[NAVLCM_CLASS_PARAM_T_MOTION_TYPE_FORWARD] = util_load_texture_from_file (filename);


}

void render_quad (double x1, double y1, double x2, double y2)
{
    glBegin (GL_QUADS);
    glVertex2f (x1, y1);
    glVertex2f (x1, y2);
    glVertex2f (x2, y2);
    glVertex2f (x2, y1);
    glEnd();

}

void render_box (double x1, double y1, double x2, double y2)
{
    glBegin (GL_LINE_LOOP);
    glVertex2f (x1, y1);
    glVertex2f (x1, y2);
    glVertex2f (x2, y2);
    glVertex2f (x2, y1);
    glEnd();

}

void render_wrong_way (gboolean wrong_way, int width, int height)
{
    if (!wrong_way)
        return;

    glColor3f (1.0, 0, 0);

    int radius = 50;
    int c0 = 7*radius, r0 = 420+1.5*radius;
    
    util_draw_disk (c0, r0, .0, radius);

    glColor3f (1,1,1);
    glBegin (GL_QUADS);
    glVertex2f (c0-.7*radius, r0-.20*radius);
    glVertex2f (c0+.7*radius, r0-.20*radius);
    glVertex2f (c0+.7*radius, r0+.20*radius);
    glVertex2f (c0-.7*radius, r0+.20*radius);
    glEnd ();

    double pos[3] = {c0-36, r0+13, 0};

    glColor3f (1,0,0);

    bot_gl_draw_text(pos, GLUT_BITMAP_8_BY_13 /*GLUT_BITMAP_HELVETICA_12*/, "WRONG WAY",
            BOT_GL_DRAW_TEXT_JUSTIFY_CENTER |
            BOT_GL_DRAW_TEXT_ANCHOR_TOP |
            BOT_GL_DRAW_TEXT_ANCHOR_LEFT);

}

void render_future_directions (RendererGuidance *self, int *directions, int num, int c0, int r0)
{
    int dw = 80;
    int dh = dw;
    int icol = c0;
    int irow = r0;

    for (int i=0;i<num;i++) {
        if (directions[i] != -1) {
            if (self->textures[directions[i]]) {
                glutil_texture_draw_pt (self->textures[directions[i]], icol, irow, icol, 
                        irow+dh, icol+dw, irow+dh, icol+dw, irow);
                icol += 1.5 * dw;
            }
        }
    }
}

void render_motion_type (navlcm_class_param_t *param, int center_x, int center_y, int image_width, int image_height, 
        int width, int height)
{
    if (!param)
        return;

    int radius = 50;
    int c0 = radius, r0 = 500+1.5*radius;

    glColor3f (.8, .8, .8);
    glLineWidth (2);

    util_draw_disk (c0, r0, .0, radius);

    glColor3f (0,0,0);
    util_draw_circle (c0, r0, radius);

    // draw a colored circle for each motion type
    int nradius = radius/7;

    // forward
    int c= c0, r = r0;
    glColor3f (1,0,1);
    if (param->motion_type[0] == NAVLCM_CLASS_PARAM_T_MOTION_TYPE_FORWARD) {
        util_draw_disk (c, r, .0, nradius);
        util_draw_disk (center_x, center_y, .0, radius/2);
    }
    else
        util_draw_circle (c, r, nradius);

    // left
    c = c0 - .7 * radius;
    r = r0;
    glColor3f (1,0,0);
    if (param->motion_type[0] == NAVLCM_CLASS_PARAM_T_MOTION_TYPE_LEFT) {
        util_draw_disk (c, r, .0, nradius);
        util_draw_disk (center_x-image_width, center_y, .0, radius/2);
    }
    else
        util_draw_circle (c, r, nradius);

    // right
    c = c0 + .7 * radius;
    r = r0;
    glColor3f (0,1,0);
    if (param->motion_type[0] == NAVLCM_CLASS_PARAM_T_MOTION_TYPE_RIGHT) {
        util_draw_disk (c, r, .0, nradius);
        util_draw_disk (center_x+image_width, center_y, .0, radius/2);
    }
    else
        util_draw_circle (c, r, nradius);

    // up
    c = c0;
    r = r0 - .7 * radius;
    glColor3f (1,1,0);
    if (param->motion_type[0] == NAVLCM_CLASS_PARAM_T_MOTION_TYPE_UP) {
        util_draw_disk (c, r, .0, nradius);
        util_draw_disk (center_x, center_y-image_height, .0, radius/2);
    }
    else
        util_draw_circle (c, r, nradius);

    // down
    c = c0;
    r = r0 + .7 * radius;
    glColor3f (0,1,1);
    if (param->motion_type[0] == NAVLCM_CLASS_PARAM_T_MOTION_TYPE_DOWN) {
        util_draw_disk (c, r, .0, nradius);
        util_draw_disk (center_x, center_y+image_height, .0, radius/2);
    }
    else
        util_draw_circle (c, r, nradius);


}

void render_pdf_plot (navlcm_class_param_t *param, int map_width, int width, int height)
{
    if (!param) return;

    int pwidth = map_width;
    int pheight = int (.75 * map_width)-10;
    int c0 = map_width + 20;
    int r0 = 280;

    // axis
    glColor3f(1,1,1);
    glBegin (GL_LINES);
    glVertex2f (c0, r0+pheight);
    glVertex2f (c0+pwidth, r0+pheight);
    glVertex2f (c0+pwidth, r0+pheight);
    glVertex2f (c0+pwidth-5, r0+pheight-5);
    glVertex2f (c0+pwidth, r0+pheight);
    glVertex2f (c0+pwidth-5, r0+pheight+5);
    glVertex2f (c0, r0);
    glVertex2f (c0, r0+pheight);
    glVertex2f (c0, r0);
    glVertex2f (c0-5, r0+5);
    glVertex2f (c0, r0);
    glVertex2f (c0+5, r0+5);
    glEnd();

    // find maxima
    int maxima_ind = -1;
    double maxima_val = .0;
    for (int i=0;i<param->pdf_size;i++) {
        if (maxima_ind == -1 || maxima_val < param->pdfval[i]) {
            maxima_ind = param->pdfind[i];
            maxima_val = param->pdfval[i];
        }
    }

    int minindex = MAX(0, maxima_ind -10);
    minindex = 10 * int (1.0 * minindex / 10);
    int maxindex = minindex + 20;
    double minval = .0, maxval = maxima_val;

    if (minindex == -1 || maxindex == -1 || maxindex == minindex)
        return;
    if (fabs (maxval-minval)<1E-6)
        return;

    double xstep = 1.0 * pwidth / (maxindex - minindex);
    double ystep = 1.0 * pheight / (maxval - minval);

    // sort data by increasing index values
    GQueue *data = g_queue_new ();
    for (int i=0;i<param->pdf_size;i++) {
        if (param->pdfind[i] < minindex || maxindex < param->pdfind[i]) 
            continue;
        pair_t *p = (pair_t*)malloc(sizeof(pair_t));
        p->key = param->pdfind[i];
        p->val = param->pdfval[i];
        g_queue_push_tail (data, p);
    }
    g_queue_sort (data, pair_comp_key, NULL);

    // draw
    glColor3f (1,0,0);
    glBegin (GL_LINE_STRIP);
    for (GList *iter=g_queue_peek_head_link (data);iter;iter=iter->next) {
        pair_t *p = (pair_t*)iter->data;
        double x = c0 + xstep * (p->key-minindex);
        double y = r0 + pheight - ystep * (p->val-minval);
        glVertex2f (x, y);

    }
    glEnd();

    // plot title
    double pos[3] = {c0+10, r0, 0};
    bot_gl_draw_text(pos, GLUT_BITMAP_8_BY_13 /*GLUT_BITMAP_HELVETICA_12*/, "Node est. PDF",
            BOT_GL_DRAW_TEXT_JUSTIFY_CENTER |
            BOT_GL_DRAW_TEXT_ANCHOR_TOP |
            BOT_GL_DRAW_TEXT_ANCHOR_LEFT);

    // ticks
    glColor3f(1,1,1);
    for (GList *iter=g_queue_peek_head_link (data);iter;iter=iter->next) {
        pair_t *p = (pair_t*)iter->data;
        if (p->key % 2 != 0)
            continue;
        double x = c0 + xstep * (p->key-minindex);
        double y1 = r0 + pheight;
        double y2 = r0 + pheight + 5;
        glBegin (GL_LINES);
        glVertex2f (x, y1);
        glVertex2f (x, y2);
        glEnd ();
        double pos_tick[3] = {x, y1, 0};
        char tick_label[10];
        sprintf (tick_label, "%d", p->key);
        bot_gl_draw_text(pos_tick, GLUT_BITMAP_8_BY_13, tick_label, 
                BOT_GL_DRAW_TEXT_JUSTIFY_CENTER |
                BOT_GL_DRAW_TEXT_ANCHOR_TOP |
                BOT_GL_DRAW_TEXT_ANCHOR_LEFT);
    }

    // free data
    for (GList *iter=g_queue_peek_head_link (data);iter;iter=iter->next) {
        pair_t *p = (pair_t*)iter->data;
        free (p);
    }
    g_queue_free (data);

}

void render_text_info (navlcm_class_param_t *param, int width, int height)
{
    if (!param) return;

    int c0 = 400;
    int r0 = 10+240+10;
    double pos[3] = {c0, r0};
    int textheight = 20;

    glColor3f (1,0,0);

    char text[256];
    sprintf (text, "Node: %d", param->nodeid_now);

    bot_gl_draw_text(pos, GLUT_BITMAP_8_BY_13 /*GLUT_BITMAP_HELVETICA_12*/, text,
            BOT_GL_DRAW_TEXT_JUSTIFY_CENTER |
            BOT_GL_DRAW_TEXT_ANCHOR_TOP |
            BOT_GL_DRAW_TEXT_ANCHOR_LEFT);
    pos[1] += textheight;

    sprintf (text, "Rot.guidance: %.1f Hz", 1000000.0 / (int)param->rotation_guidance_period);
    bot_gl_draw_text(pos, GLUT_BITMAP_8_BY_13 /*GLUT_BITMAP_HELVETICA_12*/, text,
            BOT_GL_DRAW_TEXT_JUSTIFY_CENTER |
            BOT_GL_DRAW_TEXT_ANCHOR_TOP |
            BOT_GL_DRAW_TEXT_ANCHOR_LEFT);
    pos[1] += textheight;

    sprintf (text, "Node estimation: %.1f Hz", 1000000.0 / (int)param->node_estimation_period);
    bot_gl_draw_text(pos, GLUT_BITMAP_8_BY_13 /*GLUT_BITMAP_HELVETICA_12*/, text,
            BOT_GL_DRAW_TEXT_JUSTIFY_CENTER |
            BOT_GL_DRAW_TEXT_ANCHOR_TOP |
            BOT_GL_DRAW_TEXT_ANCHOR_LEFT);
    pos[1] += textheight;

    sprintf (text, "Confidence: %.5f", param->confidence);
    bot_gl_draw_text(pos, GLUT_BITMAP_8_BY_13 /*GLUT_BITMAP_HELVETICA_12*/, text,
            BOT_GL_DRAW_TEXT_JUSTIFY_CENTER |
            BOT_GL_DRAW_TEXT_ANCHOR_TOP |
            BOT_GL_DRAW_TEXT_ANCHOR_LEFT);
    pos[1] += textheight;

    sprintf (text, "Psi distance: %.5f", param->psi_distance);
    bot_gl_draw_text(pos, GLUT_BITMAP_8_BY_13 /*GLUT_BITMAP_HELVETICA_12*/, text,
            BOT_GL_DRAW_TEXT_JUSTIFY_CENTER |
            BOT_GL_DRAW_TEXT_ANCHOR_TOP |
            BOT_GL_DRAW_TEXT_ANCHOR_LEFT);
    pos[1] += textheight;


}

void render_compass (navlcm_class_param_t *param, int c0, int r0, int width, int height)
{
    if (!param) return;

    // visual compass
    int radius = 80;

    glColor3f (1,1,1);
    glLineWidth (2);

    glPushMatrix ();
    glTranslatef (c0, r0, .0);
    bot_gl_draw_circle (radius);
    glPopMatrix ();

    glLineWidth (1);
    // draw segment every 10 degrees
    double a = 0.0;
    int num = 36;
    glBegin (GL_LINES);
    for (int i=0;i<num;i++) {
        double c1 = c0 + .95 * radius * cos(a);
        double r1 = r0 + .95 * radius * sin(a);
        double c2 = c0 + radius * cos(a);
        double r2 = r0 + radius * sin(a);
        glVertex2f(c1,r1);
        glVertex2f(c2,r2);
        a += 2*PI/num;
    }
    glEnd();

    // draw an arrow
    // angle 0 pointing upward
    // draw -angle because coord frame is upside down
    //glColor3f (.3,0,0);
    //draw_arrow (c0, r0, radius, - (angle[1] + PI/2)); 
    glColor (RED);
    draw_arrow (c0, r0, radius, - (param->orientation[0] + PI/2)); 

}

static void render_node_images (navlcm_class_param_t *state, dijk_graph_t *dg, int desired_width,
        int window_width, int window_height, int nsensors, gboolean draw_images, gboolean draw_features)
{
    if (!state) return;


    if (draw_images) {
        if (state) {
            dijk_edge_t *e = dijk_graph_find_edge_by_id (dg, state->nodeid_now,
                    state->nodeid_next);
            if (e) {
                int idx=0;
                for (GList *iter=g_queue_peek_head_link (e->img);iter;iter=iter->next) {
                    botlcm_image_t *img = (botlcm_image_t*)iter->data;
                    int c0 = 2 * img->width + 10;
                    int r0 = 0;

                    util_draw_image (img, idx, window_width, window_height, TRUE, FALSE, desired_width, nsensors, c0, r0, 10);

                    if (draw_features) {
                        util_draw_features (e->features, window_width, window_height, TRUE, FALSE,
                                desired_width, nsensors, c0, r0, 10, 3, FALSE, FALSE, FALSE);
                    }
                    idx++;
                }
            }
        }
    }
}

navlcm_ui_map_node_t *find_ui_map_node_by_id (navlcm_ui_map_t *map, int id)
{
    if (!map) return NULL;

    for (int i=0;i<map->n_nodes;i++) {
        if (map->nodes[i].id == id)
            return map->nodes + i;
    }

    return NULL;
}

double find_weight_by_node_id (navlcm_class_param_t *param, int id)
{
    double weight = 1.0;
    if (!param)
        return weight;

    for (int i=0;i<param->pdf_size;i++) {
        if (param->pdfind[i] == id)
            return param->pdfval[i];
    }

    return weight;
}

/* render only a local part of the map, centered on the user's location
*/
static void render_ui_map_local (navlcm_ui_map_t *map, navlcm_class_param_t *param, double zoom, int map_width, int decimate, int width, int height)
{
    if (!map) return;

    int x = 10;
    int y = 280;
    int map_height = int(.75 * map_width);
    int node_radius = MAX (4, int (.3/zoom));
    int border = 10;

    // compute the centroid of all nodes, using the PDF as a weight
    double center_x = .0, center_y = .0;
    double total_weight = .0;
    for (int i=0;i<map->n_nodes;i++) {
        double weight = find_weight_by_node_id (param, map->nodes[i].id);
        center_x += weight * map->nodes[i].x;
        center_y += weight * map->nodes[i].y;
        total_weight += weight;
    }
    if (fabs (total_weight) > 1E-6) {
        center_x /= total_weight;
        center_y /= total_weight;
    }

    // draw edges
    glColor3f (1,1,1);
    for (int i=0;i<map->n_edges;i++) {
        navlcm_ui_map_node_t *nd1 = find_ui_map_node_by_id (map, map->edges[i].id1);
        navlcm_ui_map_node_t *nd2 = find_ui_map_node_by_id (map, map->edges[i].id2);
        if (nd1 && nd2) {
            double xn1 = x + map_width/2 + (nd1->x - center_x) / zoom * map_width / 2;
            double yn1 = y + map_height/2 + (nd1->y - center_y) / zoom * map_height / 2;
            double xn2 = x + map_width/2 + (nd2->x - center_x) / zoom * map_width / 2;
            double yn2 = y + map_height/2 + (nd2->y - center_y) / zoom * map_height / 2;
            if (xn1 < x + border || xn1 > x + map_width - border) continue;
            if (yn1 < y + border || yn1 > y + map_height - border) continue;
            if (xn2 < x + border || xn2 > x + map_width - border) continue;
            if (yn2 < y + border || yn2 > y + map_height - border) continue;
            glBegin (GL_LINES);
            glVertex2f (xn1, yn1);
            glVertex2f (xn2, yn2);
            glEnd ();
        }
    }

    // draw nodes
    for (int i=0;i<map->n_nodes;i++) {
        double xn = x + map_width/2 + (map->nodes[i].x - center_x) / zoom * map_width / 2;
        double yn = y + map_height/2 + (map->nodes[i].y - center_y) / zoom * map_height / 2;
        if (xn < x + border || xn > x + map_width - border) continue;
        if (yn < y + border || yn > y + map_height - border) continue;
        double weight = find_weight_by_node_id (param, map->nodes[i].id);
        glColor3f (weight, 0, 0);
        util_draw_disk (xn, yn, .0, node_radius);
        glColor3f (1,1,1);
        util_draw_circle (xn, yn, node_radius);
        double pos[3] = {xn + node_radius, yn + node_radius/2, 0};
        char node_name[20];
        sprintf (node_name, "%d", map->nodes[i].id);
        glColor3f (1,0,0);
        bot_gl_draw_text(pos, GLUT_BITMAP_8_BY_13, node_name,
                BOT_GL_DRAW_TEXT_JUSTIFY_CENTER |
                BOT_GL_DRAW_TEXT_ANCHOR_TOP |
                BOT_GL_DRAW_TEXT_ANCHOR_LEFT);
    }

    // frame box
    glColor3f (1,1,1);
    glBegin (GL_LINE_LOOP);
    glVertex2f (x,y);
    glVertex2f (x+map_width,y);
    glVertex2f (x+map_width,y+map_height);
    glVertex2f (x,y+map_height);
    glEnd();
}

/* renders the whole map 
*/
static void render_ui_map (navlcm_ui_map_t *map, int map_width, int decimate, int width, int height)
{
    if (!map) return;

    int x = 10;
    int y = 280;
    int map_height = .75 * map_width;
    int node_radius = 4;
    double scale = .9;
    int offset = 10;
    int64_t last_utime = 0;
    double cnx, cny;

    // find position of latest node
    for (int i=0;i<map->n_nodes;i++) {
        double nx = offset + scale * map->nodes[i].x * map_width + x;
        double ny = offset + scale * map->nodes[i].y * map_height + y;
        if (map->nodes[i].utime >= last_utime) {
            last_utime = map->nodes[i].utime;
            cnx = nx;
            cny = ny;
        }
    }

    // draw edges as lines
    glColor3f (1,1,1);
    for (int i=0;i<map->n_edges;i++) {
        navlcm_ui_map_node_t *nd1 = find_ui_map_node_by_id (map, map->edges[i].id1);
        navlcm_ui_map_node_t *nd2 = find_ui_map_node_by_id (map, map->edges[i].id2);
        if (nd1 && nd2) {
            double nx1 = offset + scale * nd1->x * map_width + x;
            double ny1 = offset + scale * nd1->y * map_height + y;
            double nx2 = offset + scale * nd2->x * map_width + x;
            double ny2 = offset + scale * nd2->y * map_height + y;
            glBegin (GL_LINES);
            glVertex2f (nx1, ny1);
            glVertex2f (nx2, ny2);
            glEnd ();
        }
    }

    // frame box
    glColor3f (1,1,1);
    glBegin (GL_LINE_LOOP);
    glVertex2f (x,y);
    glVertex2f (x+map_width,y);
    glVertex2f (x+map_width,y+map_height);
    glVertex2f (x,y+map_height);
    glEnd();

    // draw red dot at current location
    glColor3f (1,0,0);
    glPointSize (10);
    glBegin (GL_POINTS);
    glVertex2f (cnx, cny);
    glEnd ();

}

/* (x0, y0) is the top left corner of the histogram
*/
static void render_histogram (navlcm_dictionary_t *d, int sensori, int sensorj, int x0, int y0, int rendering_width, 
        int rendering_height, gboolean details)
{
    if (!d) return;

    int border=10;
    int xl = x0 + border;
    int xr = x0 + rendering_width - border;
    int yt = y0 + border;
    int yb = y0 + rendering_height - border;
    double bin_width = 1.0 / d->num * (xr-xl);
    double maxval = 1.0;

    glColor3f(1,1,1);
    for (int i=0;i<d->num;i++) {
        double val;
        sscanf (d->vals[i], "%lf", &val);
        val = fmin (maxval, val);
        double x1 = xl + (1.0*i) * bin_width;
        double x2 = x1 + bin_width;
        double y1 = yb;
        double y2 = yb - val * (yb-yt);
        render_quad (x1, y1, x2, y2);
    }

    glColor3f (1,1,1);
    render_box (xl, yb, xr, yt);

    char text[128];
    sprintf (text, "%d - %d", sensori, sensorj);
    double pos[3] = {xl + 10, yt, 0};
    bot_gl_draw_text(pos, GLUT_BITMAP_HELVETICA_12, text,
            BOT_GL_DRAW_TEXT_JUSTIFY_CENTER |
            BOT_GL_DRAW_TEXT_ANCHOR_TOP |
            BOT_GL_DRAW_TEXT_ANCHOR_LEFT |
            BOT_GL_DRAW_TEXT_DROP_SHADOW);

    if (details) {
        // ticks
        for (int i=-180;i<=180;i+=45) {
            double xx = xl + (i+180)/360.0 * (xr-xl);
            double yy = yb;
            glBegin (GL_LINES);
            glVertex2f (xx, yy);
            glVertex2f (xx, yy+10);
            glEnd();
            sprintf (text, "%d", i);
            double pos[3] = {xx, yy+20, 0};
            bot_gl_draw_text(pos, GLUT_BITMAP_HELVETICA_12, text,
                    BOT_GL_DRAW_TEXT_JUSTIFY_CENTER |
                    BOT_GL_DRAW_TEXT_ANCHOR_TOP |
                    BOT_GL_DRAW_TEXT_ANCHOR_LEFT |
                    BOT_GL_DRAW_TEXT_DROP_SHADOW);

        }
    }

}

static void render_histograms (GHashTable *hist, int rendering_width, int x0, int y0, int nsensors, int width, int height)
{
    if (!hist) return;

    GHashTableIter iter;
    gpointer key, value;
    int hist_rendering_width = rendering_width / nsensors;
    navlcm_dictionary_t *dd=NULL;

    g_hash_table_iter_init (&iter, hist);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
        char *channel = (char*)key;
        navlcm_dictionary_t *d = (navlcm_dictionary_t*)value;
        int sensori, sensorj;
        if (sscanf (channel, "CLASS_HIST_%d_%d", &sensori, &sensorj) != 2)
            fprintf (stderr, "failed to parse channel name %s\n", channel);
        int x = x0 + sensorj * hist_rendering_width;
        int y = y0 + sensori * hist_rendering_width;
        render_histogram (d, sensori, sensorj, x, y, hist_rendering_width, hist_rendering_width, FALSE);
        if (sensori==0 && sensorj==1)
            dd = d;
    }

    if (dd)
        render_histogram (dd, 0, 1, rendering_width + 10, rendering_width/2+10, 
                int(rendering_width), int(rendering_width/2-20), TRUE);
}

static void render_similarity_matrix (double *mat, int size, int rendering_size, int width, int height)
{
    if (!mat || size==0) return;

    int cellsize = MAX((int)(1.0 * rendering_size / size),2);
    int x = 400;
    int y = 10;

    double maxval = corrmat_max (mat, size);
    double minval = corrmat_min (mat, size);

    for (int i=0;i<size;i++) {
        int ry = y + i*cellsize;
        int rx = x;
        for (int j=0;j<size;j++) {
            double val = CORRMAT_GET (mat, size, size-1-i, j);
            val = 1.0-(val-minval)/(maxval-minval);
            glColor3f (val, val, val);
            glBegin (GL_QUADS);
            glVertex2f(rx,ry);
            glVertex2f(rx+cellsize,ry);
            glVertex2f(rx+cellsize, ry+cellsize);
            glVertex2f(rx,ry+cellsize);
            glEnd();
            rx += cellsize;
        }
    }
}

static void my_draw( Viewer *viewer, Renderer *renderer )
{
    RendererGuidance *self = (RendererGuidance*) renderer->user;

    int nsensors;
    navconf_get_nsensors (self->config, &nsensors);

    // transform into window coordinates, where <0, 0> is the top left corner
    // of the window and <viewport[2], viewport[3]> is the bottom right corner
    // of the window
    GLint viewport[4];
    glGetIntegerv (GL_VIEWPORT, viewport);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, viewport[2], 0, viewport[3]);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0, viewport[3], 0);
    glScalef (1, -1, 1);

    int desired_width = bot_gtk_param_widget_get_int (self->pw, PARAM_IMAGE_DESIRED_WIDTH);
    int desired_height = (int)(1.0 * desired_width * 240 / 376);
    int center_x = desired_width + 10;
    int center_y = int (1.0 * 240 / 376 * desired_width + 10);

    int map_width = bot_gtk_param_widget_get_int (self->pw, PARAM_NAME_UI_MAP_SIZE);
    int map_decimate = bot_gtk_param_widget_get_int (self->pw, PARAM_NAME_UI_MAP_DECIMATE);
    double map_zoom = bot_gtk_param_widget_get_double (self->pw, PARAM_NAME_MAP_ZOOM);

    if (bot_gtk_param_widget_get_bool(self->pw, PARAM_SHOW_MOTION_TYPE))
        render_motion_type (self->navguide_state, center_x, center_y, desired_width, desired_height, viewport[2], viewport[3]);
    
    if (bot_gtk_param_widget_get_bool(self->pw, PARAM_SHOW_MOTION_TYPE))
        render_future_directions (self, self->future_direction, 3, 10, 2 * desired_height);

    if (bot_gtk_param_widget_get_bool(self->pw, PARAM_SHOW_EXTRA_NAV))
        render_wrong_way (self->navguide_state->wrong_way, viewport[2], viewport[3]);

    if (bot_gtk_param_widget_get_bool(self->pw, PARAM_SHOW_COMPASS))
        render_compass (self->navguide_state, center_x, center_y, viewport[2], viewport[3]);

    if (bot_gtk_param_widget_get_bool(self->pw, PARAM_SHOW_TEXT_INFO))
        render_text_info (self->navguide_state, viewport[2], viewport[3]);

    if (bot_gtk_param_widget_get_bool(self->pw, PARAM_SHOW_LOC_PDF))
        render_pdf_plot (self->navguide_state, map_width, viewport[2], viewport[3]);

    int cellsize = bot_gtk_param_widget_get_int (self->pw, PARAM_NAME_SIM_MATRIX_SIZE);
    if (bot_gtk_param_widget_get_bool(self->pw, PARAM_SHOW_SIMILARITY_MATRIX))
        render_similarity_matrix (self->similarity_matrix, self->similarity_matrix_size, cellsize, viewport[2], viewport[3]);

    if (bot_gtk_param_widget_get_bool(self->pw, PARAM_SHOW_UI_MAP))
        render_ui_map (self->ui_map, map_width, map_decimate, viewport[2], viewport[3]);
    if (bot_gtk_param_widget_get_bool(self->pw, PARAM_SHOW_UI_MAP_LOCAL))
        render_ui_map_local (self->ui_map, self->navguide_state, map_zoom, map_width, map_decimate, viewport[2], viewport[3]);

    int hist_width = bot_gtk_param_widget_get_int (self->pw, PARAM_NAME_CLASS_HISTOGRAMS_SIZE);
    if (bot_gtk_param_widget_get_bool(self->pw, PARAM_SHOW_CLASS_HISTOGRAMS))
        render_histograms (self->class_hist, hist_width, 10, 10, nsensors, viewport[2], viewport[3]);

    gboolean draw_node_images = bot_gtk_param_widget_get_bool(self->pw, PARAM_SHOW_NODE_IMAGES);
    gboolean draw_node_features = bot_gtk_param_widget_get_bool(self->pw, PARAM_SHOW_NODE_FEATURES);

    render_node_images (self->navguide_state, self->dg, desired_width, viewport[2], viewport[3], nsensors, draw_node_images, draw_node_features);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

static void on_navguide_state (const lcm_recv_buf_t *rbuf, const char *channel, 
        const navlcm_class_param_t *msg, void *user )
{
    RendererGuidance *self = (RendererGuidance*) user;

    if (self->navguide_state)
        navlcm_class_param_t_destroy (self->navguide_state);

    self->navguide_state = navlcm_class_param_t_copy (msg);

    viewer_request_redraw(self->viewer);
}

static void on_ui_map (const lcm_recv_buf_t *rbuf, const char *channel,
        const navlcm_ui_map_t *msg, void *user)
{
    RendererGuidance *self = (RendererGuidance*) user;

    if (self->ui_map)
        navlcm_ui_map_t_destroy (self->ui_map);

    self->ui_map = navlcm_ui_map_t_copy (msg);
}

static void on_future_direction (const lcm_recv_buf_t *rbuf, const char *channel,
        const navlcm_generic_cmd_t *msg, void *user)
{
    RendererGuidance *self = (RendererGuidance*) user;

    self->future_direction[msg->code] = atoi (msg->text);

}

static void on_class_histogram (const lcm_recv_buf_t *rbuf, const char *channel,
        const navlcm_dictionary_t *msg, void *user)
{
    RendererGuidance *self = (RendererGuidance*) user;

    g_hash_table_insert (self->class_hist, strdup (channel), navlcm_dictionary_t_copy (msg));

    viewer_request_redraw(self->viewer);
}

static void on_similarity_matrix (const lcm_recv_buf_t *rbuf, const char *channel,
        const navlcm_dictionary_t *msg, void *user)
{
    RendererGuidance *self = (RendererGuidance*) user;

    if (self->similarity_matrix)
        free (self->similarity_matrix);

    self->similarity_matrix = dictionary_to_corrmat (msg, &self->similarity_matrix_size);
}

char *prompt_user_for_file (RendererGuidance *self, gboolean write)
{
    GtkWidget *dialog;

    char *filename = NULL;

    dialog = gtk_file_chooser_dialog_new ("Select file",
            NULL,
            write ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            write ? GTK_STOCK_SAVE : GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
            NULL);

    if (self->dirname)
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), 
                self->dirname);

    if (write)
        gtk_file_chooser_set_do_overwrite_confirmation 
            (GTK_FILE_CHOOSER (dialog), TRUE);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {

        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

        if (self->dirname)
            free (self->dirname);
        self->dirname = strdup ( gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog)));
    }

    gtk_widget_destroy (dialog);

    return filename;
}

static void on_param_widget_changed (BotGtkParamWidget *pw, const char *name, void *user)
{
    RendererGuidance *self = (RendererGuidance*) user;

    // read map from file
    if (!strcmp (name, RENDERER_GUIDANCE_LOAD_MAP)) {
        char *filename = prompt_user_for_file (self, FALSE);
        if (filename) {
            dijk_graph_destroy (self->dg);
            dijk_graph_init (self->dg);
            dijk_graph_read_from_file (self->dg, filename);
            g_free (filename);
        }
    }

    // demo code
    if (!strcmp (name, RENDERER_GUIDANCE_SET_REFERENCE)) {
        navlcm_generic_cmd_t cmd;
        cmd.code = CLASS_CHECK_CALIBRATION;
        cmd.text = (char*)"";
        navlcm_generic_cmd_t_publish (self->lcm, "CLASS_CMD", &cmd);

    }

    viewer_request_redraw(self->viewer);
}

static void on_load_preferences (Viewer *viewer, GKeyFile *keyfile, void *user_data)
{
    RendererGuidance *self = (RendererGuidance*)user_data;
    bot_gtk_param_widget_load_from_key_file (self->pw, keyfile, RENDERER_NAME);
}

static void on_save_preferences (Viewer *viewer, GKeyFile *keyfile, void *user_data)
{
    RendererGuidance *self = (RendererGuidance*)user_data;
    bot_gtk_param_widget_save_to_key_file (self->pw, keyfile, RENDERER_NAME);
}

void setup_renderer_guidance(Viewer *viewer, int priority) 
{
    RendererGuidance *self = 
        (RendererGuidance*) calloc(1, sizeof(RendererGuidance));

    Renderer *renderer = &self->renderer;

    renderer->draw = my_draw;
    renderer->destroy = my_free;
    renderer->name = (char*)RENDERER_NAME;
    self->pw = BOT_GTK_PARAM_WIDGET(bot_gtk_param_widget_new());

    self->future_direction[0] = -1;
    self->future_direction[1] = -1;
    self->future_direction[2] = -1;

    renderer->widget = GTK_WIDGET(self->pw);
    renderer->enabled = 1;
    renderer->user = self;

    load_icons (self);

    self->lcm = globals_get_lcm();
    self->config = globals_get_config();
    self->viewer = viewer;
    self->dirname = NULL;
    self->dg = dijk_graph_new();
    self->similarity_matrix = NULL;
    self->similarity_matrix_size = 0;
    self->ui_map = NULL;
    self->navguide_state = NULL;
    self->class_hist = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, classifier_destroy);

    bot_gtk_param_widget_add_booleans (self->pw, (BotGtkParamWidgetUIHint)0, 
            PARAM_SHOW_NODE_IMAGES, 0, PARAM_SHOW_NODE_FEATURES, 0, NULL);
    bot_gtk_param_widget_add_booleans (self->pw, (BotGtkParamWidgetUIHint)0, 
            PARAM_SHOW_SIMILARITY_MATRIX, 0, 
            PARAM_SHOW_MOTION_TYPE, 0, NULL);
    bot_gtk_param_widget_add_booleans (self->pw, (BotGtkParamWidgetUIHint)0, 
            PARAM_SHOW_COMPASS, 0 ,
            PARAM_SHOW_TEXT_INFO, 0 , NULL);
    bot_gtk_param_widget_add_booleans (self->pw, (BotGtkParamWidgetUIHint)0, 
            PARAM_SHOW_EXTRA_NAV, 0 ,
            PARAM_SHOW_LOC_PDF, 0 , NULL);
    bot_gtk_param_widget_add_booleans (self->pw, (BotGtkParamWidgetUIHint)0, 
            PARAM_SHOW_UI_MAP, 0 , 
            PARAM_SHOW_UI_MAP_LOCAL, 0 , 
            PARAM_SHOW_CLASS_HISTOGRAMS, 1, NULL);

    bot_gtk_param_widget_add_buttons( self->pw, RENDERER_GUIDANCE_LOAD_MAP, RENDERER_GUIDANCE_SET_REFERENCE, NULL);

    bot_gtk_param_widget_add_int (self->pw, PARAM_NAME_SIM_MATRIX_SIZE, 
            BOT_GTK_PARAM_WIDGET_SLIDER, 50, 500, 10, 150);
    bot_gtk_param_widget_add_int (self->pw, PARAM_NAME_UI_MAP_SIZE,
            BOT_GTK_PARAM_WIDGET_SLIDER, 50, 500, 10, 150);
    bot_gtk_param_widget_add_int (self->pw, PARAM_NAME_UI_MAP_DECIMATE,
            BOT_GTK_PARAM_WIDGET_SLIDER, 1, 100, 10, 10);
    bot_gtk_param_widget_add_int (self->pw, PARAM_NAME_CLASS_HISTOGRAMS_SIZE,
            BOT_GTK_PARAM_WIDGET_SLIDER, 1, 1000, 10, 10);
    bot_gtk_param_widget_add_double(self->pw, PARAM_IMAGE_DESIRED_WIDTH,
            BOT_GTK_PARAM_WIDGET_SLIDER, 1, 752, 10, 376);
    bot_gtk_param_widget_add_double(self->pw, PARAM_NAME_MAP_ZOOM,
            BOT_GTK_PARAM_WIDGET_SLIDER, .01, 1.0, .01, 1.0);

    navlcm_class_param_t_subscribe(self->lcm, "CLASS_STATE", on_navguide_state, self);
    navlcm_dictionary_t_subscribe (self->lcm, "SIMILARITY_MATRIX", on_similarity_matrix, self);
    navlcm_ui_map_t_subscribe (self->lcm, "UI_MAP.*", on_ui_map, self);
    navlcm_dictionary_t_subscribe (self->lcm, "CLASS_HIST.*", on_class_histogram, self);
    navlcm_generic_cmd_t_subscribe (self->lcm, "FUTURE_DIRECTION", on_future_direction, self);

    g_signal_connect (G_OBJECT (self->pw), "changed", 
            G_CALLBACK (on_param_widget_changed), self);

    viewer_add_renderer(viewer, renderer, priority);

    g_signal_connect (G_OBJECT (viewer), "load-preferences", 
            G_CALLBACK (on_load_preferences), self);
    g_signal_connect (G_OBJECT (viewer), "save-preferences",
            G_CALLBACK (on_save_preferences), self);

}
