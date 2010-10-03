#include "dijkstra.h"

#define DIJK_DEBUG 1

int MOTION_TYPE_REVERSE (int motion_type)
{
    if (motion_type == NAVLCM_CLASS_PARAM_T_MOTION_TYPE_LEFT)
        return NAVLCM_CLASS_PARAM_T_MOTION_TYPE_RIGHT;
    if (motion_type == NAVLCM_CLASS_PARAM_T_MOTION_TYPE_RIGHT)
        return NAVLCM_CLASS_PARAM_T_MOTION_TYPE_LEFT;
    if (motion_type == NAVLCM_CLASS_PARAM_T_MOTION_TYPE_UP)
        return NAVLCM_CLASS_PARAM_T_MOTION_TYPE_DOWN;
    if (motion_type == NAVLCM_CLASS_PARAM_T_MOTION_TYPE_DOWN)
        return NAVLCM_CLASS_PARAM_T_MOTION_TYPE_UP;

    return motion_type;
}

dijk_edge_t *dijk_edge_new (navlcm_feature_list_t *f, GQueue *img, botlcm_image_t *up_img, botlcm_pose_t *pose, navlcm_gps_to_local_t *gps, gboolean reverse, dijk_node_t *start, dijk_node_t *end, int motion_type)
{
    dijk_edge_t *e = (dijk_edge_t*)malloc(sizeof(dijk_edge_t));

    e->features = f;
    e->img = img;
    e->up_img = up_img;
    e->pose = pose;
    e->gps_to_local = gps;
    e->start = start;
    e->end = end;
    e->timestamp = 0;
    e->reverse = reverse;
    e->motion_type = motion_type;

    if (e->start) 
        g_queue_push_tail (e->start->edges, e);

    return e;
}

dijk_edge_t *dijk_edge_new_with_copy (navlcm_feature_list_t *f, GQueue *img, botlcm_image_t *up_img, botlcm_pose_t *pose, navlcm_gps_to_local_t *gps, gboolean reverse, dijk_node_t *start, dijk_node_t *end, int motion_type)
{
    navlcm_feature_list_t *f_copy = f ? navlcm_feature_list_t_copy (f) : NULL;
    botlcm_image_t *up_img_copy = up_img ? botlcm_image_t_copy (up_img) : NULL;
    botlcm_pose_t *pose_copy = pose ? botlcm_pose_t_copy (pose) : NULL;
    navlcm_gps_to_local_t *gps_copy = gps ? navlcm_gps_to_local_t_copy (gps) : NULL;
    GQueue *img_copy = NULL;
    if (img) {
        img_copy = g_queue_new ();
        for (GList *iter=g_queue_peek_head_link (img);iter;iter=iter->next) {
            g_queue_push_tail (img_copy, botlcm_image_t_copy ((botlcm_image_t*)iter->data));
        }
    }

    return dijk_edge_new (f_copy, img_copy, up_img_copy, pose_copy, gps_copy, reverse, start, end, motion_type);
}

dijk_node_t *dijk_node_new (int uid, gboolean checkpoint, int64_t utime)
{
    dijk_node_t *n = (dijk_node_t*)malloc (sizeof(dijk_node_t));

    n->label = NULL;
    n->timestamp = 0;
    n->dist = .0;
    n->edges = g_queue_new ();
    n->previous = NULL;
    n->uid = uid;
    n->checkpoint = checkpoint;
    n->utime = utime;
    n->pdf0 = .0;
    n->pdf1 = .0;

    return n;
}

dijk_graph_t *dijk_graph_new ()
{
    dijk_graph_t *d = (dijk_graph_t*)malloc(sizeof(dijk_graph_t));
    dijk_graph_init (d);

    return d;
}

void dijk_graph_add_new_node (dijk_graph_t *g, navlcm_feature_list_t *f, GQueue *img, botlcm_image_t *up_img, botlcm_pose_t *pose, navlcm_gps_to_local_t *gps, gboolean checkpoint, dijk_edge_t *current_edge, int motion_type)
{
    int64_t utime = f->utime;
    int uid = g_queue_get_length (g->nodes);

    if (g_queue_is_empty (g->nodes)) {
        dijk_node_t *n = dijk_node_new (uid, checkpoint, utime);
        dijk_edge_t *e = dijk_edge_new_with_copy (f, img, up_img, pose, gps, 0, n, NULL, motion_type);

        dijk_graph_insert_node (g, n);
        dijk_graph_insert_edge (g, e);
    } else {
        dijk_node_t *n = dijk_node_new (uid, checkpoint, utime);
        current_edge->end = n;
        dijk_edge_t *e1 = dijk_edge_new_with_copy (f, img, up_img, pose, gps, 1, n, current_edge->start, MOTION_TYPE_REVERSE (motion_type));
        dijk_edge_t *e2 = dijk_edge_new_with_copy (f, img, up_img, pose, gps, 0, n, NULL, motion_type);

        dijk_graph_insert_node (g, n);
        dijk_graph_insert_edge (g, e1);
        dijk_graph_insert_edge (g, e2);
    }
}

dijk_edge_t *dijk_graph_latest_edge (dijk_graph_t *dg)
{
    if (g_queue_is_empty (dg->edges))
        return NULL;

    return (dijk_edge_t*)g_queue_peek_tail (dg->edges);
}

dijk_edge_t *dijk_graph_find_edge_by_id (dijk_graph_t *dg, int id0, int id1)
{
    dijk_node_t *n0 = dijk_graph_find_node_by_id (dg, id0);
    dijk_node_t *n1 = dijk_graph_find_node_by_id (dg, id1);

    if (!n0 || !n1) return NULL;

    return dijk_node_find_edge (n0, n1);
}

/* find a node by ID
*/
dijk_node_t *dijk_graph_find_node_by_id (dijk_graph_t *dg, int id)
{
    /*
       for (GList *iter=g_queue_peek_head_link (nodes);iter;iter=g_list_next(iter)) {
       dijk_node_t *nd = (dijk_node_t*)iter->data;
       if (nd->g->uid == id) return nd;
       }
       */
    gpointer data = g_hash_table_lookup (dg->alias, &id);
    if (!data) return NULL;
    int *data_int = (int*)data;
    gpointer d = g_queue_peek_nth (dg->nodes, *data_int);
    if (!d) {
        printf ("mmm... failed to find node %d at position %d\n", id, *data_int);
    }
    assert (d);

    return (dijk_node_t*)d;
}

dijk_edge_t *dijk_get_nth_edge (dijk_node_t *n, int p)
{
    gpointer data = g_queue_peek_nth (n->edges, p);
    if (!data)
        return NULL;
    return (dijk_edge_t*)data;
}

navlcm_feature_list_t *dijk_node_get_nth_features (dijk_node_t *n, int p)
{
    dijk_edge_t *e = dijk_get_nth_edge (n, p);
    if (!e)
        return NULL;
    return e->features;
}

/* init a graph
*/
void dijk_graph_init (dijk_graph_t *dg)
{
    dg->nodes = g_queue_new ();
    dg->edges = g_queue_new ();
    dg->alias = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, g_free);
}

/* deep copy
*/
dijk_graph_t *dijk_graph_copy (dijk_graph_t *dg)
{
    dijk_graph_t *g = (dijk_graph_t*)malloc(sizeof(dijk_graph_t));
    dijk_graph_init (g);

    for (GList *iter=g_queue_peek_head_link (dg->nodes);iter;iter=iter->next) {
        dijk_node_t *n = (dijk_node_t*)iter->data;
        dijk_node_t *n2 = dijk_node_new (n->uid, n->checkpoint, n->utime);
        dijk_graph_insert_node (g, n2);
    }

    for (GList *iter=g_queue_peek_head_link (dg->edges);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;
        dijk_node_t *n1 = e->start ? dijk_graph_find_node_by_id (g, e->start->uid) : NULL;
        dijk_node_t *n2 = e->end ? dijk_graph_find_node_by_id (g, e->end->uid) : NULL;
        dijk_edge_t *e2 = dijk_edge_new_with_copy (e->features, e->img, e->up_img, e->pose, e->gps_to_local, e->reverse, n1, n2, e->motion_type);
        g_queue_push_tail (g->edges, e2);
    }

    // sanity check
    for (GList *iter=g_queue_peek_head_link (g->nodes);iter;iter=iter->next) {
        dijk_node_t *n = (dijk_node_t*)iter->data;
        GQueue *n2n = dijk_node_neighbors (n);
        for (GList *iter1=g_queue_peek_head_link (n2n);iter1;iter1=iter1->next) {
            for (GList *iter2=iter1->next;iter2;iter2=iter2->next) {
                if (iter1->data && iter2->data) {
                    printf ("%d %d\n", ((dijk_node_t*)iter1->data)->uid, ((dijk_node_t*)iter2->data)->uid);
                    assert ((dijk_node_t*)iter1->data != (dijk_node_t*)iter2->data);
                }
            }
        }
        g_queue_free (n2n);
    }

    return g;
}

void dijk_graph_insert_nodes (dijk_graph_t *dg, GQueue *nodes)
{
    if (nodes) {

        // create queue of nodes
        for (GList *iter=g_queue_peek_head_link (nodes);iter;iter=iter->next) {

            dijk_node_t *n = (dijk_node_t*)iter->data;
            dijk_graph_insert_node (dg, n);
        }
        dbg (DBG_CLASS, "[dijk] init %d nodes.", g_queue_get_length (dg->nodes));
    }
}

void dijk_graph_insert_node (dijk_graph_t *dg, dijk_node_t *nd) 
{
    assert (nd);

    nd->dist = -1; // infinity
    nd->previous = NULL;
    nd->timestamp = 0;

    g_hash_table_insert (dg->alias, g_intdup (nd->uid), g_intdup (g_queue_get_length (dg->nodes)));
    g_queue_push_tail (dg->nodes, nd);
}

gboolean dijk_graph_remove_node (dijk_graph_t *dg, dijk_node_t *n)
{
    g_queue_remove (dg->nodes, n);
    gpointer data = g_hash_table_lookup (dg->alias, &n->uid);
    if (!data) return FALSE;
    int *data_int = (int*)data;
    int index = (int)*data_int;

    g_hash_table_remove (dg->alias, &n->uid);

    // update index in hash table
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init (&iter, dg->alias);
    while (g_hash_table_iter_next (&iter, &key, &value)) 
    {
        int *k = (int*)key;
        int *v = (int*)value;
        if (*v > index) {
            *v = *v-1;
        }
    }
    return TRUE;
}

void dijk_graph_remove_edge (dijk_graph_t *dg, dijk_edge_t *e)
{
    g_queue_remove (dg->edges, e);
}

void dijk_graph_insert_edge (dijk_graph_t *dg, dijk_edge_t *e)
{
    g_queue_push_tail (dg->edges, e);
}

void dijk_node_set_label (dijk_node_t *n, const char *txt)
{
    if (n->label)
        free (n->label);

    n->label = strdup (txt);
}

/* destroy a node
*/
void dijk_node_destroy (dijk_node_t *nd)
{
    if (nd->label)
        free (nd->label);
    g_queue_free (nd->edges);

    free (nd);
}

/* destroy an edge
*/
void dijk_edge_destroy (dijk_edge_t *e)
{
    if (e->features)
        navlcm_feature_list_t_destroy (e->features);
    if (e->img) {
        for (GList *iter=g_queue_peek_head_link (e->img);iter;iter=iter->next) {
            botlcm_image_t *img = (botlcm_image_t*)iter->data;
            botlcm_image_t_destroy (img);
        }
        g_queue_free (e->img);
    }
    if (e->up_img)
        botlcm_image_t_destroy (e->up_img);
    if (e->pose)
        botlcm_pose_t_destroy (e->pose);
    if (e->gps_to_local)
        navlcm_gps_to_local_t_destroy (e->gps_to_local);
}

/* destroy a dijkstra's graph
*/
void dijk_graph_destroy (dijk_graph_t *dg)
{
    if (dg->nodes) {
        for (GList *iter=g_queue_peek_head_link(dg->nodes);iter;iter=iter->next) {
            dijk_node_t *nd = (dijk_node_t*)iter->data;
            dijk_node_destroy (nd);
        }
        g_queue_free (dg->nodes);
        dg->nodes = NULL;
    }
    if (dg->edges) {
        for (GList *iter=g_queue_peek_head_link(dg->edges);iter;iter=iter->next) {
            dijk_edge_t *nd = (dijk_edge_t*)iter->data;
            dijk_edge_destroy (nd);
        }
        g_queue_free (dg->edges);
        dg->edges = NULL;
    }

    if (dg->alias) {
        g_hash_table_destroy (dg->alias);
        dg->alias = NULL;
    }
}


/* find the node with smallest distance (linear search)
*/
dijk_node_t *dijk_find_node_smallest_dist (GQueue *nodes)
{
    dijk_node_t *bnd = NULL;
    int mindist = -1;

    for (GList *iter=g_queue_peek_head_link (nodes);iter;iter=iter->next) {
        dijk_node_t *nd = (dijk_node_t*)iter->data;
        if (nd->dist == -1) continue;
        if (mindist == -1 || nd->dist < mindist) {
            bnd = nd;
            mindist = nd->dist;
        }
    }

    return bnd;
}

dijk_edge_t *dijk_find_edge (GQueue *edges, dijk_node_t *n)
{
    for (GList *iter=g_queue_peek_head_link(edges);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;
        if (e->start == n)
            return e;
    }
    return NULL;
}

dijk_edge_t *dijk_find_edge_full (GQueue *edges, dijk_node_t *n1, dijk_node_t *n2)
{
    for (GList *iter=g_queue_peek_head_link(edges);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;
        if (e->start == n1 && e->end == n2)
            return e;
    }
    return NULL;
}

/* distance between two nodes
*/
int dijk_node_dist (dijk_node_t *n1, dijk_node_t *n2)
{
    return 1;
}

GQueue *dijk_node_neighbors (dijk_node_t *nd)
{
    GQueue *q = g_queue_new ();

    for (GList *iter=g_queue_peek_head_link (nd->edges);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;
        if (e->start == nd && e->end)
            g_queue_push_tail (q, e->end);
    }

    return q;
}

GQueue *dijk_edge_neighbors (dijk_edge_t *nd)
{
    GQueue *q = g_queue_new ();

    for (GList *iter=g_queue_peek_head_link (nd->start->edges);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;
        g_queue_push_tail (q, e);
    }

    for (GList *iter=g_queue_peek_head_link (nd->end->edges);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;
        if (e->end != nd->start)
            g_queue_push_tail (q, e);
    }

    return q;
}

dijk_edge_t *dijk_node_find_edge (dijk_node_t *n1, dijk_node_t *n2)
{
    if (!n1) 
        return NULL;

    for (GList *iter=g_queue_peek_head_link (n1->edges);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;
        if (e->end == n2)
            return e;
    }
    return NULL;
}

gboolean dijk_node_has_neighbor (dijk_node_t *n1, dijk_node_t *n2) 
{
    if (dijk_node_find_edge (n1, n2))
        return TRUE;

    return FALSE;
}

int dijk_node_degree (dijk_node_t *n)
{
    return g_queue_get_length (n->edges);
}

/* recursively expand a tree.
*/
void dijk_expand_tree_rec (GNode *root, GNode *node, int radius, int64_t timestamp)
{
    if (radius == 0) return;

    dijk_node_t *nd = (dijk_node_t*)node->data;

    GQueue *neighbors = dijk_node_neighbors (nd);

    // add neighbors as children
    for (GList *iter=g_queue_peek_head_link (neighbors);iter;iter=iter->next) {
        dijk_node_t *n = (dijk_node_t*)iter->data;
        if (n->timestamp == timestamp) continue;
        g_node_insert_data (node, -1, n);
        n->timestamp = timestamp; // mark as visited
    }
    g_queue_free (neighbors);

    // process children
    for (GNode *child=g_node_first_child (node);child;child = child->next) {
        dijk_expand_tree_rec (root, child, radius == -1 ? -1 : radius-1, timestamp);
    }
}

/* convert a graph to a tree.
 * stop at given radius (-1 to never stop)
 */
GNode* dijk_to_tree (dijk_node_t *nd, int radius) 
{
    // apply timestamp
    nd->timestamp = bot_timestamp_now ();

    GNode *tree = g_node_new (nd);

    dijk_expand_tree_rec (tree, tree, radius, nd->timestamp);

    return tree;
}

/* recursively expand a tree.
*/
void dijk_expand_dual_tree_rec (GNode *root, GNode *node, int radius, int64_t timestamp)
{
    if (radius == 0) return;

    dijk_edge_t *nd = (dijk_edge_t*)node->data;

    GQueue *neighbors = dijk_edge_neighbors (nd);

    // add neighbors as children
    for (GList *iter=g_queue_peek_head_link (neighbors);iter;iter=iter->next) {
        dijk_edge_t *n = (dijk_edge_t*)iter->data;
        if (n->timestamp == timestamp) continue;
        g_node_insert_data (node, -1, n);
        n->timestamp = timestamp; // mark as visited
    }
    g_queue_free (neighbors);

    // process children
    for (GNode *child=g_node_first_child (node);child;child = child->next) {
        dijk_expand_dual_tree_rec (root, child, radius == -1 ? -1 : radius-1, timestamp);
    }
}

/* convert a graph to a dual tree (nodes in the tree are edges in the graph)
 * stop at given radius (-1 to never stop)
 */
GNode* dijk_to_dual_tree (dijk_edge_t *ed, int radius) 
{
    // apply timestamp
    ed->timestamp = bot_timestamp_now ();

    GNode *tree = g_node_new (ed);

    dijk_expand_dual_tree_rec (tree, tree, radius, ed->timestamp);

    return tree;
}

void dijk_edge_print (dijk_edge_t *e)
{
    printf ("edge %d %d\n", e->start ? e->start->uid : -1, e->end ? e->end->uid : -1);
}

void dijk_graph_print (dijk_graph_t *dg)
{
    int nnodes = g_queue_get_length (dg->nodes);
    int nedges = g_queue_get_length (dg->edges);

    printf ("********* graph [%d,%d] ************\n", nnodes, nedges);

    for (GList *iter=g_queue_peek_head_link (dg->nodes);iter;iter=iter->next) {
        dijk_node_t *n = (dijk_node_t*)iter->data;
        printf ("node [%d]\t%d edges:", n->uid, g_queue_get_length (n->edges));
        for (GList *eiter=g_queue_peek_head_link(n->edges);eiter;eiter=eiter->next) {
            dijk_edge_t *e = (dijk_edge_t*)eiter->data;
            printf ("[%d\t%d\t%d\t%d] ", e->start ? e->start->uid : -1, e->end ? e->end->uid : -1, e->reverse, e->motion_type);
        }
        printf ("\n");
    }
    for (GList *iter=g_queue_peek_head_link (dg->edges);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;
        printf ("edge [%d]\t[%d]\trev: %d\ttype:%d\n", e->start ? e->start->uid : -1, e->end ? e->end->uid : -1, e->reverse, e->motion_type);
    }
}

/* print callback
*/
gboolean dijk_tree_print_cb (GNode *node, gpointer data)
{
    dijk_node_t *nd = (dijk_node_t*)node->data;

    printf ("[%d] ", nd->uid);

    for (GList *iter=g_queue_peek_head_link (nd->edges);iter;iter=iter->next)
        printf ("%d ", ((dijk_edge_t*)iter->data)->end->uid);

    if (G_NODE_IS_ROOT (node) || node == g_node_last_child (node->parent))
        printf ("\n");

    return FALSE;
}

/* print out a tree
*/
void dijk_tree_print (GNode *node)
{
    printf ("[%d] nodes, [%d] max height\n", g_node_n_nodes (node, G_TRAVERSE_ALL), g_node_max_height (node));

    g_node_traverse (node, G_PRE_ORDER, G_TRAVERSE_ALL, -1, dijk_tree_print_cb, NULL);
}

/* find the shortest path between two nodes
 * output: a queue of edges
 */
GQueue *dijk_find_shortest_path (dijk_graph_t *dg, dijk_node_t *src, dijk_node_t *dst)
{
    GTimer *timer = g_timer_new ();

    dbg (DBG_CLASS, "searching for path between node %d and node %d", src->uid, dst->uid);

    // set all distances to infinity and previous links to null
    for (GList *iter=g_queue_peek_head_link (dg->nodes);iter;iter=iter->next) {
        dijk_node_t *nd = (dijk_node_t*)iter->data;
        nd->dist = -1;
        nd->previous = NULL;
        nd->timestamp = 0;
    }

    // set distance of source node to 0
    src->dist = 0;

    // work on a shallow copy
    GQueue *nodes = g_queue_copy (dg->nodes);

    int64_t timestamp = bot_timestamp_now ();

    // main loop
    while (!g_queue_is_empty (nodes)) {

        // find node with smallest distance
        dijk_node_t *nd = dijk_find_node_smallest_dist (nodes);

        // all remaining vertices are inaccessible
        if (nd->dist == -1) break;

        // stop if target is reached
        if (nd == dst) break;

        // mark as visited
        nd->timestamp = timestamp;

        // remove node from queue
        g_queue_remove (nodes, nd);

        // for each neighbor
        GQueue *neighbors = dijk_node_neighbors (nd);
        for (GList *iter=g_queue_peek_head_link(neighbors);iter;iter=iter->next) {
            dijk_node_t *nc = (dijk_node_t*)iter->data;
            if (nc->timestamp != timestamp) {
                int alt = nd->dist + dijk_node_dist (nd, nc);
                if (nc->dist == -1 || alt < nc->dist) {
                    nc->dist = alt;
                    nc->previous = nd;
                }
            }
        }
        g_queue_free (neighbors);
    }

    g_queue_free (nodes);

    dbg (DBG_CLASS, "[dijk] elapsed time (search): %.3f secs.", g_timer_elapsed (timer, NULL));

    // generate path
    nodes = g_queue_new ();
    dijk_node_t *nd = dst;
    assert (nd);

    while (1) {
        g_queue_push_head (nodes, nd);
        if (nd->previous) {
            nd = (dijk_node_t*)nd->previous;
        } else 
            break;
    }

    GQueue *path = g_queue_new ();

    // convert node series into edge series
    for (GList *iter=g_queue_peek_head_link(nodes);iter;iter=iter->next) {
        if (iter->next) {
            dijk_node_t *n1 = (dijk_node_t*)iter->data;
            dijk_node_t *n2 = (dijk_node_t*)iter->next->data;
            dijk_edge_t *e = dijk_node_find_edge (n1, n2);
            assert (e);
            assert (e->start == n1);
            assert (e->end == n2);
            g_queue_push_tail (path, e);
        }
    }

    g_queue_free (nodes);

    // print path
    /*
    dbg (DBG_CLASS, "found shortest path length %d between gate %d and gate %d.",
            g_queue_get_length (path), src->uid, dst->uid);

    for (GList *iter=g_queue_peek_head_link(path);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;
        printf ("[%d %d (%d)] ", e->start->uid, e->end->uid, e->motion_type);
    }
    printf ("\n");
    */

    if (!g_queue_is_empty (path)) {
        // check path sanity
        assert (((dijk_edge_t*)(g_queue_peek_head(path)))->start == src);
        assert (((dijk_edge_t*)(g_queue_peek_tail(path)))->end == dst);
    }

    dbg (DBG_CLASS, "[dijk] elapsed time (path gen.): %.3f secs.", g_timer_elapsed (timer, NULL));
    g_timer_destroy (timer);

    return path;
}

int dijk_graph_n_nodes (dijk_graph_t *dg)
{
    if (!dg) return 0;

    return g_queue_get_length (dg->nodes);
}

/* Tests the Dijkstra's algorithm on random graphs
*/
void dijk_unit_testing ()
{
    int N = 10000;
    int src_id, dst_id;

    // create graph
    dijk_graph_t *g = dijk_graph_new ();

    dbg (DBG_CLASS, "inserting %d nodes...", N);

    for (int i=0;i<N;i++) {
        dijk_node_t *n = dijk_node_new (i, 0, 0);
        dijk_graph_insert_node (g, n);
    }

    dbg (DBG_CLASS, "creating chain...");
    // create a chain
    for (GList *iter=g_queue_peek_head_link(g->nodes);iter;iter=iter->next) {
        dijk_node_t *n1 = (dijk_node_t*)iter->data;
        if (iter->next) {
            dijk_node_t *n2 = (dijk_node_t*)iter->next->data;
            dijk_edge_t *e1 = dijk_edge_new (NULL, NULL, NULL, NULL, NULL, 0, n1, n2, -1);
            dijk_edge_t *e2 = dijk_edge_new (NULL, NULL, NULL, NULL, NULL, 1, n2, n1, -1);
            dijk_graph_insert_edge (g, e1);
            dijk_graph_insert_edge (g, e2);
        }
    }

    dbg (DBG_CLASS, "created %d nodes and %d edges.", g_queue_get_length (g->nodes), g_queue_get_length (g->edges));

    dbg (DBG_CLASS, "adding random edges...");

    // add random edges
    srand (time (NULL));
    for (int i=0;i<N/4;i++) {
        src_id = (int)(1.0*rand()/RAND_MAX*(N-1));
        dst_id = (int)(1.0*rand()/RAND_MAX*(N-1));
        if (src_id != dst_id) {
            dijk_node_t *n1 = (dijk_node_t*)g_queue_peek_nth (g->nodes, src_id);
            dijk_node_t *n2 = (dijk_node_t*)g_queue_peek_nth (g->nodes, dst_id);
            if (!dijk_node_has_neighbor (n1, n2)) {
                dijk_edge_t *e1 = dijk_edge_new (NULL, NULL, NULL, NULL, NULL, 0, n1, n2, -1);
                dijk_edge_t *e2 = dijk_edge_new (NULL, NULL, NULL, NULL, NULL, 1, n2, n1, -1);
                dijk_graph_insert_edge (g, e1);
                dijk_graph_insert_edge (g, e2);
            }
        }
    }

    dbg (DBG_CLASS, "graph now has %d edges.", g_queue_get_length (g->edges));

    // select source and target randomly
    src_id = (int)(1.0*rand()/RAND_MAX*(N-1));
    dst_id = (int)(1.0*rand()/RAND_MAX*(N-1));
    while (dst_id == src_id) {
        dst_id = (int)(1.0*rand()/RAND_MAX*(N-1));
    }

    dijk_node_t *src = (dijk_node_t*)g_queue_peek_nth (g->nodes, src_id);
    dijk_node_t *dst = (dijk_node_t*)g_queue_peek_nth (g->nodes, dst_id);

    dbg (DBG_CLASS, "searching for path between %d and %d", src->uid, dst->uid);

    // search for path
    GQueue *path = dijk_find_shortest_path (g, src, dst);

    g_queue_free (path);

    dijk_graph_destroy (g);
}

int dijk_graph_max_node_id (dijk_graph_t *dg)
{
    int maxid = -1;
    for (GList *iter=g_queue_peek_head_link (dg->nodes);iter;iter=iter->next) {
        dijk_node_t *n = (dijk_node_t*)iter->data;
        if (maxid < n->uid)
            maxid = n->uid;
    }
    return maxid;
}

void dijk_graph_write_to_file (dijk_graph_t *dg, const char *filename)
{
    FILE *fp = fopen (filename, "wb");
    if (!fp)
        return;

    dijk_graph_write (dg, fp);

    fclose (fp);

    dbg (DBG_CLASS, "wrote graph to file %s", filename);

}
void dijk_graph_read_from_file (dijk_graph_t *dg, const char *filename)
{
    FILE *fp = fopen (filename, "rb");
    if (!fp)
        return;

    dijk_graph_read (dg, fp);

    fclose (fp);

    dbg (DBG_CLASS, "read graph from file %s", filename);
}

void dijk_graph_read (dijk_graph_t *dg, FILE *fp)
{
    int nnodes;
    assert (fread (&nnodes, sizeof(int), 1, fp)==1);

    // read nodes
    for (int i=0;i<nnodes;i++) {
        int uid, checkpoint, haslabel;
        int64_t utime;
        char label[256];

        fread (&uid, sizeof(int), 1, fp);
        fread (&checkpoint, sizeof(unsigned char), 1, fp);
        fread (&utime, sizeof(int64_t), 1, fp);

        dijk_node_t *n = dijk_node_new (uid, checkpoint, utime);

        int labelsize;
        fread (&labelsize, sizeof(int), 1, fp);

        if (labelsize) {
            n->label = (char*)malloc((labelsize+1));
            memset (n->label, '\0', labelsize+1);
            fread (n->label, sizeof(unsigned char), labelsize, fp);
        }

        fread (&n->pdf0, sizeof(double), 1, fp);
        fread (&n->pdf1, sizeof(double), 1, fp);

        dijk_graph_insert_node (dg, n);
    }

    // read edges
    int nedges = 0;
    assert (fread (&nedges, sizeof(int), 1, fp)==1);

    for (int i=0;i<nedges;i++) {

        navlcm_feature_list_t *f = (navlcm_feature_list_t*)malloc(sizeof(navlcm_feature_list_t));
        if (navlcm_feature_list_t_read (f, fp)<0) {
            free (f);
            f = NULL;
        }

        GQueue *images = g_queue_new ();
        int nimages = 0;
        fread (&nimages, sizeof(int), 1, fp);

        for (int j=0;j<nimages;j++) {
            botlcm_image_t *img = (botlcm_image_t*)malloc(sizeof(botlcm_image_t));
            if (botlcm_image_t_read (img, fp)<0) {
                free (img);
                img = NULL;
            }
            g_queue_push_tail (images, img);
        }

        botlcm_image_t *up_img = (botlcm_image_t*)malloc(sizeof(botlcm_image_t));
        if (botlcm_image_t_read (up_img, fp)<0) {
            free (up_img);
            up_img = NULL;
        } else {
        }

        botlcm_pose_t *pose = (botlcm_pose_t*)malloc(sizeof(botlcm_pose_t));
        if (botlcm_pose_t_read (pose, fp)<0) {
            free (pose);
            pose = NULL;
        } else {
        }

        navlcm_gps_to_local_t *gps = (navlcm_gps_to_local_t*)malloc(sizeof(navlcm_gps_to_local_t));
        if (navlcm_gps_to_local_t_read (gps, fp)<0) {
            free (gps);
            gps = NULL;
        } else {
        }

        unsigned char reverse;

        fread (&reverse, sizeof(unsigned char), 1, fp);

        int motion_type;
        fread (&motion_type, sizeof (int), 1, fp);

        int nodeid1, nodeid2;
        fread (&nodeid1, sizeof(int), 1, fp);
        fread (&nodeid2, sizeof(int), 1, fp);

        dijk_node_t *n1 = NULL;
        dijk_node_t *n2 = NULL;
        if (nodeid1 != -1)
            n1 = dijk_graph_find_node_by_id (dg, nodeid1);
        if (nodeid2 != -1)
            n2 = dijk_graph_find_node_by_id (dg, nodeid2);

        dijk_edge_t *e = dijk_edge_new (f, images, up_img, pose, gps, reverse, n1, n2, motion_type);

        dijk_graph_insert_edge (dg, e);
    }

    dbg (DBG_CLASS, "read graph with %d nodes and %d edges.", nnodes, nedges);
}

void dijk_graph_write (dijk_graph_t *g, FILE *fp)
{
    // write nodes
    int nnodes = g_queue_get_length (g->nodes);

    fwrite (&nnodes, sizeof (int), 1, fp);

    for (GList *iter=g_queue_peek_head_link (g->nodes);iter;iter=iter->next) {

        dijk_node_t *n = (dijk_node_t*)iter->data;

        fwrite (&n->uid, sizeof(int), 1, fp);
        fwrite (&n->checkpoint, sizeof(unsigned char), 1, fp);
        fwrite (&n->utime, sizeof(int64_t), 1, fp);

        int labelsize = n->label ? strlen (n->label) : 0;
        fwrite (&labelsize, sizeof(int), 1, fp);

        if (labelsize > 0)
            fwrite (n->label, sizeof(unsigned char), strlen (n->label), fp);

        fwrite (&n->pdf0, sizeof(double), 1, fp);
        fwrite (&n->pdf1, sizeof(double), 1, fp);
    }

    // write edges
    int nedges = g_queue_get_length (g->edges);

    fwrite (&nedges, sizeof(int), 1, fp);

    for (GList *iter=g_queue_peek_head_link (g->edges);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;

        navlcm_feature_list_t_write (e->features, fp);

        int nimages = g_queue_get_length (e->img);
        fwrite (&nimages, sizeof(int), 1, fp);

        for (GList *iter=g_queue_peek_head_link (e->img);iter;iter=iter->next) {
            botlcm_image_t *img = (botlcm_image_t*)iter->data;
            botlcm_image_t_write (img, fp);
        }

        botlcm_image_t_write (e->up_img, fp);
        botlcm_pose_t_write (e->pose, fp);
        navlcm_gps_to_local_t_write (e->gps_to_local, fp);

        fwrite (&e->reverse, sizeof(unsigned char), 1, fp);
        fwrite (&e->motion_type, sizeof(int), 1, fp);

        int nodeid1 = e->start ? e->start->uid : -1;
        int nodeid2 = e->end ? e->end->uid : -1;

        fwrite (&nodeid1, sizeof(int), 1, fp);
        fwrite (&nodeid2, sizeof(int), 1, fp);

    }

    dbg (DBG_CLASS, "wrote graph with %d nodes and %d edges.", g_queue_get_length (g->nodes),
            g_queue_get_length (g->edges));
}

void dijk_save_images_to_file (dijk_graph_t *dg, const char *dirname, gboolean overwrite)
{
    for (GList *iter=g_queue_peek_head_link (dg->edges);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;
        if (!e->start || !e->end)
            continue;

        char filename[256];
        sprintf (filename, "%s/%04d-%04d-%d.png", dirname, e->start->uid, e->end->uid, e->reverse);

        if (!overwrite && file_exists (filename))
            continue;

        image_stitch_image_to_file (e->img, filename);

        dbg (DBG_CLASS, "saved edge image to file %s", filename);
    }
}

gboolean dijk_graph_has_node (dijk_graph_t *dg, dijk_node_t *n)
{
    for (GList *iter=g_queue_peek_head_link (dg->nodes);iter;iter=iter->next) {
        dijk_node_t *t = (dijk_node_t*)iter->data;
        if (n == t)
            return TRUE;
    }

    for (GList *eiter=g_queue_peek_head_link (dg->edges);eiter;eiter=eiter->next) {
        dijk_edge_t *e = (dijk_edge_t*)eiter->data;
        if (e->start == n || e->end == n)
            return TRUE;
    }

    return FALSE;
}

void dijk_graph_remove_edge_by_id (dijk_graph_t *dg, int id1, int id2)
{
    dijk_node_t *n1 = dijk_graph_find_node_by_id (dg, id1);
    dijk_node_t *n2 = dijk_graph_find_node_by_id (dg, id2);

    gboolean ok = 0;

    // break link between n1 and n2
    dijk_edge_t *e12 = dijk_node_find_edge (n1, n2);
    if (e12) {
        g_queue_remove (n1->edges, e12);
        dijk_graph_remove_edge (dg, e12);
        ok = 1;
    }
    dijk_edge_t *e21 = dijk_node_find_edge (n2, n1);
    if (e21) {
        g_queue_remove (n2->edges, e21);
        dijk_graph_remove_edge (dg, e21);
        ok = 1;
    }

    if (ok) 
        dbg (DBG_CLASS, "removed edge %d-%d", id1, id2);
}

void dijk_graph_merge_nodes_by_id (dijk_graph_t *dg, int id1, int id2)
{
    dijk_node_t *nd1 = dijk_graph_find_node_by_id (dg, id1);
    dijk_node_t *nd2 = dijk_graph_find_node_by_id (dg, id2);
    if (nd1 && nd2) {
        dijk_graph_merge_nodes (dg, nd1, nd2);
        dbg (DBG_CLASS, "merged nodes %d and %d", nd1->uid, nd2->uid);
    }

}

/* remove <n2> from the graph and make its edges point to <n1>
*/
void dijk_graph_merge_nodes (dijk_graph_t *dg, dijk_node_t *n1, dijk_node_t *n2)
{
    // break link between n1 and n2
    dijk_edge_t *e12 = dijk_node_find_edge (n1, n2);
    if (e12) {
        g_queue_remove (n1->edges, e12);
        dijk_graph_remove_edge (dg, e12);
    }
    assert (!dijk_node_find_edge (n1, n2));
    dijk_edge_t *e21 = dijk_node_find_edge (n2, n1);
    if (e21) {
        g_queue_remove (n2->edges, e21);
        dijk_graph_remove_edge (dg, e21);
    }
    assert (!dijk_node_find_edge (n2, n1));

    // all neighbors of n2 point to n1
    GQueue *n1n = dijk_node_neighbors (n1);
    GQueue *n2n = dijk_node_neighbors (n2);

    // check no duplicates
    for (GList *iter1=g_queue_peek_head_link (n2n);iter1;iter1=iter1->next) {
        for (GList *iter2=iter1->next;iter2;iter2=iter2->next) {
            assert ((dijk_node_t*)iter1->data != (dijk_node_t*)iter2->data);
        }
    }

    for (GList *iter=g_queue_peek_head_link (n2n);iter;iter=iter->next) {
        dijk_node_t *n = (dijk_node_t*)iter->data;
        assert (n);
        assert (n != n1);
        dijk_edge_t *e = dijk_node_find_edge (n, n2);
        assert (e);
        assert (e->start == n);
        assert (e->end == n2);
        if (dijk_node_has_neighbor (n, n1)) {
            g_queue_remove (n->edges, e);
            dijk_graph_remove_edge (dg, e);
        } else {
            e->end = n1;
            assert (e->start != e->end);
        }
    }

    // all edges of n2 point-start from n1
    for (GList *eiter=g_queue_peek_head_link (n2->edges);eiter;eiter=eiter->next) {
        dijk_edge_t *e = (dijk_edge_t*)eiter->data;
        assert (e->start = n2);
        dijk_node_t *n = e->end;
        if (dijk_node_has_neighbor (n1, n)) {
            dijk_graph_remove_edge (dg, e);
        } else {
            g_queue_push_tail (n1->edges, e);
        }
        e->start = n1;
        assert (e->start != e->end);
    }

    // transfer metadata
    if (n2->checkpoint)
        n1->checkpoint = TRUE;
    if (!n1->label && n2->label)
        n1->label = strdup (n2->label);
    n1->utime = n2->utime;

    // remove n2 from graph
    assert (dijk_graph_remove_node (dg, n2));

    // sanity check
    assert (!dijk_graph_has_node (dg, n2));

    // destroy n2
    dijk_node_destroy (n2);

}

/* merge nodes given a list of components
*/
void dijk_apply_components (dijk_graph_t *dg, GQueue *components)
{
    dbg (DBG_CLASS, "applying %d components.", g_queue_get_length (components));

    for (GList *iter=g_queue_peek_head_link (components);iter;iter=iter->next) {
        component_t *c = (component_t*)iter->data;
        for (GList *piter=g_queue_peek_head_link (c->pt);piter;piter=piter->next) {
            pair_int_t *p = (pair_int_t*)piter->data;
            if (p->val == p->key) continue;

            printf ("merging node %d and node %d\n", p->key, p->val);
            assert (p->val <= p->key);

            dijk_node_t *n2 = dijk_graph_find_node_by_id (dg, p->key);
            dijk_node_t *n1 = dijk_graph_find_node_by_id (dg, p->val);

            // may happen if a node is called for merge twice
            if (!n1 || !n2)
                continue;

            printf ("merging node %d and node %d [**]\n", n1->uid, n2->uid);

            dijk_graph_merge_nodes (dg, n1, n2);

        }
    }
}

void dijk_graph_render_layout (dijk_graph_t *dg, const char *mode, int max_nodeid, Agraph_t **igraph, GVC_t **icontext)
{
    // create context (automatically calls aginit())
    *icontext = gvContext();

    // open a graph
    *igraph = agopen((char*)"map", AGRAPHSTRICT); // strict, directed graph

    GVC_t *context = *icontext;
    Agraph_t *graph = *igraph;

    assert (graph && context);

    // add all nodes to the graph
    //
    for (GList *iter=g_queue_peek_head_link (dg->nodes);iter;iter=g_list_next (iter)) {
        dijk_node_t *n = (dijk_node_t*)iter->data;
        if (max_nodeid > 0 && n->uid > max_nodeid) continue;
        char name[5];
        sprintf (name, "%d", n->uid);
        agnode (graph, name);
        Agnode_t *n1 = agfindnode (graph, name);
        if (n->label) {
            char label[128];
            sprintf (label, "%d -- %s", n->uid, n->label);
            agset (n1, (char*)"label", label);
        }
    }

    // create edges
    int count=0;
    for (GList *iter=g_queue_peek_head_link (dg->nodes);iter;iter=g_list_next (iter)) {
        dijk_node_t *n = (dijk_node_t*)iter->data;
        if (max_nodeid > 0 && n->uid > max_nodeid) continue;
        // find corresponding node
        char name[5];
        sprintf (name, "%d", n->uid);
        Agnode_t *n1 = agfindnode (graph, name);
        if (!n1) {
            dbg (DBG_ERROR, "failed to retrieve node from name %s", name);
            return;
        }
        for (GList *eiter=g_queue_peek_head_link (n->edges);eiter;eiter=eiter->next) {
            dijk_edge_t *e = (dijk_edge_t*)eiter->data;
            if (e->start != n) {
                dbg (DBG_ERROR, "Error: node %d has wrong edge: %d %d", n->uid, e->start->uid, e->end->uid);
            }
            assert (e->start == n);
            if (!e->end) continue;
            assert (e->start != e->end);
            assert (e->start->uid != e->end->uid);
            sprintf (name, "%d", e->end->uid);
            Agnode_t *n2 = agfindnode (graph, name);
            if (!n2) {
                dbg (DBG_ERROR, "failed to retrieve node from name %s", name);
                continue;
            }
            assert (n1->id != n2->id);
            agedge (graph, n1, n2);
            count++;
        }
    }

    // layout the graph
    // choices are: manual, fdp, dot, neato, twopi, circo

    if (!strcmp (mode, "fdp")) {
        gvLayout (context, graph, (char*)"fdp");
    } 
    if (!strcmp (mode, "dot")) {
        gvLayout (context, graph, (char*)"dot");
    }
    if (!strcmp (mode, "neato")) {
        gvLayout (context, graph, (char*)"neato");
    }
    if (!strcmp (mode, "twopi")) {
        gvLayout (context, graph, (char*)"twopi");
    }
    if (!strcmp (mode, "circo")) {
        gvLayout (context, graph, (char*)"circo");
    }

}

void dijk_graph_layout_to_file (dijk_graph_t *dg, const char *mode, const char *format, const char *filename, int max_nodeid)
{
    Agraph_t *graph;
    GVC_t* context;

    dijk_graph_render_layout (dg, mode, max_nodeid, &graph, &context);

    // render graph to file
    dbg (DBG_CLASS, "saving graph to file %s", filename);
    gvRenderFilename(context, graph, (char*)format, (char*)filename);
    dbg (DBG_CLASS, "done.");

    // free graph
    gvFreeLayout(context, graph);
    agclose (graph);
    gvFreeContext(context);

}

void dijk_graph_remove_triplets (dijk_graph_t *dg)
{
    for (GList *iter=g_queue_peek_head_link (dg->nodes);iter;iter=iter->next) {
        dijk_node_t *n = (dijk_node_t*)iter->data;

        if (dijk_node_degree (n) != 2)
            continue;

        dijk_edge_t *e0 = (dijk_edge_t*)g_queue_peek_nth (n->edges, 0);
        dijk_edge_t *e1 = (dijk_edge_t*)g_queue_peek_nth (n->edges, 1);
        if (!e0->end || !e1->end)
            continue;

        dijk_node_t *n0 = e0->end;
        dijk_node_t *n1 = e1->end;

        if (dijk_node_degree (n0) <=2 ) continue;
        if (dijk_node_degree (n1) <=2 ) continue;

        dijk_edge_t *e01 = dijk_node_find_edge (n0, n1);
        if (e01) {
            dijk_graph_remove_edge (dg, e01);
            g_queue_remove (n0->edges, e01);
            dijk_edge_destroy (e01);
        }
        dijk_edge_t *e10 = dijk_node_find_edge (n1, n0);
        if (e10) {
            dijk_graph_remove_edge (dg, e10);
            g_queue_remove (n1->edges, e10);
            dijk_edge_destroy (e10);
        }

        printf ("removed edge between %d and %d\n", n0->uid, n1->uid);
    }
}

void dijk_graph_test_mission (dijk_graph_t *dg, GQueue *mission)
{
    for (GList *iter=g_queue_peek_head_link (mission);iter;iter=iter->next) {
        if (iter->next) {
            int *id1 = (int*)iter->data;
            int *id2 = (int*)iter->next->data;

            dijk_node_t *nd1 = dijk_graph_find_node_by_id (dg, *id1);
            dijk_node_t *nd2 = dijk_graph_find_node_by_id (dg, *id2);

            GQueue *path = dijk_find_shortest_path (dg, nd1, nd2);

            printf ("path from %d to %d:\n", *id1, *id2);

            for (GList *piter=g_queue_peek_head_link (path);piter;piter=piter->next) {
                dijk_edge_t *e = (dijk_edge_t*)piter->data;
                if (e->start)
                    printf ("%d\n", e->start->uid);
            }

            g_queue_free (path);
        }
    }
}

void dijk_graph_remove_node_by_id (dijk_graph_t *dg, int id)
{
    dijk_node_t *n = dijk_graph_find_node_by_id (dg, id);
    if (!n) return;

    if (dijk_node_degree (n) != 2) {
        printf ("can't remove this node.\n");
        return;
    }

    printf ("removing node %d\n", id);

    GQueue *ns = dijk_node_neighbors (n);

    dijk_node_t *n0 = (dijk_node_t*)g_queue_peek_nth (ns, 0);
    dijk_node_t *n1 = (dijk_node_t*)g_queue_peek_nth (ns, 1);

    for (GList *eiter=g_queue_peek_head_link (n0->edges);eiter;eiter=eiter->next) {
        dijk_edge_t *e = (dijk_edge_t*)eiter->data;
        if (e->end == n)
            e->end = n1;
    }

    for (GList *eiter=g_queue_peek_head_link (n1->edges);eiter;eiter=eiter->next) {
        dijk_edge_t *e = (dijk_edge_t*)eiter->data;
        if (e->end == n)
            e->end = n0;
    }

    dijk_graph_remove_node (dg, n);
    dijk_node_destroy (n);
}


void dijk_print_component (dijk_graph_t *dg, component_t *c, const char *corrmat_filename, const char *filename)
{
    FILE *fp = fopen (filename, "a");
    if (!fp)
        return;

    int size = 0;
    double * corrmat = corrmat_read (corrmat_filename, &size);

    int64_t last_utime=0;

    if (dijk_graph_n_nodes (dg) == 0)
        return;

    for (GList *iter=g_queue_peek_head_link (c->pt);iter;iter=iter->next) {
        pair_int_t *p = (pair_int_t*)iter->data;

        dijk_node_t *n1 = dijk_graph_find_node_by_id (dg, p->val);
        dijk_node_t *n2 = dijk_graph_find_node_by_id (dg, p->key);
        if (!n2) {
            fprintf (stderr, "failed to find node2 %d.\n", p->key);
        }
        if (!n1) {
            fprintf (stderr, "failed to find node1 %d.\n", p->val);
        }

        fprintf (fp, "%ld %ld\n", n1->utime, n2->utime);
    }

    fprintf (fp, "\n");

    free (corrmat);

    fclose (fp);
}

void dijk_graph_print_node_utimes (dijk_graph_t *dg, const char *filename)
{
    FILE *fp = fopen (filename, "w");

    if (!fp) return;

    for (GList *iter=g_queue_peek_head_link (dg->nodes);iter;iter=iter->next) {
        dijk_node_t *n = (dijk_node_t*)iter->data;
        fprintf (fp, "%ld %d\n", n->utime, n->uid);
    }

    fclose (fp);
}

/* convert a graph into a ui map (basic version)
*/
navlcm_ui_map_t *dijk_graph_to_ui_map_basic (dijk_graph_t *dg, const char *mode)
{
    if (!dg || !dg->nodes || !dg->edges)
        return NULL;

    if (g_queue_is_empty (dg->nodes) || g_queue_is_empty (dg->edges))
        return NULL;

    // convert to ui map
    navlcm_ui_map_t *mp = (navlcm_ui_map_t*)malloc(sizeof(navlcm_ui_map_t));
    mp->n_nodes = g_queue_get_length (dg->nodes);
    mp->nodes = (navlcm_ui_map_node_t*)malloc(mp->n_nodes*sizeof(navlcm_ui_map_node_t));
    mp->n_edges = g_queue_get_length (dg->edges);
    mp->edges = (navlcm_ui_map_edge_t*)malloc(mp->n_edges*sizeof(navlcm_ui_map_edge_t));

    for (int i=0;i<mp->n_nodes;i++) {
        navlcm_ui_map_node_t *mn = &mp->nodes[i];
        dijk_node_t *dn = (dijk_node_t*)g_queue_peek_nth (dg->nodes, i);
        mn->x = i;
        mn->y = 1.0*i/10;
        mn->label = dn->label ? strdup (dn->label) : strdup (" ");
        mn->id = dn->uid;
        mn->utime = dn->utime;
    }

    for (int i=0;i<mp->n_edges;i++) {
        navlcm_ui_map_edge_t *me = &mp->edges[i];
        dijk_edge_t *de = (dijk_edge_t*)g_queue_peek_nth (dg->edges, i); 
        me->id1 = de->start ? de->start->uid : -1;
        me->id2 = de->end ? de->end->uid : -1;
        me->motion_type = de->motion_type;
        me->reverse = de->reverse;
    }

    // normalize coordinates
    double minx = 1E6, miny = 1E6, maxx = -1E6, maxy = -1E6;
    for (int i=0;i<mp->n_nodes;i++) {
        minx = fmin (minx, mp->nodes[i].x);
        miny = fmin (miny, mp->nodes[i].y);
        maxx = fmax (maxx, mp->nodes[i].x);
        maxy = fmax (maxy, mp->nodes[i].y);
    }
    for (int i=0;i<mp->n_nodes;i++) {
        mp->nodes[i].x = (mp->nodes[i].x - minx)/(maxx-minx);
        mp->nodes[i].y = (mp->nodes[i].y - miny)/(maxy-miny);
    }

    return mp;
}

/* convert a graph into a ui map
*/
navlcm_ui_map_t *dijk_graph_to_ui_map (dijk_graph_t *dg, const char *mode)
{
    if (!dg || !dg->nodes || !dg->edges)
        return NULL;

    if (g_queue_is_empty (dg->nodes) || g_queue_is_empty (dg->edges))
        return NULL;

    // render the graph using graphviz
    Agraph_t *graph;
    GVC_t* context;

    dijk_graph_render_layout (dg, mode, -1, &graph, &context);

    // convert to ui map
    navlcm_ui_map_t *mp = (navlcm_ui_map_t*)malloc(sizeof(navlcm_ui_map_t));
    mp->n_nodes = g_queue_get_length (dg->nodes);
    mp->nodes = (navlcm_ui_map_node_t*)malloc(mp->n_nodes*sizeof(navlcm_ui_map_node_t));
    mp->n_edges = g_queue_get_length (dg->edges);
    mp->edges = (navlcm_ui_map_edge_t*)malloc(mp->n_edges*sizeof(navlcm_ui_map_edge_t));

    assert (graph);

    int icount=0;

    printf ("********************************************\n");

    for (Agnode_t *v=agfstnode (graph);v;v = agnxtnode(graph,v)) {
        navlcm_ui_map_node_t  *mn = &mp->nodes[icount];
        icount++;

        sscanf (v->name, "%d", &mn->id);
        mn->label = strdup (ND_label(v)->text);
        mn->x = ND_coord_i(v).x;
        mn->y = ND_coord_i(v).y;
        dijk_node_t *nd = dijk_graph_find_node_by_id (dg, mn->id);
        assert (nd);
        printf ("node %d: %.3f %.3f\n", mn->id, mn->x, mn->y);
        mn->utime = nd->utime;
    }

    icount=0;
    for (GList *iter=g_queue_peek_head_link (dg->edges);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;
        navlcm_ui_map_edge_t  *mn = &mp->edges[icount];
        icount++;
        mn->id1 = e->start ? e->start->uid : -1;
        mn->id2 = e->end ? e->end->uid : -1;
        mn->motion_type = e->motion_type;
        mn->reverse = e->reverse;
    }

    // normalize coordinates
    double minx = 1E6, miny = 1E6, maxx = -1E6, maxy = -1E6;
    for (int i=0;i<mp->n_nodes;i++) {
        minx = fmin (minx, mp->nodes[i].x);
        miny = fmin (miny, mp->nodes[i].y);
        maxx = fmax (maxx, mp->nodes[i].x);
        maxy = fmax (maxy, mp->nodes[i].y);
    }
    for (int i=0;i<mp->n_nodes;i++) {
        mp->nodes[i].x = (mp->nodes[i].x - minx)/(maxx-minx);
        mp->nodes[i].y = (mp->nodes[i].y - miny)/(maxy-miny);
    }

    // free graph
    gvFreeLayout(context, graph);
    agclose (graph);
    gvFreeContext(context);

    return mp;
}

/* given a graph with nodes and edges, a pose (navigation path)
 * and the estimate of the current edge (nodeid1, nodeid2),
 * compute the distance to ground-truth
 * <fix_poses> is an optional set of ground-truth poses needed to fix <pose>
 * presumably they come from ground-truth (e.g. carmen)
 */
void dijk_graph_ground_truth_localization (dijk_graph_t *dg, botlcm_pose_t *pose, GQueue *fix_poses, dijk_node_t *node, int num_features, const char *filename)
{
    if (!node) return;

    // fix pose with ground-truth data if possible
    botlcm_pose_t *pose_copy = botlcm_pose_t_copy (pose);
    if (util_fix_pose (&pose_copy, fix_poses) < 0)
        return;

    dijk_edge_t *ref_edge = dijk_get_nth_edge (node, 0);

    if (!ref_edge) return;

    // physical distance to estimated edge
    double dist = bot_vector_dist_3d (pose_copy->pos, ref_edge->pose->pos);

    // topological distance to actual edge
    double best_dist = .0;
    dijk_edge_t *best_edge = NULL;
    for (GList *iter=g_queue_peek_head_link (dg->edges);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;
        double d = bot_vector_dist_3d (e->pose->pos, pose_copy->pos);
        if (!best_edge || d < best_dist) {
            best_dist = d;
            best_edge = e;
        }
    }

    if (best_edge) {
        GQueue *path = dijk_find_shortest_path (dg, node, best_edge->start);
        int top_dist = g_queue_get_length (path);
        g_queue_free (path);

        FILE *fp = fopen (filename, "a");
        // estimated node, estimated edge, physical distance, actual edge, top distance
        int ref_edge_start_id = ref_edge->start ? ref_edge->start->uid : -1;
        int ref_edge_end_id = ref_edge->end ? ref_edge->end->uid : -1;
        int best_edge_start_id = best_edge->start ? best_edge->start->uid : -1;
        int best_edge_end_id = best_edge->end ? best_edge->end->uid : -1;

        fprintf (fp, "%ld %d %d-%d %.4f %d-%d %d %d\n", pose->utime, node->uid, ref_edge_start_id, ref_edge_end_id, dist, best_edge_start_id, best_edge_end_id, top_dist, num_features);
        fclose (fp);
    }

    botlcm_pose_t_destroy (pose_copy);
}

/* Given a set of poses, fix the poses in the graph dg
 * Presumably, the <poses> data comes from ground-truth (e.g. Carmen)
 */
void dijk_graph_fix_poses (dijk_graph_t *dg, GQueue *poses)
{
    if (g_queue_is_empty (poses))
        return;

    for (GList *iter=g_queue_peek_head_link (dg->edges);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;

        if (util_fix_pose (&e->pose, poses) < 0) {
            fprintf (stderr, "failed to fix pose for edge\n");
            dijk_edge_print (e);
        }
    }
}

/* estimate path length (in seconds and meters)
*/
void dijk_integrate_time_distance (GQueue *path, double *time_secs, double *distance_m)
{
    *time_secs = .0;
    *distance_m = .0;

    for (GList *iter = g_queue_peek_head_link (path);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;
        *time_secs += dijk_edge_time_length (e);
    }

    // average human walking speed 1.3m/s
    double speed = 1.0;

    *distance_m = speed * *time_secs;

    // round to closest 5 seconds and meters
    //*time_secs = 5.0 * (int)(*time_secs/5);
    //*distance_m = 5.0 * (int)(*distance_m/5);

}

double dijk_edge_time_length (dijk_edge_t *e)
{
    if (!e->start || !e->end)
        return .0;

    int64_t dt = 0;
    if (e->start->utime > e->end->utime)
        dt = e->start->utime - e->end->utime;
    else
        dt = e->end->utime - e->start->utime;

    if (dt > 20000000)
        dt = 2000000;

    return 1.0*((int)dt)/1000000;
}

/* find the next turn (left/right) or up/down motion in the path
 * the path is a series of edges
 * searches forward to maxdepth
 * output: motion type
 */
int dijk_find_future_direction_in_path (GQueue *path, int mindepth, int maxdepth, int *motion_type, int size)
{
    *motion_type = -1;

    for (int i=0;i<size;i++)
        motion_type[i] = -1;

    if (!path) return -1;

    int depth = 0;
    int count=0;

    for (GList *iter = g_queue_peek_head_link (path);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;
        assert (e);
        if (!e->start) {
            depth++;
            continue;
        }
        if (depth >= mindepth) {
            motion_type[count] = e->motion_type;
            count++;
        }
        depth++;
        if (depth >= maxdepth || count >= size) 
            break;
    }

    return 0;
}

/* make sure that each edge has the opposite motion type as its sibling
*/
void dijk_graph_enforce_type_symmetry (dijk_graph_t *dg)
{
    for (GList *iter=g_queue_peek_head_link (dg->edges);iter;iter=iter->next) {
        dijk_edge_t *e = (dijk_edge_t*)iter->data;
        dijk_edge_t *s = dijk_node_find_edge (e->end, e->start);
        if (s && e->motion_type != NAVLCM_CLASS_PARAM_T_MOTION_TYPE_FORWARD) {
            s->motion_type = MOTION_TYPE_REVERSE (e->motion_type);
        }
    }
}

