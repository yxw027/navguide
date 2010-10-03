#include "corrmat.h"

/* Smith & Waterman algorithm described in
 * Ho & Newman, Detecting Loop Closure with Scene Sequences, IJCV’07
 *
 * The algorithm takes as input the similarity matrix and transforms it into 
 * an alignment matrix, from which components (node sequences alignments) are extracted.
 */
double *corrmat_init (int n, double val)
{
    double *m = (double *)malloc(n*n*sizeof(double));
    for (int i=0;i<n;i++) {
        for (int j=0;j<n;j++) {
            CORRMAT_SET (m, n, i, j, val);
        }
    }

    return m;
}

int *corrmat_int_init (int n, int val)
{
    int *m = (int *)malloc(n*n*sizeof(int));
    for (int i=0;i<n;i++)
        for (int j=0;j<n;j++)
            CORRMAT_SET (m, n, i, j, val);

    return m;
}

void corrmat_print (double *m, int n)
{
    for (int i=0;i<n;i++) {
        for (int j=0;j<n;j++) {
            printf ("%.4f ", CORRMAT_GET (m, n, i, j));
        }
        printf ("\n");
    }
}

/* <n> is the current size of matrix <m>
 */
double *corrmat_resize_once (double *m, int *n, double val)
{
    double *p = corrmat_init (*n+1, val);

    for (int i=0;i<*n;i++) {
        for (int j=0;j<*n;j++) {
            assert (m);
            CORRMAT_SET (p, *n+1, i, j, CORRMAT_GET (m, *n, i, j));
        }
    }
   
    free (m);
    *n = *n + 1;

    return p;
}

void corrmat_write (double *corrmat, int n, const char *filename)
{
    FILE *fp = fopen (filename, "w");
    if (!fp) {
        dbg (DBG_ERROR, "failed to open %s in write mode.", filename);
        return;
    }

    dbg (DBG_CLASS, "writing corr. matrix %d x %d to file %s", n, n, filename);

    for (int i=0;i<n;i++) {
        for (int j=0;j<n;j++) {
            fprintf (fp, "%f ", CORRMAT_GET (corrmat, n, i, j));
        }
        fprintf (fp, "\n");
    }

    fclose (fp);
}

void corrmat_int_write (int *corrmat, int n, const char *filename)
{
    FILE *fp = fopen (filename, "w");
    if (!fp) {
        dbg (DBG_ERROR, "failed to open %s in write mode.", filename);
        return;
    }

    dbg (DBG_CLASS, "writing corr. matrix %d x %d to file %s", n, n, filename);

    for (int i=0;i<n;i++) {
        for (int j=0;j<n;j++) {
            fprintf (fp, "%d ", CORRMAT_GET (corrmat, n, i, j));
        }
        fprintf (fp, "\n");
    }

    fclose (fp);
}

double *corrmat_read (const char *filename, int *n)
{
    *n = 0;

    FILE *fp = fopen (filename, "r");
    if (!fp) {
        dbg (DBG_ERROR, "failed to open %s in read mode.", filename);
        return NULL;
    }

    int size = 0;
    double v;
    while (fscanf (fp, "%lf", &v) == 1) {
        size++;
    }

    fclose (fp);

    fp = fopen (filename, "r");
    double *m = (double*)malloc(size*sizeof(double));

    int count=0;
    while (fscanf (fp, "%lf", &v) == 1) {
        m[count] = v;
        count++;
    }

    fclose (fp);

    *n = (int)(sqrt(size));

    return m;
}

void corrmat_smoothen (double *corrmat, int n)
{
    double *h = corrmat_copy (corrmat, n);
    for (int i=0;i<n;i++)
        for (int j=0;j<n;j++)
            CORRMAT_SET (h, n, i, j, .0);

    for (int i=0;i<n;i++) {
        dbg (DBG_CLASS, "i=%d", i);
        for (int k=0;k<n;k++) {

            double cik = CORRMAT_GET (corrmat, n, i, k);

            for (int j=i+1;j<n;j++) {
                for (int l=0;l<n;l++) {

                    double cjl = CORRMAT_GET (corrmat, n, j, l);

                    if (fabs (i-j) > 10) continue;
                    if (fabs (fabs (i-j)-fabs(l-k)) > 10) continue;

                    double cijkl = (cik + cjl)/2.0;
                    gboolean rev = l < k;

                    int imin = i > 2 ? i-2 : 0;
                    int kmin = MIN (k,l) > 2 ? MIN(k,l)-2 : 0;
                    int imax = j < n-2 ? j+2 : n-1;
                    int kmax = MAX (k,l) < n-2 ? MAX(k,l)+2 : n-1;

                    for (int ii=imin;ii<=imax;ii++) {
                        
                        int kk = nearbyint (rev ? 
                                kmax - 1.0 * (ii-imin)/(imax-imin)*(kmax-kmin) :
                                kmin + 1.0 * (ii-imin)/(imax-imin)*(kmax-kmin));

                        assert (0 <= kk && kk < n);

                        CORRMAT_SET (h, n, ii, kk, CORRMAT_GET (h, n, ii, kk) + cijkl);

                    }

                    for (int kk=kmin;kk<=kmax;kk++) {
                        
                        int ii = nearbyint (rev ? 
                                imax - 1.0 * (kk-kmin)/(kmax-kmin)*(imax-imin) :
                                imin + 1.0 * (kk-kmin)/(kmax-kmin)*(imax-imin));

                        assert (0 <= ii && ii < n);

                        CORRMAT_SET (h, n, ii, kk, CORRMAT_GET (h, n, ii, kk) + cijkl);

                    }
                }
            }
        }
    }


    // normalize
       for (int i=0;i<n;i++) {
       double total = .0;
       for (int j=0;j<n;j++)
       total += CORRMAT_GET (h, n, i, j);
       for (int j=0;j<n;j++)
       CORRMAT_SET (h, n, i, j, CORRMAT_GET (h, n, i, j) / total);
       }

    // set diagonal to zero
    for (int i=0;i<n;i++) {
        for (int j=i-5;j<n;j++)
            if (0 <= j)
                CORRMAT_SET (h, n, i, j, .0);
    }

    for (int i=0;i<n;i++)
        for (int j=0;j<n;j++)
            CORRMAT_SET (corrmat, n, i, j, CORRMAT_GET (h, n, i, j));

    free (h);
}

void corrmat_compute_alignment_matrix (double *corrmat, int n, double thresh, double delta, gboolean rev, double **hout, int **hindout)
{
    // init the data
    double *h = corrmat_init (n, .0);
    int *hind = corrmat_int_init (n, 0);

    // apply filter
    for (int i=0;i<n;i++) 
        for (int j=0;j<n;j++) 
            if (CORRMAT_GET (corrmat, n, i, j) < thresh)
                CORRMAT_SET (corrmat, n, i, j, -2.0);

    // compute alignment matrix
    for (int i=0;i<n;i++) {
        int jmax = rev ? n-1-i : i;

        int diagsize = 0;

        for (int j=0;j<jmax;j++) {

            if (!rev && fabs (j-i) < diagsize) {
                CORRMAT_SET (h, n, i, j, .0);
                continue;
            }

            if (rev && fabs (j-n-1+i) < diagsize) {
                CORRMAT_SET (h, n, i, j, .0);
                continue;
            }
            if (CORRMAT_GET (corrmat, n, i, j) < .0) {
                CORRMAT_SET (h, n, i, j, .0);
                continue;
            }
            if (i==0 || j==0) {
                CORRMAT_SET (h, n, i, j, CORRMAT_GET (corrmat, n, i, j));
                CORRMAT_SET (hind, n, i, j, 1);
                continue;
            }

            double h1,h2,h3;
            h1 = CORRMAT_GET (h, n, i-1, j-1);
            h2 = CORRMAT_GET (h, n, i, j-1);
            h3 = CORRMAT_GET (h, n, i-1, j);

            assert (h1>-1.0);
            assert (h2>-1.0);
            assert (h3>-1.0);

            if (h2 > fmax (h1,h3)) {
                CORRMAT_SET (h, n, i, j, h2 + CORRMAT_GET (corrmat, n, i, j) - delta);
                CORRMAT_SET (hind, n, i, j, 2);
            } else if (h3 > fmax (h1,h2)) {
                CORRMAT_SET (h, n, i, j, h3 + CORRMAT_GET (corrmat, n, i, j) - delta);
                CORRMAT_SET (hind, n, i, j, 3);
            } else {
                CORRMAT_SET (h, n, i, j, h1 + CORRMAT_GET (corrmat, n, i, j));
                CORRMAT_SET (hind, n, i, j, 1);
            }
        }
    }

    *hout = h;
    *hindout = hind;
}

double *corrmat_copy (double *mat, int n)
{
    double *h = corrmat_init (n, .0);
    for (int i=0;i<n;i++) {
        for (int j=0;j<n;j++) {
            CORRMAT_SET (h, n, i, j, CORRMAT_GET (mat, n, i, j));
        }
    }

    return h;
}

void corrmat_convolve (double *m, int size, double *kern, int ksize)
{
    double *mc = corrmat_copy (m, size);
    double sum = .0;
    for (int i=0;i<ksize*ksize;i++)
        sum += fabs(kern[i]);

    for (int i=ksize/2;i<size-ksize/2;i++) {
        for (int j=ksize/2;j<size-ksize/2;j++) {
            double v = .0;
            for (int ii=0;ii<ksize;ii++) {
                for (int jj=0;jj<ksize;jj++) {
                    double k = kern[ii*ksize+jj];
                    v += k * CORRMAT_GET (mc, size, i+ii-ksize/2, j+jj-ksize/2);
                }
            }
            CORRMAT_SET (m, size, i, j, v/sum);
        }
    }
    free (mc);
}

int *corrmat_int_copy (int *mat, int n)
{
    int *h = corrmat_int_init (n, .0);
    for (int i=0;i<n;i++) {
        for (int j=0;j<n;j++) {
            CORRMAT_SET (h, n, i, j, CORRMAT_GET (mat, n, i, j));
        }
    }

    return h;
}

void corrmat_reverse (double *mat, int n)
{
    double *copy = corrmat_copy (mat, n);

    for (int i=0;i<n;i++)
        for (int j=0;j<n;j++)
            CORRMAT_SET (mat, n, i, j, CORRMAT_GET (copy, n, n-1-i, j));

    free (copy);
}

void corrmat_int_reverse (int *mat, int n)
{
    int *copy = corrmat_int_copy (mat, n);

    for (int i=0;i<n;i++)
        for (int j=0;j<n;j++)
            CORRMAT_SET (mat, n, i, j, CORRMAT_GET (copy, n, n-1-i, j));

    free (copy);
}

void corrmat_threshold (double *h, int n, double s)
{
    for (int i=0;i<n;i++)
        for (int j=0;j<n;j++)
            CORRMAT_SET (h, n, i, j, fmax (s, CORRMAT_GET (h, n, i, j) + s));
}

void corrmat_shift (double *h, int n, double s)
{
    for (int i=0;i<n;i++)
        for (int j=0;j<n;j++)
            CORRMAT_SET (h, n, i, j, CORRMAT_GET (h, n, i, j) + s);
}

double corrmat_min (double *h, int n)
{
    double minval = CORRMAT_GET (h, n, 0, 0);
    for (int i=0;i<n;i++)
        for (int j=0;j<n;j++)
            minval = fmin (minval, CORRMAT_GET (h, n, i, j));

    return minval;
}

double corrmat_max (double *h, int n)
{
    double maxval = CORRMAT_GET (h, n, 0, 0);
    for (int i=0;i<n;i++)
        for (int j=0;j<n;j++)
            maxval = fmax (maxval, CORRMAT_GET (h, n, i, j));

    return maxval;
}

double corrmat_local_max (double *h, int n, int radius, int ri, int rj)
{
    double dval = CORRMAT_GET (h, n, ri, rj);

    for (int ii=0;ii<=2*radius;ii++) {
        for (int jj=0;jj<=2*radius;jj++) {
            if (ii==radius && jj==radius) continue;
            int key = ri + ii - radius;
            int val = rj + jj - radius;
            if (key < 0 || val < 0 || n-1 < key || n-1 < val) continue;
            if (CORRMAT_GET (h, n, key, val) > dval + 1E-6)
                return .0;
        }
    }
    return dval;
}

int corrmat_find_global_max (double *h, int n, int *maxi, int *maxj, double *v) {

    double maxval = .0;
    *maxi = -1;
    *maxj = -1;

    for (int i=0;i<n;i++) {
        for (int j=0;j<n;j++) {
            double v = CORRMAT_GET (h, n, i, j);
            if (*maxi == -1 || maxval < v) {
                maxval = v;
                *maxi = i;
                *maxj = j;
            }
        }
    }

    *v = maxval;

    if (maxval > 1E-6)
        return 1;
    else
        return 0;
}

component_t *component_t_new ()
{
    component_t *c = (component_t*)malloc(sizeof(component_t));
    c->pt = g_queue_new ();
    c->score = .0;
    c->reverse = 0;
    return c;
}

void component_t_print (component_t *c)
{
    printf ("------ component (%d el., score: %.3f) -------\n", g_queue_get_length (c->pt), c->score);
    for (GList *iter=g_queue_peek_head_link (c->pt);iter;iter=g_list_next(iter)) {
        pair_int_t *p = (pair_int_t*)iter->data;
        printf ("%d %d\n", p->key, p->val);
    }
}

void component_t_free (component_t *c)
{
    for (GList *iter=g_queue_peek_head_link (c->pt);iter;iter=g_list_next(iter)) 
        free ((pair_int_t*)iter->data);
    g_queue_free (c->pt);
}

int component_t_cmp (gconstpointer a, gconstpointer b, gpointer data)
{
    component_t *ca = (component_t*)a;
    component_t *cb = (component_t*)b;
    if (ca->score < cb->score)
        return -1;
    return 1;
}

gboolean component_t_valid (component_t *c, int n, int radius, double max_slope_error)
{
    // slope test
    pair_int_t *phead = (pair_int_t*)g_queue_peek_head (c->pt);
    pair_int_t *ptail = (pair_int_t*)g_queue_peek_tail (c->pt);
    int dx = ptail->key-phead->key;
    int dy = ptail->val - phead->val;
    double slope = atan2 (dx, dy);
    // 135 degrees corresponds to the diagonal direction in the matrix
    // it is not a parameter of the method
    double slope_error = c->reverse ? fabs (slope - to_radians (135.0)) : fabs (slope + to_radians (135.0));
    printf ("slope: %.4f  error: %.4f\n", to_degrees (slope), to_degrees (slope_error));
    if (slope_error > to_radians (max_slope_error))
        return FALSE;

    // for non-reverse sequences, test distance to diagonal
    if (c->reverse)
        return TRUE;

    for (GList *iter=g_queue_peek_head_link (c->pt);iter;iter=g_list_next(iter)) {
        pair_int_t *p = (pair_int_t*)iter->data;
        if (fabs (p->key-p->val) > radius) return TRUE;
    }
    return FALSE;
}

int corrmat_find_component (double * hmat, int * hmatind, double * hmatrev, int* hmatindrev, int n, component_t *comp, double *mask1, double *mask2, double tail_min_thresh, int alignment_min_node_id)
{
    int maxi, maxj;
    double val;
    gboolean rev = FALSE;

    // search for global maximum in the mask
    int ok = corrmat_find_global_max (mask1, n, &maxi, &maxj, &val);
    if (!ok) {
        ok = corrmat_find_global_max (mask2, n, &maxi, &maxj, &val);
        rev = TRUE;
    }

    if (!ok) return -1;

    int *hind = rev ? hmatindrev : hmatind;
    double *h = rev ? hmatrev : hmat;

    // back-trace through matrix
    printf ("==================== rev = %d =====================\n", rev);
    printf ("local maxima at %d,%d val %.4f\n", maxi, maxj, val);
    comp->score = val;
    comp->reverse = rev;

    while (1) {
        if (maxi < 0 || maxj < 0 || n-1 < maxi || n-1 < maxj) break;
        double val = CORRMAT_GET (h, n, maxi, maxj);
        int valind = CORRMAT_GET (hind, n, maxi, maxj);
        if (valind == 0 || val < tail_min_thresh) { printf ("stop: [%d][%d] %.3f %d\n", maxi, maxj, val, valind); break;}
        if (maxi > alignment_min_node_id || maxj > alignment_min_node_id) {
            pair_int_t *p = (pair_int_t*)malloc(sizeof(pair_int_t));
            p->key = maxi;
            p->val = maxj;
            g_queue_push_tail (comp->pt, p);
        }
        printf ("[%d][%d] %.3f %d\n", maxi, maxj, val, valind);

        assert (0 <= maxi && maxi <= n-1);
        assert (0 <= maxj && maxj <= n-1);

        // mark as visited
        if (rev)
            CORRMAT_SET (mask2, n, maxi, maxj, .0);
        else
            CORRMAT_SET (mask1, n, maxi, maxj, .0);

        if (CORRMAT_GET (hind, n, maxi, maxj) == 1) { if (rev) maxi++; else maxi--; maxj--; continue;}
        if (CORRMAT_GET (hind, n, maxi, maxj) == 2) { maxj--; continue;}
        if (CORRMAT_GET (hind, n, maxi, maxj) == 3) { if (rev) maxi++; else maxi--; continue;}

    }

    printf ("length: %d\n", g_queue_get_length (comp->pt));

    return 0;
}

void corrmat_find_component_list (double * hmat, int * hmatind, double * hmatrev, int* hmatindrev, int n, double alignment_threshold, double alignment_tail_thresh, int min_seq_length, int alignment_min_diag_distance, double alignment_max_slope_error, int alignment_search_radius, int alignment_min_node_id, GQueue *components)
{
    // generate mask
    double *mask1 = corrmat_init (n, .0);
    double *mask2 = corrmat_init (n, .0);

    dbg (DBG_CLASS, "alignment threshold: %.5f  tail threshold: %.5f  min seq length: %d  min diag distance: %d  max slope error: %.4f",
            alignment_threshold, alignment_tail_thresh, min_seq_length, alignment_min_diag_distance, alignment_max_slope_error);

    // find local maxima and set the rest to zero
    for (int i=0;i<n;i++) {
        for (int j=0;j<n;j++) {
            CORRMAT_SET (mask1, n, i, j, corrmat_local_max (hmat, n, alignment_search_radius, i, j));
            CORRMAT_SET (mask2, n, i, j, corrmat_local_max (hmatrev, n, alignment_search_radius, i, j));
            if (CORRMAT_GET (mask1, n, i, j) < alignment_threshold)
                CORRMAT_SET (mask1, n, i, j, .0);
            if (CORRMAT_GET (mask2, n, i, j) < alignment_threshold)
                CORRMAT_SET (mask2, n, i, j, .0);
            if (CORRMAT_GET (mask1, n, i, j) > 1E-6)
                printf ("mask1[%d][%d] = %.4f\n", i, j, CORRMAT_GET (mask1, n, i, j));
            if (CORRMAT_GET (mask2, n, i, j) > 1E-6)
                printf ("mask2[%d][%d] = %.4f\n", i, j, CORRMAT_GET (mask2, n, i, j));
        }
    }

    corrmat_write (mask1, n, "mask1.txt");

    while (1) {
        component_t *comp = (component_t*)malloc(sizeof(component_t));
        comp->pt = g_queue_new ();
        comp->score = .0;

        // find next component
        if (corrmat_find_component (hmat, hmatind, hmatrev, hmatindrev, n, comp, mask1, mask2, alignment_tail_thresh, alignment_min_node_id) != 0) {
            component_t_free (comp);
            break;
        }

        printf ("found component length %d\n", g_queue_get_length (comp->pt));

        if (g_queue_get_length (comp->pt) < min_seq_length) { 
            printf ("component too short. skipping.\n"); 
            component_t_free (comp); 
            continue;
        }

        if (!component_t_valid (comp, n, alignment_min_diag_distance, alignment_max_slope_error)) { 
            printf ("invalid component. skipping.\n");
            component_t_free (comp); 
            continue;
        }

        // add component to queue
        g_queue_push_tail (components, comp);
    }

    // sort components by decreasing score
    g_queue_sort (components, component_t_cmp, NULL);
    g_queue_reverse (components);

    dbg (DBG_CLASS, "found %d components.", g_queue_get_length (components));

    free (mask1);
    free (mask2);
}

int* corrmat_components_to_matrix (GQueue *components, int n)
{
    // save to file
    int *al = corrmat_int_init (n, .0);

    for (GList *iter=g_queue_peek_head_link (components);iter;iter=iter->next) {
        component_t *c = (component_t*)iter->data;
        for (GList *piter=g_queue_peek_head_link(c->pt);piter;piter=piter->next) {
            pair_int_t *p = (pair_int_t*)piter->data;
            CORRMAT_SET (al, n, p->key, p->val, 1);
        }
    }
}

void corrmat_cleanup_component (component_t *c)
{
    gboolean done = FALSE;
    int prev_kv;
    pair_int_t *p;
    GList *iter;
    int count=0;

    // all keys must match at most one val
    //
    while (!done) {
        done = TRUE;

        prev_kv=-1;
        for (iter=g_queue_peek_head_link (c->pt);iter;iter=g_list_next(iter)) {
            p = (pair_int_t*)iter->data;
            if (prev_kv != -1 && p->val == prev_kv) {
                done = FALSE;
                break;
            }
            prev_kv = p->val;
        }
        if (!done) {
            assert (iter);
            g_queue_remove (c->pt, p);
            count++;
        }
    }
    dbg (DBG_CLASS, "removed %d elements from component.", count);

    done = FALSE;

    // all vals must match at most one key
    //
    while (!done) {
        done = TRUE;

        prev_kv=-1;
        for (iter=g_queue_peek_head_link (c->pt);iter;iter=g_list_next(iter)) {
            p = (pair_int_t*)iter->data;
            if (prev_kv != -1 && p->key == prev_kv) {
                done = FALSE;
                break;
            }
            prev_kv = p->key;
        }
        if (!done) {
            assert (iter);
            g_queue_remove (c->pt, p);
            count++;
        }
    }
    dbg (DBG_CLASS, "removed %d elements from component.", count);

}

void corrmat_cleanup_component_list (GQueue *components)
{
    for (GList *iter=g_queue_peek_head_link(components);iter;iter=g_list_next(iter)) {
        component_t *c = (component_t*)iter->data;
        component_t_print (c);
        corrmat_cleanup_component (c);
        component_t_print (c);
    }
}

navlcm_dictionary_t * corrmat_to_dictionary (double *mat, int size)
{
    int maxstrlen=10;
    navlcm_dictionary_t* dict = (navlcm_dictionary_t*)malloc(sizeof(navlcm_dictionary_t));
    dict->num = size*size;
    dict->keys = (char**)malloc(size*size*sizeof(char*));
    dict->vals = (char**)malloc(size*size*sizeof(char*));

    for (int i=0;i<size;i++) {
        for (int j=0;j<size;j++) {
            char c[10];
            sprintf (c, "%.5f", CORRMAT_GET (mat, size, i, j));
            dict->keys[i*size+j] = strdup (c);
            dict->vals[i*size+j] = strdup (c);
        }
    }

    return dict;
}

double *dictionary_to_corrmat (const navlcm_dictionary_t *dict, int *size)
{
    *size = math_round (1.0 * sqrt (dict->num * 1.0));
    assert (fabs (1.0**size**size-1.0*dict->num) < 1E-3);

    double *mat = (double*)calloc(dict->num, sizeof(double));

    for (int i=0;i<*size;i++) {
        for (int j=0;j<*size;j++) {
            double val;
            sscanf (dict->keys[i**size+j], "%lf", &val);
            CORRMAT_SET (mat, *size, i, j, val);
        }
    }
    return mat;
}

