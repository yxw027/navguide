/* Node estimation algorithm using recursive Bayesian estimation.
 */

#include "state.h"

static double g_state_sigma = 1.0;

gboolean state_transition_convolve_cb (GNode *node, gpointer data)
{
    int depth = g_node_depth (node);

    dijk_node_t *nd = (dijk_node_t*)node->data;

    double *val = (double*)data;

    *val += nd->pdf1 * exp (-powf(depth,2)/(2.0*g_state_sigma*g_state_sigma));

    return FALSE;
}

gboolean state_observation_sum_cb (GNode *node, gpointer data)
{
    dijk_node_t *nd = (dijk_node_t*)node->data;

    double *val = (double*)data;

    *val += nd->pdf1;

    return FALSE;
}

gboolean state_observation_normalize_cb (GNode *node, gpointer data)
{
    dijk_node_t *nd = (dijk_node_t*)node->data;

    double *val = (double*)data;

    nd->pdf1 /= *val;

    return FALSE;
}

gboolean state_transition_sum_cb (GNode *node, gpointer data)
{
    dijk_node_t *nd = (dijk_node_t*)node->data;

    double *val = (double*)data;

    *val += nd->pdf0;

    return FALSE;
}

gboolean state_transition_normalize_cb (GNode *node, gpointer data)
{
    dijk_node_t *nd = (dijk_node_t*)node->data;

    double *val = (double*)data;

    nd->pdf0 /= *val;

    return FALSE;
}

/* apply timestamp
*/
gboolean state_transition_timestamp_cb (GNode *node, gpointer data)
{
    dijk_node_t *nd = (dijk_node_t*)node->data;
    int64_t *timestamp = (int64_t*)data;
    nd->timestamp = *timestamp;

    return FALSE;
}

/* apply the transition update to the belief state
 * <dg> is the graph (entire map)
 * <cg> is the current gate
 */
void state_transition_update (dijk_graph_t *dg, int radius, double state_sigma)
{
    if (!dg) return;

    g_state_sigma = state_sigma;

    int64_t timestamp = bot_timestamp_now ();

    /* reset all pdf to zero */
    for (GList *iter=g_queue_peek_head_link(dg->nodes);iter;iter=iter->next) {
        dijk_node_t *nd = (dijk_node_t*)iter->data;
        nd->pdf0 = .0;
    }

    /* loop through all nodes */
    for (GList *iter=g_queue_peek_head_link(dg->nodes);iter;iter=iter->next) {
        dijk_node_t *nd = (dijk_node_t*)iter->data;

        // convert to dual tree
        GNode *tr = dijk_to_tree (nd, radius);

        // convolve with gaussian kernel
        double val = .0;
        g_node_traverse (tr, G_PRE_ORDER, G_TRAVERSE_ALL, -1, state_transition_convolve_cb, &val);
        nd->pdf0 = val;
        
        g_node_destroy (tr);
    }

    // normalize
    double t = .0;
    for (GList *iter=g_queue_peek_head_link(dg->nodes);iter;iter=iter->next) {
        dijk_node_t *nd = (dijk_node_t*)iter->data;
        t += nd->pdf0;
    }

    if (t > 1E-6) {
        for (GList *iter=g_queue_peek_head_link(dg->nodes);iter;iter=iter->next) {
            dijk_node_t *nd = (dijk_node_t*)iter->data;
            nd->pdf0 /= t;
        }
    }
}

void state_print_to_file (dijk_graph_t *dg, const char *filename)
{
    char tmpfilename[256];
    sprintf (tmpfilename, "%s.zzz.tmp", filename);

    FILE *fp = fopen (tmpfilename, "w");
    if (!fp)
        return;

    for (GList *iter=g_queue_peek_head_link (dg->nodes);iter;iter=iter->next) {
        dijk_node_t *nd = (dijk_node_t*)iter->data;
        fprintf (fp, "%d %.5f\n", nd->uid, nd->pdf1);
    }

    fclose (fp);

    rename (tmpfilename, filename);
}

/* print out the state
*/
void state_print (dijk_graph_t *dg, dijk_node_t *cg)
{
    dbg (DBG_CLASS, "*************************************************");
    double sum_pdf1 = .0, sum_pdf0 = .0;

    for (GList *iter=g_queue_peek_head_link (dg->nodes);iter;iter=iter->next) {
        dijk_node_t *nd = (dijk_node_t*)iter->data;
        if (cg && nd == cg) {
            dbg (DBG_CLASS, "gate [%d]: %.4f\t%.4f (***)", nd->uid, nd->pdf0, nd->pdf1);
        } else {
            dbg (DBG_CLASS, "gate [%d]: %.4f\t%.4f", nd->uid, nd->pdf0, nd->pdf1);
        }
        sum_pdf1 += nd->pdf1;
        sum_pdf0 += nd->pdf0;
    }
    dbg (DBG_CLASS, "sum pdf0 = %.4f  sum pdf1 = %.4f", sum_pdf0, sum_pdf1);
}

gboolean state_compute_variance_cb (GNode *nd, gpointer data)
{
    double *variance = (double*)data;
    dijk_node_t *n = (dijk_node_t*)nd->data;

    *variance += n->pdf1 * powf (1.0 * g_node_depth (nd), 2);
    return FALSE;
}

gboolean state_observation_cb (GNode *nd, gpointer data)
{
    dijk_node_t *n = (dijk_node_t*)nd->data;
    navlcm_feature_list_t *f = (navlcm_feature_list_t*)data;
    navlcm_feature_list_t *fn = dijk_node_get_nth_features (n, 0);
    assert (fn);
    printf ("state obs. %d %d\n", f->num, fn->num); 
    double prob = 1.0 - class_psi_distance (fn, f, NULL);
    n->pdf1 = n->pdf0 * prob;
    n->timestamp = f->utime;
    return FALSE;
}

/* apply the observation update to the belief state
 * limiting the application to <radius> distance to edge <e>
 */
void state_observation_update (dijk_graph_t *dg, dijk_edge_t *e, int radius, navlcm_feature_list_t *f, GQueue *path, double *variance)
{
    if (!e || !e->start)
        return;

    // convert to dual tree
    GNode *tr = dijk_to_tree (e->start, radius);

    // apply observation update to the tree 
    g_node_traverse (tr, G_PRE_ORDER, G_TRAVERSE_ALL, -1, state_observation_cb, f);

    // compute variance across tree
    *variance = .0;
    g_node_traverse (tr, G_PRE_ORDER, G_TRAVERSE_ALL, -1, state_compute_variance_cb, variance);
    *variance /= radius;

    // destroy tree
    g_node_destroy (tr);

    // set to zero for all nodes out of the tree
    for (GList *iter=g_queue_peek_head_link(dg->nodes);iter;iter=iter->next) {
        dijk_node_t *nd = (dijk_node_t*)iter->data;
        if (nd->timestamp != f->utime) {
            nd->pdf0 = .0;
            nd->pdf1 = .0;
        }
    }

    // hack: set to zero for all nodes out of the path
    for (GList *iter=g_queue_peek_head_link(dg->nodes);iter;iter=iter->next) {
        dijk_node_t *nd = (dijk_node_t*)iter->data;
        if (!path) continue;
        gboolean on_path = FALSE;
        for (GList *eiter=g_queue_peek_head_link (path);eiter;eiter=eiter->next) {
            dijk_edge_t *e = (dijk_edge_t*)eiter->data;
            if (e->start == nd || e->end == nd) {
                on_path = TRUE;
            }
        }
        if (!on_path) {
    //        nd->pdf0 = .0;
    //        nd->pdf1 = .0;
        }
    }

    // normalize
    double t = state_sum (dg);

    if (t > 1E-6) {
        for (GList *iter=g_queue_peek_head_link(dg->nodes);iter;iter=iter->next) {
            dijk_node_t *nd = (dijk_node_t*)iter->data;
            nd->pdf1 /= t;
        }
    }

}

double state_sum (dijk_graph_t *dg)
{
    double t = .0;
    for (GList *iter=g_queue_peek_head_link(dg->nodes);iter;iter=iter->next) {
        dijk_node_t *nd = (dijk_node_t*)iter->data;
        t += nd->pdf1;
    }

    return t;
}

gboolean state_find_maximum_cb (GNode *node, gpointer data)
{
    dijk_node_t *n1 = (dijk_node_t*)node->data;
    dijk_node_t **n2 = (dijk_node_t**)data;

    if (n1->pdf1 < (*n2)->pdf1)
        *n2 = n1;

    return FALSE;
}

/* find the node with maximum probability
 * <og> is the output gate (the one with highest probability)
 */
void state_find_maximum (dijk_graph_t *dg, dijk_node_t **og)
{
    double maxpdf = -1.0;

    for (GList *iter=g_queue_peek_head_link (dg->nodes);iter;iter=iter->next) {
        dijk_node_t *nd = (dijk_node_t*)iter->data;
        if (nd->pdf1 > maxpdf) {
            maxpdf = nd->pdf1;
            *og = nd;
        }
    }
}


/* update the classifier state with pdf values
*/
void state_update_class_param (navlcm_class_param_t *state, dijk_graph_t *dg)
{
    if (g_queue_is_empty (dg->nodes))
        return;

    if (state->pdfval)
        free (state->pdfval);
    if (state->pdfind)
        free (state->pdfind);
    state->pdf_size = g_queue_get_length (dg->nodes);
    state->pdfind = (int*)calloc(state->pdf_size, sizeof(int));
    state->pdfval = (double*)calloc(state->pdf_size, sizeof(double));

    int count = 0;
    for (GList *iter=g_queue_peek_head_link(dg->nodes);iter;iter=iter->next) {
        dijk_node_t *nd = (dijk_node_t*)iter->data;
        state->pdfind[count] = nd->uid;
        state->pdfval[count] = nd->pdf1;
        count++;
    }
}

void state_init (dijk_graph_t *dg, dijk_node_t *n)
{
    assert (n);

    for (GList *iter=g_queue_peek_head_link (dg->nodes);iter;iter=iter->next) {
        dijk_node_t *p = (dijk_node_t*)iter->data;
        if (p == n) {
            p->pdf0 = 1.0;
            p->pdf1 = 1.0;
        }
        else {
            p->pdf0 = .0;
            p->pdf1 = .0;
        }
    }
}

