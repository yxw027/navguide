/*
 * Incremental vocabulary using bags of words.
 *
 * This is an implementation of the method described in 
 * D. Filliat, Interactive Learning of Visual Topological Navigation, IROS 08.
 */

#include "bags.h"

#define BAG_DBG 0

static int g_nid = 0;

/* return the depth of a node in the tree
 */
int node_depth (GNode *node)
{
    int depth = 0;
    GNode *par = node->parent;
    while (par) {
        depth++;
        par = par->parent;
    }
    return depth;
}

/* compute the centroid of a bag
 */
void bag_t_centroid (GQueue *bags, float *cc, int size)
{
    int count=0;

    for (int i=0;i<size;i++)
        cc[i] = 0.0;

    for (GList *iter = g_queue_peek_head_link (bags);iter;iter=g_list_next(iter)) {
        bag_t *b = (bag_t*)iter->data;
        for (int i=0;i<size;i++) {
            cc[i] += b->cc[i];
        }
        count++;
    }

    if (count > 0) {
        for (int i=0;i<size;i++)
            cc[i] /= count;
   }
}

/* compute the difference between two sets of words
 * (for validation)
 */
GQueue *bag_t_diff (GQueue *bags1, GQueue *bags2)
{
    GQueue *bags = g_queue_new ();

    for (GList *iter = g_queue_peek_head_link (bags1);iter;iter=g_list_next(iter)) {
        bag_t *b1 = (bag_t*)iter->data;
        gboolean found=FALSE;
        for (GList *iter2 = g_queue_peek_head_link (bags2);iter2;iter2=g_list_next(iter2)) {
            bag_t *b2 = (bag_t*)iter2->data;
            if (vect_sqdist_float (b1->cc, b2->cc, b1->cc_size) < 1E-6 && b1->n == b2->n) {
                found = TRUE;
                break;
            }
        }
        if (!found) {
            g_queue_push_head (bags, b1);
        }
    }
    return bags;
}

/* init a bag (i.e. "word")
*/
int bag_t_init (bag_t *b, navlcm_feature_t *ft, int id)
{
    b->cc = NULL;
    b->cc_size = 0;
    b->ind = g_queue_new ();
    b->n = 0;
    bag_t_insert_feature (b, ft, id);
    return 0;
}

/* destroy a bag
 */
void bag_t_destroy (bag_t *b)
{
    if (b->cc)
        free (b->cc);
    g_queue_free (b->ind);
}

/* insert a feature along with an index in a bag
*/
int bag_t_insert_feature (bag_t *b, navlcm_feature_t *ft, int id)
{
    // insert index in the bag indices
    g_queue_push_head (b->ind, GINT_TO_POINTER (id));
    b->n++;

    if (!b->cc) {
        b->cc = (float*)malloc(ft->size*sizeof(float));
        b->cc_size = ft->size;
        for (int i=0;i<ft->size;i++)
            b->cc[i] = 1.0*ft->data[i];
    }
    // do not update bag centroid here.

    return 0;
}

/* L2 distance between a feature and a bag centroid
 * assuming normalized descriptors
 */
float bag_t_distance (bag_t *b, navlcm_feature_t *f)
{
    assert (f->size == b->cc_size);
    double dot = .0;
    for (int i=0;i<f->size;i++)
        dot += b->cc[i] * f->data[i];
    return 2.0 - 2 * dot;
//    return 1.0 * sqrt (1.0*vect_sqdist_float (b->cc, f->data, f->size));
}

/* search an index in a bag
*/
int bag_t_search_index (bag_t *b, int index)
{
    GList *el = g_queue_find (b->ind, GINT_TO_POINTER(index));
    if (el) return 1;
    return 0;
}

/* brute-force search for bags containing a feature
 */
void bag_t_search (GQueue *bags, navlcm_feature_t *ft, float bag_radius, GQueue *bbags)
{
    for (GList *iter=g_queue_peek_head_link (bags);iter;iter=g_list_next(iter)) {
        bag_t *b = (bag_t*)iter->data;
        float d = bag_t_distance (b, ft);
        if (d < bag_radius) {
            g_queue_push_head (bbags, b);
        }
    }
}

/* find closest child to a given feature
*/
GNode *tree_find_closest_child (GNode *node, navlcm_feature_t *f)
{
    GNode *cchild=NULL, *child=NULL;
    float d;

    assert (!G_NODE_IS_LEAF (node));

    child = g_node_first_child (node);
    cchild = child;
    d = node_distance_d (child, f->data);

    while (child) {
        float r = node_distance_d (child, f->data);
        if (r < d) {
            d = r;
            cchild = child;
        }
        child = child->next;
    }

    return cchild;
}

struct node_dist_t { GNode *node; float d;};

gint node_dist_comp (gconstpointer a, gconstpointer b, gpointer data)
{
    node_dist_t *na = (node_dist_t*)a;
    node_dist_t *nb = (node_dist_t*)b;
    if (fabs (na->d-nb->d) < 1E-6) return 0;
    if (na->d < nb->d) return -1;
    return 1;
}

/* compute distance to border given two nodes a, b and a feature f
*/
float node_frontier_distance (GNode *a, GNode *b, navlcm_feature_t *f)
{
    assert (a && b);
    tree_node_t *na = (tree_node_t*)a->data;
    tree_node_t *nb = (tree_node_t*)b->data;
    float abdotaf=0.0;
    float *ab, *af;

    ab = (float*)malloc(f->size*sizeof(float));
    af = (float*)malloc(f->size*sizeof(float));
    for (int i=0;i<f->size;i++) {
        abdotaf += 1.0 * (nb->cc[i] - na->cc[i]) * (1.0*f->data[i] - na->cc[i]);
        ab[i] = 1.0 * (nb->cc[i] - na->cc[i]);
        af[i] = 1.0 * (1.0*f->data[i] - na->cc[i]);
    }

    float normab = vect_length_float (ab, f->size);
    float normaf = vect_length_float (af, f->size);
    float costheta = abdotaf / (normab * normaf);

    free (ab);
    free (af);
    return fabs (costheta * normaf - normab / 2.0);
    
}

/* find the N closest nodes to a given node using the frontier distance node
 */
GQueue *tree_find_closest_children_frontier (GNode *node, navlcm_feature_t *f, unsigned int N, float bag_radius)
{
    GNode *child, *cnode;
    GQueue *data, *nodes;
    GList *iter;

    // find the closest child
    cnode = tree_find_closest_child (node, f);
    assert (cnode);

    // for each child, compute the frontier distance
    data = g_queue_new ();

    // debugging: return all children
#if BAG_DBG
    child = g_node_first_child (node);
    while (child) {
        g_queue_push_head (data, child);
        child = child->next;
    }
    return data;
#endif

    child = g_node_first_child (node);
    while (child) {
        node_dist_t *nd = (node_dist_t*)malloc(sizeof(node_dist_t));
        nd->node = child;
        if (child != cnode) {
            nd->d = node_frontier_distance (cnode, child, f);
        } else {
            nd->d = 0.0;
        }
        g_queue_push_head (data, nd);
        child = child->next;
    }

    // sort the data by increasing frontier distance
    g_queue_sort (data, node_dist_comp, NULL);

#if BAG_DBG
    printf ("***************\n");
    for (iter=g_queue_peek_head_link(data);iter;iter=g_list_next(iter)) {
        node_dist_t *nd = (node_dist_t*)iter->data;
        printf ("%.4f\n", nd->d);
    }
#endif

//    printf ("N = %d, size = %d\n", N, g_queue_get_length (data));

    nodes = g_queue_new ();
    iter = g_queue_peek_head_link (data);
    while (iter) {
        node_dist_t *nd = (node_dist_t*)iter->data;
        if (N < 0 || g_queue_get_length (nodes) < N) {
            if (nd->d < bag_radius) 
                g_queue_push_head (nodes, nd->node);
        }
        free (nd);
        iter = iter->next;
    }
    g_queue_free (data);    
    return nodes;
}

/* find closest node from a given feature
*/
void tree_t_find_closest_node (GNode *node, navlcm_feature_t *ft, GNode **cnode, float *mindist)
{
    GNode *child;

    if (G_NODE_IS_LEAF (node)) {
        float d = node_distance_d (node, ft->data);

        if (!*cnode || d < *mindist) {
            *mindist = d;
            *cnode = node;
        }
    } else {
        child = g_node_first_child (node);
        while (child) {
            tree_t_find_closest_node (child, ft, cnode, mindist);
            child = child->next;
        }
    }
}

/* search for a feature in the tree
 */
void tree_t_search_feature_full (GNode *node, navlcm_feature_t *ft, unsigned int N, float bag_radius, GQueue *bbags)
{
    // if not leaf, look for the n closest nodes within the children
    if (!G_NODE_IS_LEAF (node)) {
        GQueue *neighbors = tree_find_closest_children_frontier (node, ft, N, bag_radius);
        for (GList *iter=g_queue_peek_head_link (neighbors);iter;iter=g_list_next(iter)) {
            GNode *nd = (GNode*)iter->data;
            tree_t_search_feature_full (nd, ft, N, bag_radius, bbags);
        }
        g_queue_free (neighbors);

    }

    // if leaf, brute-force search for the closest bags
    if (G_NODE_IS_LEAF (node)) {
        bag_t_search (((tree_node_t*)node->data)->bags, ft, bag_radius, bbags);

    }
}

/* entry point for search in vocabulary
 * use tree-search if <node> is not NULL. use brute-force search otherwise.
 */
void bag_node_t_search_feature (GQueue *bags, GNode *node, navlcm_feature_t *ft, unsigned int N, float bag_radius, GQueue *bbags)
{
    if (!node) {
        bag_t_search (bags, ft, bag_radius, bbags);
    } else {
        tree_t_search_feature_full (node, ft, N, bag_radius, bbags);
    }
}

/* insert a feature in a set of words (bags)
 * using exhaustive search
 * <bbags> contains the list of words that have been hit
*/
int bag_t_insert_feature_full (GQueue *bags, navlcm_feature_t *ft, float bag_radius, int id, gboolean *inserted, GQueue *bbags)
{
    // exhaustive search
    GList *el = g_queue_peek_head_link (bags);

    while (el) {
        bag_t *b = (bag_t*)el->data;
        float d = bag_t_distance (b, ft);
        assert (d < 1.0001);
        if (d < bag_radius) {
            // insert the feature in the bag
            bag_t_insert_feature (b, ft, id);
            if (bbags) g_queue_push_head (bbags, b);
            *inserted = TRUE;
        }
        el = el->next;
    }
    return 0;
}

/* insert a feature in a set of words (bags)
 * using exhaustive search
 * <bbags> contains the list of words that have been hit
 * this is the fast version based on MKL
*/
int bag_t_insert_feature_full_fast (GQueue *bags, navlcm_feature_t *ft, float bag_radius, int id, gboolean *inserted, GQueue *bbags)
{
    int nbags = g_queue_get_length (bags);
    GList *iter;

    float *m1 = (float*)calloc(nbags * ft->size, sizeof(float));

    // convert bags descriptors to matrix
    iter=g_queue_peek_head_link (bags);
    for (int i=0;i<nbags;i++) {
        bag_t *b = (bag_t*)iter->data;
        memcpy (m1 + i * ft->size, b->cc, ft->size * sizeof(float));
        iter = iter->next;
    }

    // compute dot products
    float *dotprod = (float*)calloc (nbags * 1, sizeof(float));
    math_matrix_mult_mkl_float (nbags, 1, ft->size, m1, ft->data, dotprod);

    // for each bag within radius distance, update index list
    iter=g_queue_peek_head_link (bags);
    for (int i=0;i<nbags;i++) {
        if (2.0 - 2.0 * dotprod[i] < bag_radius) {
            bag_t *b = (bag_t*)iter->data;
            bag_t_insert_feature (b, ft, id); 
            if (bbags) g_queue_push_head (bbags, b);
            *inserted = TRUE;
        }
        iter = iter->next;
    }

    free (dotprod);
    free (m1);
}

/* find the closest bags to a feature in the vocabulary. on first call, <node> should be the root of the tree, 
 * <inserted> should be false, <*cnode> should be null.
 */
void tree_t_insert_feature_full (GNode *node, navlcm_feature_t *ft, unsigned int N, float bag_radius, int id, gboolean *inserted, GQueue *bbags, GTimer *timer)
{
    // if not leaf, look for the n closest nodes within the children
    if (!G_NODE_IS_LEAF (node)) {
        GQueue *neighbors = tree_find_closest_children_frontier (node, ft, N, bag_radius);
        for (GList *iter=g_queue_peek_head_link (neighbors);iter;iter=g_list_next(iter)) {
            GNode *nd = (GNode*)iter->data;
            tree_t_insert_feature_full (nd, ft, N, bag_radius, id, inserted, bbags, timer);
        }
        g_queue_free (neighbors);

    }

    // if leaf, linear search for the closest bags and update the closest node
    if (G_NODE_IS_LEAF (node)) {
        tree_node_t *node_data = (tree_node_t*)node->data;
        bag_t_insert_feature_full_fast (node_data->bags, ft, bag_radius, id, inserted, bbags);
    }
}


/* inserts a feature in the vocabulary. use optimized tree if <node> is not NULL, use naive search using <bbags> otherwise.
 * <id> is an index inserted in the bag (e.g. the gate ID in the map)
 * <node> is the root node of the tree
 */
gboolean bag_node_t_insert_feature (GQueue *bags, GNode *node, navlcm_feature_t *ft, unsigned int N, float bag_radius, int id, GQueue *bbags)
{
    gboolean inserted = FALSE;
    GNode *cnode = NULL;
    float mindist = .0;
    GTimer *timer = g_timer_new ();
  
    // insert the feature
    if (!node) {
        // naive (exhaustive) search
        bag_t_insert_feature_full_fast (bags, ft, bag_radius, id, &inserted, bbags);
    } else {
        // tree search
        tree_t_insert_feature_full (g_node_get_root (node), ft, N, bag_radius, id, &inserted, bbags, timer);
    }

    // if not inserted, create a new bag and add it to the tree (or to the naive set of bags)
    if (!inserted) {
        bag_t *bag = (bag_t*)malloc(sizeof(bag_t));
        bag_t_init (bag, ft, id);
        if (bbags) g_queue_push_head (bbags, bag);
        if (!node) {
            g_queue_push_head (bags, bag);
        } else {
            assert (!cnode);
            tree_t_find_closest_node (node, ft, &cnode, &mindist);
            assert (cnode);
            node_add_bag (cnode, bag);
        }
    } 

    g_timer_destroy (timer);

    return inserted;
}

/* insert a list of features in a dictionary (set of bags)
*/
int bag_tree_t_insert_feature_list (GQueue *bags, GNode *node, navlcm_feature_list_t *fs, unsigned int N, float bag_radius, int id)
{
    int merged=0;
    float merge_ratio=0.0;
    GTimer *timer;

    if (!fs) return -1;

    timer = g_timer_new ();

    // insert each feature
    for (int i=0;i<fs->num;i++) {
        navlcm_feature_t *ft = fs->el + i;
        if (bag_node_t_insert_feature (bags, node, ft, N, bag_radius, id, NULL))
            merged++;
    }

    dbg (DBG_CLASS, "midpoint timer: %.3f secs.", g_timer_elapsed (timer, NULL));

    // update the tree
    if (node)
        tree_t_node_update_full (node);

    merge_ratio = 100.0 * merged / fs->num;
    float secs = g_timer_elapsed (timer, NULL);
    dbg (DBG_CLASS, "[bags] N=%d, inserted %d features in %.3f secs. Merge ratio: %.2f %%. ", N, fs->num, secs, merge_ratio);
    if (bags)
        dbg (DBG_CLASS, "Set has %d bags now.", g_queue_get_length (bags));

    g_timer_destroy (timer);

    return 0;
}

//////////////////////////////////////////////////////////////////
//
// Tree Data Structure
//

/* return the dimension of the feature descriptor
 */
int bag_feature_size (GQueue *bags)
{
    GList *iter = g_queue_peek_head_link (bags);
    if (!iter) return -1;

    bag_t *bag = (bag_t*)iter->data;
    return bag->cc_size;
}

/* return the total number of words within the children of this node (recursive)
 */
int node_size (GNode *node) 
{
    int count=0;
    if (G_NODE_IS_LEAF (node)) 
        return g_queue_get_length ( ((tree_node_t*)node->data)->bags );
    else {
        GNode *child = g_node_first_child (node);
        while (child) {
            count += node_size (child);
            child = child->next;
        }
    }
    return count;
}

float node_distance (GNode *node, unsigned char *data)
{
    tree_node_t *nd = (tree_node_t*)node->data;
    return 1.0 * sqrt (1.0*vect_sqdist_char (nd->cc, data, nd->cc_size));
}

float node_distance_d (GNode *node, float *data)
{
    tree_node_t *nd = (tree_node_t*)node->data;
    return 1.0 * sqrt (1.0*vect_sqdist_float (nd->cc, data, nd->cc_size));
}

float tree_node_to_node_distance (GNode *n1, GNode *n2)
{
    tree_node_t *nd1 = (tree_node_t*)n1->data;
    tree_node_t *nd2 = (tree_node_t*)n2->data;
    assert (nd1->cc_size == nd2->cc_size);
    return 1.0*sqrt (1.0 * vect_sqdist_float (nd1->cc, nd2->cc, nd1->cc_size));
}

/* convert a set of bags ("words") into a node
*/
tree_node_t *node_create_from_bags (GQueue *bags)
{
    assert (bags);

    tree_node_t *node = (tree_node_t*)malloc(sizeof(tree_node_t));
    node->cc_size = bag_feature_size (bags);
    node->cc = (float*)malloc(node->cc_size*sizeof(float));
    for (int i=0;i<node->cc_size;i++)
        node->cc[i] = .0;
    node->bags = g_queue_copy (bags);
    bag_t_centroid (node->bags, node->cc, node->cc_size);
    node->id = g_nid;
    g_nid++;
    return node;
}

/* convert a single bag ("word") into a node
*/
tree_node_t *node_create_from_bag (bag_t *bag)
{
    assert (bag);
    assert (bag->cc);

    tree_node_t *node = (tree_node_t*)malloc(sizeof(tree_node_t));
    node->cc_size = bag->cc_size;
    node->cc = (float*)malloc(node->cc_size*sizeof(float));
    for (int i=0;i<node->cc_size;i++)
        node->cc[i] = .0;
    node->bags = g_queue_new ();
    g_queue_push_head (node->bags, bag);
    bag_t_centroid (node->bags, node->cc, node->cc_size);
    node->id = g_nid;
    g_nid++;
    return node;
}

tree_node_t *node_create_from_void (int size)
{
    tree_node_t *node = (tree_node_t*)malloc(sizeof(tree_node_t));
    node->cc_size = size;
    node->cc = (float*)malloc(node->cc_size*sizeof(float));
    for (int i=0;i<node->cc_size;i++)
        node->cc[i] = .0;
    node->bags = g_queue_new();
    bag_t_centroid (node->bags, node->cc, node->cc_size);
    node->id = g_nid;
    g_nid++;
    return node;
}

/* add a bag ("word") to a node
 */
void node_add_bag (GNode *node, bag_t *bag) 
{
    tree_node_t *node_data = (tree_node_t*)node->data;
    g_queue_push_head (node_data->bags, bag);
    assert (G_NODE_IS_LEAF (node));

}

int node_data_size (GNode *node)
{
    tree_node_t *node_data = (tree_node_t*)node->data;
    return bag_feature_size (node_data->bags);
}

void node_print (GNode *node) 
{
    GNode *child = g_node_first_child (node);
    int index = 0;
    dbg (DBG_CLASS, "node (%d children)", g_node_n_children (node));
    while (child) {
        dbg (DBG_CLASS, "child %d has %d el.", index, node_size (child));
        index++;
        child = child->next;
    }
}

/* init the k-mean algorithm on a node
 * with a branching factor K (K children)
 */
void node_kmean_init (GNode *node, unsigned int K)
{
    GList *iter;
    int index=0;
    tree_node_t *nd = (tree_node_t*)node->data;

    // create the children (empty)
    for (unsigned int i=0;i<K;i++) {
        tree_node_t *child = node_create_from_void (node_data_size (node));
        g_node_insert_data (node, -1, child);
    }
    assert (K == g_node_n_children (node));

    // distribute bags evenly between children
    // while removing them from the parent node
    iter = g_queue_pop_head_link (nd->bags);
    while (iter) {
        bag_t *bag = (bag_t*)iter->data;
        GNode *child = g_node_nth_child (node, index);
        node_add_bag (child, bag);
        index = (index + 1) % K;
        iter = g_queue_pop_head_link (((tree_node_t*)node->data)->bags);
    }

}

/* K-mean inner loop
 */
void node_kmean_single_run (GNode *node, gboolean *changed)
{
    GNode *child1, *child2;
    GList *iter, *iter1;
    *changed = 0;

    // update centroid for each child node
    child1 = g_node_first_child (node);
    while (child1) {
        assert (G_NODE_IS_LEAF (child1));
        tree_node_t *nd = (tree_node_t*)child1->data;
        bag_t_centroid (nd->bags, nd->cc, nd->cc_size);
        child1 = child1->next;
    }

    // move bags across nodes
    for (unsigned int i1=0;i1<g_node_n_children (node);i1++) {
        child1 = g_node_nth_child (node, i1);
        tree_node_t *nd1 = (tree_node_t*)child1->data;
        GQueue *toremove = g_queue_new ();
        for (iter1=g_queue_peek_head_link (nd1->bags);iter1;iter1=g_list_next(iter1)) {
            bag_t *b1 = (bag_t*)iter1->data;

            GNode *cnode=child1;
            float mindist = node_distance_d (child1, b1->cc);
            for (unsigned int i2=0;i2<g_node_n_children (node);i2++) {
                if (i1 == i2) continue;
                child2 = g_node_nth_child (node, i2);
                float dtonode2 = node_distance_d (child2, b1->cc);
                if (dtonode2 < mindist - 1E-6) {
                    cnode = child2;
                    mindist = dtonode2;
                }
            }

            // move bag
            if (cnode != child1) {
                tree_node_t *ndc = (tree_node_t*)cnode->data;
                g_queue_push_head (ndc->bags, b1);
                g_queue_push_head (toremove, b1);
                *changed = TRUE;
            }
        }

        // remove nodes from child1
        iter = g_queue_pop_head_link (toremove);
        while (iter) {
            g_queue_remove (nd1->bags, (bag_t*)iter->data);
            iter = g_queue_pop_head_link (toremove);
        }
        g_queue_free (toremove);
    }


#if BAG_DBG
    // sanity check
    GNode *child = g_node_first_child (node);
    while (child) {
        tree_node_t *nd = (tree_node_t*)child->data;
        for (GList *iter=g_queue_peek_head_link (nd->bags);iter;iter=g_list_next(iter)) {
            bag_t *b = (bag_t*)iter->data;
            float d1 = node_distance_d (child, b->cc); 
            GNode *child2 = g_node_first_child (node);
            while (child2) {
                if (child2!=child) {
                    float d2 = node_distance_d (child2, b->cc);
                    assert (d1 < d2 + 1E-6);
                }
                child2 = child2->next;
            }
        }
        child = child->next;
    }
#endif
}

/* K-mean full run
 */
gboolean node_kmean_full_run (GNode *node, gpointer data)
{
    gboolean changed = TRUE;

    int size = g_queue_get_length ( ((tree_node_t*)node->data)->bags );

    if (size < BAGS_MAX_CHILDREN) return FALSE;

    int K = BAGS_KMEAN_N_CHILDREN;

    // init the children nodes
    node_kmean_init (node, K);

    // update the children
    int nruns = 0;
    while (changed && nruns < 100) {
        node_kmean_single_run  (node, &changed);
        nruns++;
    }

    return FALSE;
}

/* Run K-mean on all nodes of a tree.
 * only leave nodes are considered.
 * always returns FALSE (i.e. never stop the traverse)
 */
gboolean tree_t_node_update (GNode *node, gpointer data)
{
    if (G_NODE_IS_LEAF (node))
        node_kmean_full_run (node, NULL);

    return FALSE;
}

void tree_t_node_update_full (GNode *node)
{
    GNode *root = g_node_get_root (node);

    g_node_traverse (root, G_POST_ORDER, G_TRAVERSE_ALL, -1, tree_t_node_update, NULL);
}

/* search for a bag in a node
 */
gboolean tree_child_contains_bag (GNode *node, bag_t *bag)
{
    if (!G_NODE_IS_LEAF (node)) return FALSE;

    tree_node_t *node_data = (tree_node_t*)(node->data);
    for (GList *iter=g_queue_peek_head_link(node_data->bags);iter;iter=g_list_next(iter)) {
        bag_t *b = (bag_t*)iter->data;
        if (b==bag) return TRUE;
    }
    return FALSE;
}

void tree_print_info (GNode *node)
{
    dbg (DBG_CLASS, "Tree has %d leaf nodes, depth %d", g_node_n_nodes (node, G_TRAVERSE_LEAVES), g_node_max_height (node));
}

GNode *tree_init_from_void (int size)
{
    tree_node_t *node = node_create_from_void (size);

    return g_node_new (node);
}

/* initialize a tree given a set of bags ("words")
 */
GNode *tree_init_from_bags (GQueue *bags)
{
    if (g_queue_is_empty (bags))
        return NULL;

    dbg (DBG_CLASS, "initializing tree with %d bags...", g_queue_get_length (bags));

    tree_node_t *node = node_create_from_bags (bags);

    return g_node_new (node);
}

/* initialize a tree given a single bag ("word")
 */
GNode *tree_init_from_bag (bag_t *bag)
{
    dbg (DBG_CLASS, "initializing tree with 1 bag...");

    tree_node_t *node = node_create_from_bag (bag);

    return g_node_new (node);
}

static GNode *g_node;

gboolean tree_find_node_func (GNode *node, gpointer data)
{
    bag_t *bag = (bag_t*)data;
    for (GList *iter=g_queue_peek_head_link (((tree_node_t*)node->data)->bags);iter;iter=g_list_next(iter)) {
        bag_t *b = (bag_t*)iter->data;
        int d = vect_sqdist_float (b->cc, bag->cc, b->cc_size);
        if (d < 1E-6 && bag->n == b->n) {
            g_node = node;
            return TRUE;
        }
    }
    return FALSE;
}

/* find the node that has a given bag
 * <node> is any given node in the tree
 */
void tree_find_node (GNode *node, bag_t *bag)
{
    GNode *root = g_node_get_root (node);
    g_node = NULL;

    g_node_traverse (root, G_PRE_ORDER, G_TRAVERSE_ALL, -1, tree_find_node_func, bag);
}


gboolean tree_t_print_func (GNode *node, gpointer data)
{
    if (node->prev) return FALSE;

    GNode *sib = node;
    while (sib) {

        tree_node_t *nd = (tree_node_t*)sib->data;
        printf ("[%d]", nd->id);

        if (G_NODE_IS_LEAF (sib)) {

            printf ("*%d* (%.3f %.3f %.3f) ", node_size(sib), nd->cc[0], nd->cc[1], nd->cc[2]);
        } else {
            printf ("%d ", node_size (sib));
        }
        sib = sib->next;
    }
    printf ("\n");
    fflush (stdout);

    return FALSE;
}


void tree_t_print (GNode *node)
{
    printf ("tree has %d leaf nodes and max height %d\n", g_node_n_nodes (node, G_TRAVERSE_LEAVES), g_node_max_height (node));

    g_node_traverse (node, G_LEVEL_ORDER, G_TRAVERSE_ALL, -1, tree_t_print_func, NULL);

    printf ("\n");
}

gboolean tree_t_count_bags_func (GNode *node, gpointer data)
{
    int *count = (int*)data;
    *count = *count + g_queue_get_length (((tree_node_t*)node->data)->bags);
    return FALSE;
}

/* count bags ("words") in a tree
 */
int tree_t_count_bags (GNode *node)
{
    int count=0;
    g_node_traverse (node, G_PRE_ORDER, G_TRAVERSE_LEAVES, -1, tree_t_count_bags_func, &count);
    return count;
}

gboolean tree_t_destroy_func (GNode *node, gpointer data)
{
    tree_node_t *nd = (tree_node_t*)node->data;

    if (nd->cc)
        free (nd->cc);

    bags_destroy (nd->bags);

}

void tree_t_destroy (GNode *node)
{
    g_node_traverse (node, G_PRE_ORDER, G_TRAVERSE_LEAVES, -1, tree_t_destroy_func, NULL);
}

gboolean tree_t_collect_bags_func (GNode *node, gpointer data)
{
    GQueue *queue = (GQueue*)data;
    tree_node_t *nd = (tree_node_t*)node->data;
    for (GList *iter=g_queue_peek_head_link (nd->bags);iter;iter=g_list_next(iter)) {
        bag_t *b = (bag_t*)iter->data;
        g_queue_push_head (queue, b);
    }
    return FALSE;
}

GQueue * tree_t_collect_bags (GNode *node)
{
    GQueue *queue = g_queue_new ();
    g_node_traverse (node, G_PRE_ORDER, G_TRAVERSE_LEAVES, -1, tree_t_collect_bags_func, queue);
    return queue;
}

void bags_init (GQueue *bags, navlcm_feature_list_t *features, int id)
{
    for (int i=0;i<features->num;i++) {
        bag_t *newbag = (bag_t*)malloc(sizeof(bag_t));
        bag_t_init (newbag, &features->el[i], id);
        g_queue_push_tail (bags, newbag);
    }

    dbg (DBG_CLASS, "voctree init with %d bags.", features->num);
}

void bags_append (GQueue *bags, GQueue *features, int id)
{
    dbg (DBG_CLASS, "creating %d new bags with id %d", g_queue_get_length (features), id);

    for (GList *iter=g_queue_peek_head_link (features);iter;iter=iter->next) {
        navlcm_feature_t *f = (navlcm_feature_t*)iter->data;
        bag_t *newbag = (bag_t*)malloc(sizeof(bag_t));
        bag_t_init (newbag, f, id);
        g_queue_push_tail (bags, newbag);
    }
}

void bags_destroy (GQueue *bags)
{
    for (GList *iter=g_queue_peek_head_link (bags);iter;iter=iter->next) {
        bag_t *b = (bag_t*)iter->data;
        bag_t_destroy (b);
    }
    g_queue_free (bags);

}

void bags_update (GQueue *bags, int id)
{
    for (GList *iter=g_queue_peek_head_link (bags);iter;iter=iter->next) {
        bag_t *bag = (bag_t*)iter->data;
        g_queue_push_head (bag->ind, GINT_TO_POINTER (id));
        bag->n++;
    }
}

/* input:   a vocabulary : <bags>
 *          a list of features
 *          a search radius
 * output:  create new words for features that were not matched and 
 *          return the list of matched words
 * optional: store unmatched features in <qfeatures>
 */
void bags_naive_search (GQueue *bags, navlcm_feature_list_t *features, double search_radius, 
        GQueue *qfeatures, GQueue *qbags, double *usecs, gboolean use_mkl)
{
    GTimer *timer = g_timer_new ();
    double secs;

    int nbags = g_queue_get_length (bags);

    // convert features to matrix
    float *m1 = (float*)malloc(features->num*features->desc_size*sizeof(float));
    for (int i=0;i<features->num;i++) {
        for (int j=0;j<features->desc_size;j++) {
            m1[i*features->desc_size+j] = features->el[i].data[j];
        }
    }

    // convert bags descriptors to matrix
    float *m2 = (float*)malloc(features->desc_size*nbags*sizeof(float));
    GList *iter = g_queue_peek_head_link (bags);
    for (int j=0;j<nbags;j++) {
        bag_t *bag = (bag_t*)iter->data;
        for (int i=0;i<features->desc_size;i++) {
            m2[i*nbags+j] = bag->cc[i];
        }
        iter = iter->next;
    }

    // compute dot product
    float *dotprod = (float*)malloc(features->num*nbags*sizeof(float)); 

    if (use_mkl)
        math_matrix_mult_mkl_float (features->num, nbags, features->desc_size, m1, m2, dotprod);
    else
        math_matrix_mult_naive_float (features->num, nbags, features->desc_size, m1, m2, dotprod);

    unsigned char *visited = (unsigned char *)malloc(nbags);
    for (int i=0;i<nbags;i++)
        visited[i] = 0;

    // find all bags for which the distance to a word < search_radius
    for (int i=0;i<features->num;i++) {
        gboolean seen = FALSE;
        GList *iter = g_queue_peek_head_link (bags);
        double best_dist = 1E6;
        for (int j=0;j<nbags;j++) {
            double dist = 2.0 - 2.0 * dotprod[i*nbags+j];
            best_dist = fmin (best_dist, dist);
            if (dist < search_radius) {
                seen = TRUE;
                // store bag
                bag_t *mybag = (bag_t*)iter->data;
                g_queue_push_head (qbags, mybag);
                visited[j] = 1;
            }
            iter = iter->next;
        }
        
        // if feature has not been seen, create a new word
        if (!seen && qfeatures) {
            g_queue_push_head (qfeatures, &features->el[i]);
        }
    }

    // free memory
    free (m1);
    free (m2);
    free (dotprod);
    free (visited);

    secs = g_timer_elapsed (timer, NULL);

    if (usecs)
        *usecs = secs * 1000000;

    int matched = qbags ? g_queue_get_length (qbags) : 0;
    int unmatched = qfeatures ? g_queue_get_length (qfeatures) : 0;
    int total = bags ? g_queue_get_length (bags) : 0;

    dbg (DBG_CLASS, "voctree update: %d bags, %d matched bags, %d unmatched features, %d total.  ** %.3f secs (%.1f Hz)", nbags, matched, unmatched, total, secs, 1.0/secs);
}

/* input:   a set of matched words
 * output:  a 1-D float array representing the similarity with the gates (represented as 
 *          a list of pointers in each word
 */
double *bags_naive_vote (GQueue *bags, int size)
{
    dbg (DBG_CLASS, "naive vote for %d bags", size);

    double *sim = (double*)malloc((size)*sizeof(double));
    for (int i=0;i<size;i++)
        sim[i] = .0;

    for (GList *iter=g_queue_peek_head_link (bags);iter;iter=iter->next) {
        bag_t *bag = (bag_t*)iter->data;
        double idf = 1.0 / bag->n;//1.0 - log (bag->n) / log (size);
        for (GList *iteri=g_queue_peek_head_link (bag->ind);iteri;iteri=iteri->next) {
            long int id = (long int)(iteri->data);
            sim[id] += idf;
        }
    }

    return sim;

}

////////////////////////////////////////////////////////////////////////////////////
// unit testing
//
navlcm_feature_list_t *bags_random_feature_list (int num, int desc_size)
{
    navlcm_feature_list_t *list = (navlcm_feature_list_t*)calloc(1,sizeof(navlcm_feature_list_t));
    list->num = num;
    list->el = (navlcm_feature_t*)calloc(num, sizeof(navlcm_feature_t));
    list->desc_size = desc_size;
    list->feature_type = NAVLCM_FEATURES_PARAM_T_SIFT2;
    list->sensorid = 0;
    list->utime = 0;
    list->width = 1;
    list->height = 1;
    for (int i=0;i<num;i++) {
        navlcm_feature_t *f = (navlcm_feature_t*)list->el + i;
        f->col = .0;
        f->row = .0;
        f->scale = 1.0;
        f->ori = .0;
        f->data = (float*)malloc(desc_size*sizeof(float));
        f->size = desc_size;
        f->sensorid = 0;
        f->index = 0;
        f->utime = 0;
        f->uid = 0;
        f->laplacian = 0;
        for (int k=0;k<desc_size;k++)
            f->data[k] = fmax (0, fmin (1.0, 1.0*rand()/RAND_MAX));
        math_normalize_float (f->data, desc_size);
    }
    return list;
}

void tree_unit_testing (int num_features, unsigned int N, float bag_radius)
{
    GNode *tree;
    GQueue *bags;

    gboolean merged1, merged2;
    int id=0;

    bags = g_queue_new ();
    tree = tree_init_from_void (128);

    navlcm_feature_list_t *fs = bags_random_feature_list (num_features, 128);

    printf ("inserting %d features.\n", fs->num);
    float missed = 0.0;

    for (int i=0;i<fs->num;i++) {
        navlcm_feature_t *ft = (navlcm_feature_t*)fs->el + i;

        // search feature in bags
        GQueue *dbags1 = g_queue_new ();
        GQueue *dbags2 = g_queue_new ();
        bag_node_t_search_feature (bags, NULL, ft, N, bag_radius, dbags1);
        bag_node_t_search_feature (NULL, tree, ft, N, bag_radius, dbags2);
        GQueue *diffbags1 = bag_t_diff (dbags1, dbags2);
        GQueue *diffbags2 = bag_t_diff (dbags1, dbags2);
        if (!g_queue_is_empty (diffbags1) || !g_queue_is_empty (diffbags2)) {
            missed += 1.0;
        }
        for (GList *iter=g_queue_peek_head_link (diffbags1);iter;iter=g_list_next(iter)) {
#if BAG_DBG
            bag_t *mb = (bag_t*)iter->data;
            tree_find_node (tree, mb);
            assert (g_node);
            assert (g_node->parent);
            // find closest child to ft
            GNode *cnode = tree_find_closest_child (g_node->parent, ft);
            assert (cnode);
            assert (cnode != g_node);
            // compute barrier distance
            float d = node_frontier_distance (cnode, g_node, ft);
            float dc = tree_node_to_node_distance (cnode, g_node);
            float ntoc = node_distance_d (cnode, ft->data);
            float ntog = node_distance_d (g_node, ft->data);
            printf ("dist: %.4f  %.4f %.4f %.4f radius: %.4f\n", d, dc, ntoc, ntog, bag_radius);
            assert (false);
#endif
        }
        g_queue_free (dbags1);
        g_queue_free (dbags2);
        g_queue_free (diffbags1);
        g_queue_free (diffbags2);

        // insert feature in bags
        merged1 = bag_node_t_insert_feature (bags, NULL, ft, N, bag_radius, id, NULL);

        // insert feature in tree
        merged2 = bag_node_t_insert_feature (NULL, tree, ft, N, bag_radius, id, NULL);

        // update the tree
        tree_t_node_update_full (tree);

    }

    missed /= fs->num;

    printf ("missed: %.2f %%\n", missed*100.0);
}

/* mode = 1: naive search
 * mode = 2: naive search (MKL-optimized)
 * mode = 3: tree-based search
 *
 * nsets: number of feature sets (e.g 1000)
 * nfeatures: number of features per set (e.g 500)
 * N: maximum number of children search (for tree-based search only)
 */
void bags_performance_testing (int mode, int nsets, int nfeatures, unsigned int N, char *filename)
{
    int desc_size = 128;
    double bag_word_radius = .3;
    FILE *fp;
    
    srand (time (NULL));

    printf ("generating %d sets of %d features...\n", nsets, nfeatures);

    // generate random features
    GQueue *features = g_queue_new ();
    for (int i=0;i<nsets;i++) {
        navlcm_feature_list_t *fs = bags_random_feature_list (nfeatures, 128);
        g_queue_push_tail (features, fs);
    }


    if (mode == 1 || mode == 2) {
    // naive search
        printf ("naive search...\n");
        GQueue *voctree = g_queue_new ();
        int count=0;

        fp = fopen (filename, "w");

        for (GList *iter=g_queue_peek_head_link (features);iter;iter=iter->next) {
            navlcm_feature_list_t *fs = (navlcm_feature_list_t*)iter->data;

            if (g_queue_is_empty (voctree)) {
                printf ("initializing vocabulary with %d features...\n", fs->num);
                bags_init (voctree, fs, 0);
            }


            // search features in vocabulary tree
            GQueue *qfeatures = g_queue_new (); // features that are un-matched (and yield new bags)
            GQueue *qbags = g_queue_new ();     // bags that found a match
            double usecs=0;
            if (mode == 1)
                bags_naive_search (voctree, fs, bag_word_radius, qfeatures, qbags, &usecs, 0);
            else
                bags_naive_search (voctree, fs, bag_word_radius, qfeatures, qbags, &usecs, 1);

            // create new bags for unmatched features
            bags_append (voctree, qfeatures, 0);
            fprintf (fp, "%d %.5f\n", g_queue_get_length (voctree), usecs/1000000);
            fflush (fp);

            g_queue_free (qfeatures);
            g_queue_free (qbags);

            printf ("mode = %d. progress: %.1f %%  vocabulary size: %d\n", mode, 100.0*count/nsets, g_queue_get_length (voctree));
            count++;
        }

        bags_destroy (voctree);

        fclose (fp);
    }

    if (mode == 3) {
        // tree-based search
        GNode *tree = NULL;
        int count=0;

        fp = fopen (filename, "w");

        for (GList *iter=g_queue_peek_head_link (features);iter;iter=iter->next) {
            navlcm_feature_list_t *fs = (navlcm_feature_list_t*)iter->data;
            if (!tree) tree = tree_init_from_void (desc_size);

            GTimer *timer = g_timer_new ();

            bag_tree_t_insert_feature_list (NULL, tree, fs, N, bag_word_radius, 0);

            double secs = g_timer_elapsed (timer, NULL);

            int nwords = tree_t_count_bags (tree);

            fprintf (fp, "%d %.5f\n", nwords, secs);
            fflush (fp);

            printf ("mode = %d. progress: %.1f %%  vocabulary size: %d\n", mode, 100.0*count/nsets, nwords);
            count++;
        }

        tree_t_destroy (tree);

        fclose (fp);
    }

    // clear memory
    for (GList *iter=g_queue_peek_head_link (features);iter;iter=iter->next) {
        navlcm_feature_list_t *fs = (navlcm_feature_list_t*)iter->data;
        navlcm_feature_list_t_destroy (fs);
    }
    g_queue_free (features);

}



