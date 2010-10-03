#ifndef _MODEL_H__
#define _MODEL_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <math.h>

/* from glib */
#include <glib.h>

/* from common */
#include <common/mathutil.h>

typedef struct _m_vertex_t {
    double x, y;
} m_vertex_t;

typedef struct _m_edge_t {
    m_vertex_t *sp, *ep;
} m_edge_t;

typedef struct _model_t {
    GList *verts;
    GList *edges;
} model_t;

void model_init (model_t *m);
void model_read (model_t *m, const char *filename);
void model_centroid (model_t *model, double *centroid);
void model_diameter (model_t *model, double *diam);
void model_closest_edge (model_t *model, double x, double y, m_edge_t **edge, double *dist);
void model_clear (model_t *model);
void model_bounding_box (model_t *model, double *xmin, double *xmax, double *ymin, double *ymax);
void model_translate (model_t *model, double *vect);

#endif

