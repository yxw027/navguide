/*
 * Place graph API (a node is referred to as a "gate").
 */

#include "gates.h"

gate_t *gate_new (gboolean checkpoint)
{
    gate_t *g = (gate_t*)malloc(sizeof(gate_t));

    navlcm_gps_to_local_t_clear (&g->gps_to_local);
    botlcm_pose_t_clear (&g->pose);

    g->uid = uid;
    g->img = NULL;
    g->features = NULL;
    g->neighbors = NULL;
    g->pdf0 = .0;
    g->pdf1 = .0;
    g->label = NULL;
    g->checkpoint = checkpoint;

    return g;
}

void gate_clear_neighbors (gate_t *g)
{
    for (GList *iter=g_queue_peek_head_link (g->features);iter;iter=iter->next) {
        navlcm_feature_list_t *f = (navlcm_feature_list_t*)iter->data;
        navlcm_feature_list_t_destroy (f);
    }
    g_queue_free (g->features);
    g_queue_free (g->neighbors);
    
    g->neighbors = NULL;
    g->features = NULL;
    
}

int gate_set_label (gate_t *g, const char *label)
{
    if (!g) return 0;

    free (g->label);
    g->label = strdup (label);
    return g->uid;
}

void gate_insert_features (gate_t *g, navlcm_feature_list_t *f)
{
    g_queue_push_tail (g->features, f);
}

void gate_insert_neighbor (gate_t *g, gate_t *gg)
{
    assert (g != gg);
    g_queue_push_tail (g->neighbors, gg);
}

void gate_insert_neighbor_and_features (gate_t *g, gate_t *gg, navlcm_feature_list_t *f)
{
    assert (g != gg);
    g_queue_push_tail (g->neighbors, gg);
    g_queue_push_tail (g->features, f);
}

void gate_print (gate_t *g)
{
    printf ("%d: ", g->uid);
    for (GList *iter=g_queue_peek_head_link (g->neighbors);iter;iter=iter->next) {
        gate_t *gg = (gate_t*)iter->data;
        printf ("%d ", gg->uid);
    }
    printf ("\n");
}

void gate_save_image_tile_to_file (gate_t *g, const char *filename)
{
    dbg (DBG_CLASS, "saving gate image to %s", filename);
    image_stitch_to_file (g->img, filename);
}

void gates_save_image_tile_to_file (GQueue *gates)
{
    for (GList *iter=g_queue_peek_head_link (gates);iter;iter=g_list_next(iter)) {
        gate_t *g = (gate_t*)iter->data;
        char fname[256];
        sprintf (fname, "gate-%04d.png", g->uid);
        gate_save_image_tile_to_file (g, fname);
    }

}

void gates_print (GQueue *gates)
{
    printf ("================ %d gates ===================\n", g_queue_get_length (gates));
    for (GList *iter=g_queue_peek_head_link(gates);iter;iter=g_list_next(iter)) {
        gate_t *g = (gate_t*)iter->data;
        gate_print (g);
    }
}

void gate_update_up_image (gate_t *g, gpointer data)
{   
    if (!data) return;
    if (g->up_img)
        botlcm_image_t_destroy (g->up_img);
    g->up_img = camcm_image_t_copy ((botlcm_image_t*)data);
}

void gate_add_image (gate_t *g, botlcm_image_t *img)
{
    g_queue_push_tail (g->img, botlcm_image_t_copy (img));
}

int64_t gate_utime (gate_t *g)
{
    if (!g_queue_is_empty (g->img)) {
        botlcm_image_t *img = (botlcm_image_t*)g_queue_peek_head (g->img);
        return img->utime;
    }

    return 0;
}

navlcm_feature_list_t* gate_features (gate_t *g, gate_t *gg)
{
    GList *fiter = g_queue_peek_head_link (g->features);

    for (GList *iter=g_queue_peek_head_link (g->neighbors);iter;iter=iter->next) {
        if ((gate_t*)iter->data == gg)
            return (navlcm_feature_list_t*)fiter->data;
        fiter = fiter->next;
    }

    // this should not happen...
    return NULL;
}

/* compute direction in front of a gate <g> given a set of observed features <fs>
 *
 * <calib> is a 1D array encoding the system calibration
 */
int gate_orientation (gate_t *g, navlcm_feature_list_t *fs, config_t *config, 
        double *angle_rad, lcm_t *lcm)
{
    if (!g) {
        dbg (DBG_ERROR, "gate_orientation: gate undefined.");
        return -1;
    }

    if (!fs) {
        dbg (DBG_ERROR, "gate_orientation: features undefined.");
        return -1;
    }

    // collect gate features
    navlcm_feature_list_t *sg = (navlcm_feature_list_t*)&g->ft;
    if (!sg) {
        dbg (DBG_ERROR, "gate_orientation: no features in the gate so far.");
        return -1;
    }

    return class_orientation (fs, sg, config, angle_rad, lcm);
}


void gate_save_image_to_file (gate_t *g, const char *dirname)
{
    if (!g) return;

    // check that directory exists
    if (!dir_exists (dirname)) {
        dbg (DBG_ERROR, "dir does not exist: %s", dirname);
        return;
    }

    // save image to directory
    char fname[256];
    for (int i=0;i<g->nimg;i++) {
        sprintf (fname, "%s/gate-%d-img-%d.pgm", dirname, g->uid, i);
        write_pgm (g->img[i].data, g->img[i].width, g->img[i].height, fname);
    }
}

/* compute a similarity measure between a set of observed features <fs> and a gate <g>
 * output value in [0,1] where 1 means high similarity
 *
 */
double gate_similarity (gate_t *g, navlcm_feature_list_t *fs)
{
    if (!g || !fs || fs->num==0) {
        dbg (DBG_ERROR, "warning: gate_similarity call on invalid data.");
        return 0.0;
    }

    dbg (DBG_INFO, "class info gate %d: (ft %d)", g->uid, g->ft.num);

    // fetch gate features
    navlcm_feature_list_t *hs = gate_features (g);

    if (!hs) {
        dbg (DBG_ERROR, "failed to retrieve head features for gate %d", g->uid);
        return 0.0;
    }

    dbg (DBG_CLASS, "gate_similarity: matching %d - %d features.", fs->num, hs->num);

    // match with input features
    navlcm_feature_match_set_t *matches = find_feature_matches_multi (fs, hs, TRUE, TRUE, 5, 0.80, -1.0, -1.0, TRUE);


    if (!matches || matches->num == 0)  {
        navlcm_feature_list_t_destroy (hs);
        return 0.0;
    }

    double sim = 0.0;

    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *match = matches->el + i;
        if (match->num == 0) 
            sim += 1.0;
        else {

            assert (sqrt (match->dist[0])/255.0 < 1.0 + 1E-6);
            sim += match->dist[0]/(255.0*255.0);
        }
    }

    // add penalty for unmatched features
    sim += fs->num - matches->num;

    sim /= fs->num;

    assert (-1E-6 < sim && sim < 1.0 + 1E-6);

    sim = 1.0 - sim;

    // free data
    navlcm_feature_match_set_t_destroy (matches);

    return sim;
}

void gate_set_pose (gate_t *gate, botlcm_pose_t *p)
{
    botlcm_pose_t *copy = botlcm_pose_t_copy (p);
    gate->pose = *copy;
    free (copy);
}

void gate_set_gps_to_local (gate_t *gate, navlcm_gps_to_local_t *g)
{
    navlcm_gps_to_local_t *copy = navlcm_gps_to_local_t_copy (g);
    gate->gps_to_local = *copy;
    free (copy);
}

gate_t *gate_find_by_uid (GQueue *gates, int uid)
{
    if (!gates) return NULL;
    for (GList *iter=g_queue_peek_head_link(gates);iter;iter=g_list_next(iter)) {
        gate_t *gate = (gate_t*)iter->data;
        if (gate->uid == uid)
            return gate;
    }

    return NULL;
}

// remove gate id1, merge it into gate id2
//
void gates_merge (GQueue *gates, int id1, int id2)
{
    gate_t *g1 = (gate_find_by_uid (gates, id1));
    gate_t *g2 = (gate_find_by_uid (gates, id2));

    if (!g1) {
        dbg (DBG_ERROR, "[merge] failed to retrieve gate for ids %d", id1);
        assert (false);
        return;
    }
    if (!g2) {
        dbg (DBG_ERROR, "[merge] failed to retrieve gate for ids %d", id2);
        assert (false);
        return;
    }

    if (fabs (id1-id2)<2) {
        dbg (DBG_CLASS, "gates %d and %d are too close to be merged.", id1, id2);
        return;
    }

    dbg (DBG_CLASS, "merging gates %d and %d", id1, id2);

    // remove link if they are neighbor 
    // (may happen in case of one-to-many matching)
    gate_remove_neighbor (g1, g2->uid);
    gate_remove_neighbor (g2, g1->uid);

    // transfer neighbors from id1 to id2
    dbg (DBG_CLASS, "adding %d neighbors to gate %d", g1->ng, id2);

    for (int i=0;i<g1->ng;i++) {
        gate_insert_neighbor (g2, g1->ng_names[i]);
        dbg (DBG_CLASS, "%d", g1->ng_names[i]);
    }

    // replace name everywhere
    for (GList *iter=g_queue_peek_head_link (gates);iter;iter=g_list_next(iter)) {
        gate_t *g = (gate_t*)iter->data;
        gate_replace_in_neighbors (g, id1, id2);
        gate_remove_neighbor_duplicates (g);
    }

    // remove from queue
    unsigned int n = g_queue_get_length (gates);
    g_queue_remove (gates, g1);
    if (g_queue_get_length (gates) != n-1) {
        dbg (DBG_ERROR, "failed to remove gate %d", g1->uid);
        assert (false);
    }

    // sanity check: check that name "id1" has disappeared completely
    for (GList *iter=g_queue_peek_head_link (gates);iter;iter=g_list_next(iter)) {
        gate_t *g = (gate_t*)iter->data;
        assert (g->uid != id1);
        for (int i=0;i<g->ng;i++)
            assert (g->ng_names[i] != id1);
    }

    // free memory
    gate_t_destroy (g1);

    gate_print (g2);
}

void gates_sanity_check (GQueue *gates)
{
    int count=g_queue_get_length (gates)-1;
    for (GList *iter=g_queue_peek_head_link (gates);iter;iter=g_list_next(iter)) {
        gate_t *g = (gate_t*)iter->data;
        if (g->uid != count) 
            dbg (DBG_ERROR, "sanity check failed. gate at position %d has id %d", count, g->uid);
        assert (g->uid == count);
        count--;
    }
}

void gates_merge (GQueue *gates, GQueue *components)
{
    int min_key, max_key;
    int count=0;

    dbg (DBG_CLASS, "merging %d gates, %d components", g_queue_get_length (gates),
            g_queue_get_length (components));


    for (GList *iter=g_queue_peek_head_link (components);iter;iter=g_list_next(iter)) {
        component_t *c = (component_t*)iter->data;
        
        // remove trivial links inside the key sequence
        min_key = -1;
        max_key = -1;

        for (GList *iter2=g_queue_peek_head_link (c->pt);iter2;iter2=g_list_next(iter2)) {
            pair_t *p = (pair_t*)iter2->data;
            if (min_key == -1) min_key = p->key;
            if (max_key == -1) max_key = p->key;
            min_key = MIN (min_key, p->key);
            max_key = MAX (max_key, p->key);
        }

        for (int key=min_key;key<max_key;key++) {
            gate_t *g1 = gate_find_by_uid (gates, key);
            gate_t *g2 = gate_find_by_uid (gates, key+1);
            if (g1 && g2) {
                gate_remove_neighbor (g1, g2->uid);
                gate_remove_neighbor (g2, g1->uid);
                dbg (DBG_CLASS, "removing neighbor %d from %d", g2->uid, g1->uid);
                gate_print (g1);
                dbg (DBG_CLASS, "removing neighbor %d from %d", g1->uid, g2->uid);
                gate_print (g2);
            }
        }

        // merge gates one by one
        for (GList *iter2=g_queue_peek_head_link (c->pt);iter2;iter2=g_list_next(iter2)) {
            pair_t *p = (pair_t*)iter2->data;

            if (p->key < p->val) {
                int tmp = p->val;
                p->val = p->key;
                p->key = tmp;
            }

            gates_merge (gates, p->key, p->val);

            // update names in the components
            for (GList *iter3=iter;iter3;iter3=g_list_next(iter3)) {
                component_t *c3 = (component_t*)iter3->data;
                for (GList *iter4=g_queue_peek_head_link (c3->pt);iter4;iter4=g_list_next(iter4)) {
                    if (iter4==iter2) continue;
                    pair_t *pp = (pair_t*)iter4->data;
                    if (pp->key == p->key) {
                        pp->key = p->val;
                    }
                    if (pp->val == p->key) {
                        pp->val = p->val;
                    }
                }
            }
        }

        // only process one component
        count++;
        //     if (count >= 2)
        //         break;
    }

    dbg (DBG_CLASS, "after merge: %d gates.", g_queue_get_length (gates));
}

// remove gates with no neighbors (singletons)
//
void gates_remove_singletons (GQueue *gates)
{
    gboolean done = FALSE;
    gate_t *g;

    while (!done) {
        done = TRUE;
        g = NULL;
        for (GList *iter=g_queue_peek_head_link (gates);iter;iter=g_list_next(iter)) {
            g = (gate_t*)iter->data;
            if (g->ng==0) {
                done = FALSE;

                // sanity check (no gate should have this one as a neighbor)
                for (GList *iter2=g_queue_peek_head_link (gates);iter2;iter2=g_list_next(iter2)) {
                    gate_t *g2 = (gate_t*)iter2->data;
                    assert (!gate_is_neighbor (g2, g));
                }

                printf ("removing gate %d %d\n", g->uid, rand());
                g_queue_remove (gates, g);
                break;
            }
        }
    }
}

// assumes that gates are stored in backward order (i.e head of the queue is latest gate)
//
void gates_renumber (GQueue *gates)
{
    int count=0;

    dbg (DBG_CLASS, "renumbering %d gates.", g_queue_get_length (gates));

    // renumber gates
    for (GList *iter=g_queue_peek_tail_link(gates);iter;iter=g_list_previous(iter)) {
        gate_t *g = (gate_t*)iter->data;
        if (g->uid != count) {
            for (GList *iter=g_queue_peek_head_link(gates);iter;iter=g_list_next(iter)) {
                gate_t *g2 = (gate_t*)iter->data;
                gate_replace_in_neighbors (g2, g->uid, count);
            }
            g->uid = count;
        }
        count++;
    }

    // sanity check
    gates_sanity_check (gates);
}

void gates_assign_linear_neighbors (GQueue *gates)
{
    for (GList *iter=g_queue_peek_head_link(gates);iter;iter=g_list_next(iter)) {
        gate_t *g = (gate_t*)iter->data;
        gate_t *pg = NULL, *ng = NULL;
        if (iter->prev)
            pg = (gate_t*)iter->prev->data;
        if (iter->next)
            ng = (gate_t*)iter->next->data;
        if (pg)
            gate_insert_neighbor (g, pg->uid);
        if (ng)
            gate_insert_neighbor (g, ng->uid);
    }
}

gate_t* find_gate_from_id (GQueue *gates, int id)
{
    if (!gates) return NULL;
    GList *el = g_queue_find_custom (gates, &id, (GCompareFunc)navlcm_gate_comp);
    if (!el) return NULL;
    return (gate_t*)el->data;
}

// convert a list of components (as a matrix) to a graph
// <gates>: existing gates
// <mat>: matrix encoding connection between gates (non-zero means connection)
// size: size of <mat> (mat = size x size)
//
GQueue *gates_create_from_component_list (GQueue *gates, double *mat, int size, int maxsize, int *aliasname)
{
    GQueue *new_gates;
    gate_t *prev_gate, *curr_gate;
    GHashTable *corrtab;
    *aliasname = -1;

    dbg (DBG_CLASS, "creating gates from %d existing gates and %d x %d component matrix", g_queue_get_length (gates), size, size);

    // init
    new_gates = g_queue_new ();
    prev_gate = NULL;
    corrtab = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, g_free);

    // for each row in the matrix
    for (int i=0;i<(maxsize != -1 ? maxsize : size);i++) {

        dbg (DBG_CLASS, "step %d/%d", i, maxsize);

        // fetch corresponding gate
        gate_t *g = gate_find_by_uid (gates, i);
        if (!g) {
            dbg (DBG_ERROR, "failed to find gate for id %d", i);
            assert (false);
        }
        // find first non-zero element in the matrix
        int jj=-1;
        for (int j=0;j<size;j++) {
            if (CORRMAT_GET (mat, size, i, j) > 1E-6) {
                jj = j;
                break;
            }
        }
        assert (jj != i);

        // nothing found, create a new gate
        if (jj==-1) {
            dbg (DBG_CLASS, "no match found. creating new gate id %d", g->uid);
            curr_gate = gate_t_copy (g);
            gate_clear_neighbors (curr_gate);
            // add to the queue
            g_queue_push_head (new_gates, curr_gate);
        } else {
            // found a correspondence
            // add to correspondence table
            dbg (DBG_CLASS, "adding key %d to hash table.", i);
            g_hash_table_replace (corrtab, g_intdup (i), g_intdup (jj));

            dbg (DBG_CLASS, "match between %d and %d.", i, jj);
            curr_gate = gate_find_by_uid (new_gates, jj);
            if (!curr_gate) {
                // not found with this name; lookup alias in the correspondence table
                gpointer data = g_hash_table_lookup (corrtab, &jj);
                int alias = -1;
                while (data) {
                    int *ptr = (int*)data;
                    alias = *ptr;
                    data = g_hash_table_lookup (corrtab, ptr);
                }
                assert (alias!=-1);
                dbg (DBG_CLASS, "looking up %d (alias for %d)", alias, jj);
                curr_gate = gate_find_by_uid (new_gates, alias);
                if (!curr_gate) 
                    dbg (DBG_ERROR, "failed to retrieve gate for id %d", alias);
                assert (curr_gate);
            }
        }

        assert (curr_gate);
        if (prev_gate && curr_gate->uid == prev_gate->uid) continue;

        dbg (DBG_CLASS, "we are at gate %d", curr_gate->uid);
        *aliasname = curr_gate->uid;

        // connect it to the previous one
        if (prev_gate) {
            gate_insert_neighbor (curr_gate, prev_gate->uid);
            gate_insert_neighbor (prev_gate, curr_gate->uid);
        }

        // update previous gate
        prev_gate = curr_gate;
    }

    g_hash_table_destroy (corrtab);

    dbg (DBG_CLASS, "created new set of %d gates", g_queue_get_length (new_gates));

    return new_gates;

}

void gate_list_neighbors (gate_t *g, GQueue *gates, int radius, GQueue *neighbors)
{
    if (radius == 0) {
        if (!g_queue_find (neighbors, g))
            g_queue_push_tail (neighbors, g);
    } else {
        for (int i=0;i<g->ng;i++) {
            gate_t *gg = gate_find_by_uid (gates, g->ng_names[i]);
            if (!gg) {
                dbg (DBG_ERROR, "failed to find gate %d, neighbor of %d, radius = %d",
                        g->ng_names[i], g->uid, radius);
                assert (false);
            }
            if (!g_queue_find (neighbors, gg))
                g_queue_push_tail (neighbors, gg);
            gate_list_neighbors (gg, gates, radius-1, neighbors);
        }
    }
}

void gates_cleanup (GQueue *gates)
{
    for (GList *iter = g_queue_peek_head_link (gates);iter;iter=g_list_next(iter)) {
        gate_t *g = (gate_t*)iter->data;
        gboolean done = FALSE;
        while (!done) {
            done = TRUE;
            for (int i=0;i<g->ng;i++) {
                if (fabs (g->ng_names[i]-g->uid) <= 4 &&
                        fabs (g->ng_names[i]-g->uid) > 1)  {
                    gate_remove_neighbor (g, g->ng_names[i]);
                    done = FALSE;
                    break;
                }
            }
        }
    }
}

void gates_save_layout (GQueue *gates, const char *mode, const char *format, const char *filename)
{
    Agraph_t *graph;
    GVC_t* context;

    if (!gates || g_queue_is_empty (gates))
        return;

    // create context (automatically calls aginit())
    context = gvContext();

    // open a graph
    graph = agopen((char*)"map", AGRAPHSTRICT); // strict, directed graph

    // add all gates to the graph
    //
    for (GList *iter=g_queue_peek_tail_link (gates);iter;iter=g_list_previous (iter)) {
        gate_t *gate = (gate_t*)iter->data;
        char name[5];
        sprintf (name, "%d", gate->uid);
        printf ("creating node with name %s\n", name);
        agnode (graph, name);
    }

    // create edges
    int count=0;
    for (GList *iter=g_queue_peek_tail_link (gates);iter;iter=g_list_previous (iter)) {
        gate_t *gate = (gate_t*)iter->data;
        // find corresponding node
        char name[5];
        sprintf (name, "%d", gate->uid);
        Agnode_t *n1 = agfindnode (graph, name);
        if (!n1) {
            dbg (DBG_ERROR, "failed to retrieve node from name %s", name);
            continue;
        }
        printf ("adding %d edges to node %d\n", gate->ng, gate->uid);
        for (int j=0;j<gate->ng;j++) {
            sprintf (name, "%d", gate->ng_names[j]);
            Agnode_t *n2 = agfindnode (graph, name);
            if (!n2) {
                dbg (DBG_ERROR, "failed to retrieve node from name %s", name);
                continue;
            }
            agedge (graph, n1, n2);
            count++;
        }
    }

    printf ("created %d edges.\n", count);

    // layout the graph
    // choices are: manual, fdp, dot, neato, twopi, circo
    dbg (DBG_CLASS, "rendering graph with graphviz. mode = %s", mode);

    if (strcmp (mode, "fdp")) {
        gvLayout (context, graph, (char*)"fdp");
    } 
    if (strcmp (mode, "dot")) {
        gvLayout (context, graph, (char*)"dot");
    }
    if (strcmp (mode, "neato")) {
        gvLayout (context, graph, (char*)"neato");
    }
    if (strcmp (mode, "twopi")) {
        gvLayout (context, graph, (char*)"twopi");
    }
    if (strcmp (mode, "circo")) {
        gvLayout (context, graph, (char*)"circo");
    }

    // render graph to file
    dbg (DBG_CLASS, "saving graph to file %s", filename);
    gvRenderFilename(context, graph, (char*)format, (char*)filename);
    dbg (DBG_CLASS, "done.");

    // free graph
    gvFreeLayout(context, graph);
    agclose (graph);
    gvFreeContext(context);
}

// input : similarity (correlation) matrix
// output: updated topological map
//
void gates_process_similarity_matrix (const char *filename, GQueue *gates, gboolean smooth, char *data_dir, gboolean demo, lcm_t *lcm)
{
    int size=0;
    double *hmat, *hmatrev;
    int *hmatind, *hmatindrev;
    double *corrmat, *corrmatrev;
    GQueue *components = g_queue_new ();
    char fname[256];

    // read the corr. matrix from file
    corrmat = class_corrmat_read (filename, &size);

    if (smooth) {
        // run filtering
        class_corrmat_smoothen (corrmat, size);
        // save corr. matrix to file
        class_corrmat_write (corrmat, size, "corrmat_smooth.txt");
    }

    // reverse the correlation matrix
    corrmatrev = class_corrmat_copy (corrmat, size);
    class_corrmat_reverse (corrmatrev, size);
    class_corrmat_write (corrmatrev, size, "corrmatrev.txt");

    // compute alignment matrix
    class_compute_alignment_matrix (corrmat, size, .020, .1, FALSE, &hmat, &hmatind);
    class_compute_alignment_matrix (corrmatrev, size, .020, .1, TRUE, &hmatrev, &hmatindrev);
    class_corrmat_reverse (hmatrev, size);
    class_corrmat_int_reverse (hmatindrev, size);

    // write alignment matrix to file
    class_corrmat_write (hmat, size, "hmat.txt");
    class_corrmat_int_write (hmatind, size, "hmatind.txt");
    class_corrmat_write (hmatrev, size, "hmatrev.txt");
    class_corrmat_int_write (hmatindrev, size, "hmatindrev.txt");

    // find components
    unsigned int maxcomponents = 50;
    double *mat=NULL;
    class_corrmat_find_component_list (hmat, hmatind, hmatrev, hmatindrev, size, 1.0, components, maxcomponents, &mat);

    int alias=-1;
    if (!demo) {
        GQueue *new_gates = gates_create_from_component_list (gates, mat, size, -1, &alias);
        gates_cleanup (new_gates);
        sprintf (fname, "class.gate");
        dbg (DBG_CLASS, "saving gates to %s", fname);
        FILE *fp = fopen (fname, "w");
        navlcm_gates_write_to_file (new_gates, fp);
        fclose (fp);
    } else {
        // process the similarity matrix gate by gate
        for (int j=1;j<size;j++) {
            // convert components to a graph
            alias = -1;
            GQueue *new_gates = gates_create_from_component_list (gates, mat, size, j, &alias);

            // cleanup gates
            gates_cleanup (new_gates);

            // save gates to file
            sprintf (fname, "%s/class-%d.gates", data_dir, j);
            dbg (DBG_CLASS, "saving gates to %s", fname);
            FILE *fp = fopen (fname, "w");
            navlcm_gates_write_to_file (new_gates, fp);
            fclose (fp);

            navlcm_class_state_t state;

            if (lcm) {
                state.gates_filename = fname;
                state.current_gate_uid = alias;
                dbg (DBG_CLASS, "publishing class state with filename %s and current gate ID = %d", state.gates_filename, state.current_gate_uid);
                navlcm_class_state_t_publish (lcm, "CLASS_STATE", &state);
            }
            navlcm_gates_destroy (gates);

            usleep (2000000);
        }
    }
}

// input: set of gates
// output: similarity matrix
//
void gates_compute_similarity_matrix (GQueue *gates)
{
    GNode *tree = NULL;
    int count=0;
    GList *el = NULL;
    double *corrmat;
    double bag_radius = 10.0 * sqrt(128.0);
    unsigned int N = 10;
    int maxcount = -1;
    int ngates = g_queue_get_length (gates);
    GQueue *bags = g_queue_new ();

    // correlation matrix
    corrmat = corrmat_init (ngates, .0);

    // for each gate...
    el = g_queue_peek_tail_link (gates);
    while (el) {
        gate_t *gate = (gate_t*)el->data;
        navlcm_feature_list_t *fs = gate_features (gate);
        printf ("gate uid: %d\n", gate->uid);

        if (fs) {
#if 0
            tree_unit_testing (self->bags, &self->tree, fs, N, bag_radius);
#else
            if (!tree) tree = tree_init_from_void (128);

            // vote for closest gate so far
            bag_t_vote_for_indices (bags, fs, tree, N, bag_radius, gate->uid, corrmat, ngates);

            bag_tree_t_insert_feature_list (NULL, tree, fs, N, bag_radius, gate->uid);

#endif
        }

        if (tree)
            tree_print_info (tree);

        el = el->prev;
        count++;

        if (maxcount != -1 && count>maxcount)
            break;
    }

    // print out the correlation matrix
    class_corrmat_write (corrmat, ngates, "corrmat.txt");

}

/* input:   correlation matrix
 *          size of the matrix
 *          list of bags
 *          <norm> the normal factor for the new matrix row
 * output:  new correlation matrix
 */
double * gates_update_correlation_matrix (double *corrmat, int *size, GQueue *bags, double norm)
{
    if (!corrmat) {
        *size = 1;
        return corrmat_init (1, .0);
    } 
    
    double *m = corrmat_resize_once (corrmat, size, .0);
    
    double *sim = bags_naive_vote (bags, *size);

    // normalize
    vect_normalize (sim, *size, norm);

    // smoothen
//    double *sims = math_smooth_double  (sim, *size, 3);

    // cut the diagonal
    int diagsize = 1;
    for (int i=0;i<diagsize;i++) {
        if (i <= *size - 1)
            sim[*size-1-i] = .0;
    }

    printf ("sim vector: \n");
    for (int i=0;i<*size;i++)
        printf ("%.4f ", sim[i]);
    printf ("\n");

    for (int i=0;i<*size;i++) {
        CORRMAT_SET (m, *size, *size-1, i, sim[i]);
    }

    free (sim);

    return m;

}

double* gates_loop_closure_update (GQueue *voctree, double *corrmat, int *corrmat_size, navlcm_feature_list_t *features, int id, double bag_word_radius)
{
    // init vocabulary
    if (g_queue_is_empty (voctree)) {
        bags_init (voctree, features, id);
    }

    // search features in vocabulary tree
    GQueue *qfeatures = g_queue_new (); // features that are un-matched (and yield new bags)
    GQueue *qbags = g_queue_new ();     // bags that found a match
    bags_naive_search (voctree, features, bag_word_radius, qfeatures, qbags);

    // the ratio of matched features
    double r = 1.0 - 1.0 * g_queue_get_length (qfeatures) / features->num;

    // update similarity matrix
    double *newmat = gates_update_correlation_matrix (corrmat, corrmat_size, qbags, r);

    // print out
    //corrmat_print (newmat, *corrmat_size);

    //corrmat_print (corrmat, corrmat_size);
    class_corrmat_write (newmat, *corrmat_size, "corrmat.dat");

    // create new bags for unmatched features
    bags_append (voctree, qfeatures, id);
    g_queue_free (qfeatures);

    // update the list of indices for the matched bags
    bags_update (qbags, id);
    g_queue_free (qbags);

    return newmat;
}

/* publish the list of gate labels
*/
void gates_publish_labels (GQueue *gates, lcm_t *lcm, const char *channel)
{
    navlcm_dictionary_t d;
    d.num=0;
    d.keys=NULL;
    d.vals=NULL;

    if (!gates) return;

    // convert gate labels to a dictionary structure
    for (GList *iter=g_queue_peek_head_link(gates);iter;iter=iter->next) {
        gate_t *g = (gate_t*)iter->data;
        if (!g->label) continue;
        if (strlen (g->label) < 2) continue;
        d.keys = (char**)realloc(d.keys, (d.num+1)*sizeof(char*));
        d.vals = (char**)realloc(d.vals, (d.num+1)*sizeof(char*));
        d.keys[d.num] = (char*)malloc(8);
        sprintf ( d.keys[d.num], "%d", g->uid);
        d.vals[d.num] = g->label;
        d.num++;
    }

    if (d.num > 0) {
        dbg (DBG_CLASS, "publishing labels...");
        navlcm_dictionary_t_publish (lcm, channel, &d);
    }

    // free
    for (int i=0;i<d.num;i++)
        free (d.keys[i]);

    free (d.keys);
    free (d.vals);
}

/* find the closest gate with a label
*/
int gates_closest_with_label (GQueue *gates, gate_t *gin)
{
    if (!gates || !gin) return -1;

    gate_t *bg = NULL;
    int bd = -1;

    // convert gate labels to a dictionary structure
    for (GList *iter=g_queue_peek_head_link(gates);iter;iter=iter->next) {
        gate_t *g = (gate_t*)iter->data;
        if (bd == -1 || abs (g->uid - gin->uid) < bd) {
            bd =  abs (g->uid - gin->uid);
            bg = g;
        }
    }

    if (bg)
        return bg->uid;

    return -1;
}

int gates_max_id (GQueue *gates)
{
    int maxid = 0;
    for (GList *iter=g_queue_peek_head_link(gates);iter;iter=iter->next) {
        gate_t *g = (gate_t*)iter->data;
        if (maxid < g->uid)
            maxid = g->uid;
    }
    return maxid;
}

int gates_min_id (GQueue *gates)
{
    int minid = -1;
    for (GList *iter=g_queue_peek_head_link(gates);iter;iter=iter->next) {
        gate_t *g = (gate_t*)iter->data;
        if (minid == -1 || minid > g->uid)
            minid = g->uid;
    }
    return minid;
}

