#include "model.h"
#define MAXLINE 256
static char linebuf[MAXLINE];

static char *get_next_line (FILE * f)
{
    if (fgets (linebuf, MAXLINE, f)) {
        return linebuf;
    }
    else
        return NULL;
}

void model_init (model_t *m)
{
    m->verts = NULL;
    m->edges = NULL;
}

m_edge_t *model_new_edge (m_vertex_t *v1, m_vertex_t *v2)
{
    if (!v1 || !v2) return NULL;

    m_edge_t *edge = (m_edge_t*)malloc(sizeof(m_edge_t));
    edge->sp = v1;
    edge->ep = v2;
    return edge;
}

void model_read (model_t *model, const char *filename)
{
    if (!model) {
        fprintf (stderr, "model is null.");
        return;
    }

    // open file
    FILE *fp = fopen (filename, "r");
    if (!fp) {
        fprintf (stderr, "failed to open file %s", filename);
        return;
    }

    char *l;

    // read vertices and edges
    //
    m_vertex_t *last_v = NULL;
    m_vertex_t *first_v = NULL;
    int mode = 0;
    while ((l = get_next_line (fp))) {
        if (strstr (l, "LWPOLYLINE") && strstr (l, "Layer")) {
            assert (mode == 0);
            mode = 1;
            first_v = NULL;
            last_v = NULL;
            continue;
        } else if (strstr (l, "LINE") && strstr (l, "Layer")) {
            assert (mode == 0);
            mode = 2;
            first_v = NULL;
            last_v = NULL;
            continue;
        }

        if (mode == 2) {
            if (strstr (l, "from point") || strstr (l, "to point")) {
                // read a vertex
                double x, y;
                strsep (&l, "=");
                char *str_x = strsep (&l, "=");
                char *str_y = strsep (&l, "=");
                if (str_x && str_y) {
                    if (sscanf (str_x, "%lf", &x) == 1 &&
                            sscanf (str_y, "%lf", &y) == 1) {
                        m_vertex_t *v = (m_vertex_t*)malloc(sizeof(m_vertex_t));
                        v->x = x;
                        v->y = y;
                        model->verts = g_list_prepend (model->verts, v);
                        if (!first_v) {
                            first_v = v;
                        } else {
                            // create an edge
                            m_edge_t *edge = model_new_edge (first_v, v);
                            if (edge)
                                model->edges = g_list_prepend (model->edges, edge);
                            mode = 0;
                        }
                    }
                }
            }
        }

        if (mode == 1) {
            if (strstr (l, "at point ")) {
                // read a vertex
                double x, y;
                strsep (&l, "=");
                char *str_x = strsep (&l, "=");
                char *str_y = strsep (&l, "=");
                if (str_x && str_y) {
                    if (sscanf (str_x, "%lf", &x) == 1 &&
                            sscanf (str_y, "%lf", &y) == 1) {
                        m_vertex_t *v = (m_vertex_t*)malloc(sizeof(m_vertex_t));
                        v->x = x;
                        v->y = y;
                        model->verts = g_list_prepend (model->verts, v);

                        if (!first_v)
                            first_v = v;

                        if (last_v) {
                            // create an edge
                            m_edge_t *edge = model_new_edge (last_v, v);
                            if (edge)
                                model->edges = g_list_prepend (model->edges, edge);
                        }
                        last_v = v;
                    }
                }
            } else {
                if (first_v && last_v) {
                    // close the polyline
                    m_edge_t *edge = model_new_edge (last_v, first_v);
                    if (edge)
                        model->edges = g_list_prepend (model->edges, edge);
                    mode = 0;
                }
            }
        }
    }

    printf ("read %d vertices and %d edges.\n", g_list_length (model->verts), g_list_length (model->edges));

    // close file
    fclose (fp);

}

void model_translate (model_t *model, double *vect)
{
    if (!model) return;

    for (GList *iter=g_list_first (model->verts);iter;iter=g_list_next(iter)) {
        m_vertex_t *v = (m_vertex_t*)iter->data;
        v->x += vect[0];
        v->y += vect[1];
    }
}

void model_centroid (model_t *model, double *centroid)
{
    if (!model) return;

    centroid[0] = 0.0;
    centroid[1] = 0.0;
    centroid[2] = 0.0;

    int n=0;

    if (model->verts) {
        for (GList *iter=g_list_first(model->verts);iter;iter=g_list_next(iter)) {
            m_vertex_t *v = (m_vertex_t*)iter->data;
            centroid[0] += v->x;
            centroid[1] += v->y;
            centroid[2] += 0.0;
            n++;
        }
    }

    if (n>0) {
        centroid[0] /= n;
        centroid[1] /= n;
        centroid[2] /= n;
    }
}

void model_diameter (model_t *model, double *diam)
{
    if (!model) return;

    // compute centroid
    double centroid[3];
    model_centroid (model, centroid);

    // compute max distance to centroid
    double diameter = 0.0;

    if (model->verts) {
        for (GList *iter = g_list_first (model->verts);iter;iter=g_list_next(iter)) {
            m_vertex_t *v = (m_vertex_t*)iter->data;
            double d = sqrt (pow(centroid[0]-v->x,2) + pow(centroid[1]-v->y,2) + pow(centroid[2]-0.0,2));
            diameter = fmax (diameter, d);
        }
    }

    if (diam) *diam = diameter;
}


void model_closest_edge (model_t *model, double x, double y, m_edge_t **edge, double *dist)
{
    if (!model || !model->edges) return;

    double mindist = 0.0;

    for (GList *iter=g_list_first (model->edges);iter;iter=g_list_next(iter)) {
        m_edge_t *e = (m_edge_t*)iter->data;
        double d;
        if (!math_dist_2d (x, y, e->sp->x, e->sp->y, e->ep->x, e->ep->y, NULL, NULL, &d))
            continue;
        if (fabs (mindist) < 1E-6 || d < mindist) {
            mindist = d;
            if (edge) *edge = e;
        }   
    }

    if (dist) *dist = mindist;
}

void model_clear (model_t *model)
{
    if (!model) return;

    // free vertices
    for (GList *iter=g_list_first (model->verts);iter;iter=g_list_next (iter)) {
        m_vertex_t *v = (m_vertex_t*)iter->data;
        free (v);
    }

    g_list_free (model->verts);
    model->verts = NULL;

    // free edges
    for (GList *iter=g_list_first (model->edges);iter;iter=g_list_next (iter)) {
        m_edge_t *e = (m_edge_t*)iter->data;
        free (e);
    }

    g_list_free (model->edges);
    model->edges = NULL;
}

void model_bounding_box (model_t *model, double *xmin, double *xmax, double *ymin, double *ymax)
{
    if (!model) return;
    assert (xmin && xmax && ymin && ymax);

    if (!model->verts) {
        *xmin = *xmax = *ymin = *ymax = 0.0;
        return;
    }

    *xmin = +1000000.0;
    *xmax = -1000000.0;
    *ymin = +1000000.0;
    *ymax = -1000000.0;

    for (GList *iter = g_list_first (model->verts);iter;iter=g_list_next(iter)) {
        m_vertex_t *v = (m_vertex_t*)iter->data;
        *xmin = fmin (*xmin, v->x);
        *xmax = fmax (*xmax, v->x);
        *ymin = fmin (*ymin, v->y);
        *ymax = fmax (*ymax, v->y);
    }
}

