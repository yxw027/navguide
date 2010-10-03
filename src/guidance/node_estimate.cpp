/*
 * Node estimation algorithm using only the Psi distance (no recursive Bayesian estimation).
 *
 */

#include <guidance/node_estimate.h>

/* search for the most similar gate in the neighborhood of <ref_gate> with a search radius of <radius>
 * <fs> are the currently observed features
 */
void node_estimate_update (GQueue *gates, navlcm_gate_t *ref_gate, navlcm_feature_list_t *fs, int radius, navlcm_gate_t **g1, navlcm_gate_t **g2)
{
    if (!ref_gate) { dbg (DBG_ERROR, "find_most_similar_gate: no ref gate."); return;}
    if (!fs) { dbg (DBG_ERROR, "find_most_similar_gate: no features."); return;}

    double best_sim = 0.0;
    int *ids = NULL;
    double *sims = NULL;
    int n = 0;

    // reset output
    *g1 = *g2 = NULL;

    // list neighbors
    GQueue *neighbors = g_queue_new ();
    gate_list_neighbors (ref_gate, gates, radius, neighbors);
    
    dbg (DBG_CLASS, "found %d neighbors to gate %d", ref_gate->uid, g_queue_get_length (neighbors));

    // compute similarity with neighbor gates
    for (GList *iter = g_queue_peek_head_link (neighbors);iter;iter=g_list_next(iter)) {
        navlcm_gate_t *g = (navlcm_gate_t*)iter->data;

        double sim = gate_similarity (g, fs);

        // update vector
        ids = (int*)realloc (ids, (n+1)*sizeof(int));
        ids[n] = g->uid;
        sims = (double*)realloc(sims, (n+1)*sizeof(double));
        sims[n] = sim;
        n++;

        dbg (DBG_INFO, "gate similarity: gate %d, sim = %.3f", g->uid, sim);
    }

    g_queue_free (neighbors);

    // smooth data
    double *ssims = math_smooth_double (sims, n, 5);
    for (int i=0;i<n;i++) {
        dbg (DBG_VIEWER, "sim[%d] = %.3f", ids[i], ssims[i]);
    }

    // find max value
    int id = math_max_index_double (ssims, n);

    *g1 = find_gate_from_id (gates, ids[id]);
    best_sim = ssims[id];

    if (*g1 == NULL) return;

    // find next best value
    sims[id] = 0.0;
    int id2 = math_max_index_double (ssims, n);
    double second_sim = ssims[id2];
    if (best_sim * .80 < second_sim) {
        navlcm_gate_t *g = find_gate_from_id (gates, ids[id2]);
        if (g && gate_is_neighbor (*g1, g)) {
            *g2 = g;
        }
    }

    free (ssims);
    free (sims);
    free (ids);

    if (*g1) dbg (DBG_INFO, "*** best gate ID: %d, similarity = %.3f\n", (*g1)->uid, best_sim);
    if (*g2) dbg (DBG_INFO, "*** 2nd  gate ID: %d, similarity = %.3f\n", (*g2)->uid, second_sim);

    return;
}

