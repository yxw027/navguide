#include "subdiv.h"

/* The code in this file is deprecated.
 *
 * The class_data structure contains the match matrix data. The structure allows for camera subdivision.
 * This data structure is deprecated and is now replaced by a double* matrix (see configuration file).
 */
double class_get_data (navlcm_class_data_t *data, int s, int id1, int id2) {
    int nel = data->nbuckets*data->nbuckets;
    assert (id1 >= 0 && id1 < nel);
    assert (id2 >= 0 && id2 < nel);

    return data->a[s][id1*nel+id2];
}
void class_set_data (navlcm_class_data_t *data, int s, int id1, int id2, double val) {
    int nel = data->nbuckets*data->nbuckets;
    data->a[s][id1*nel+id2] = val;
}

void class_inc_data (navlcm_class_data_t *data, int s, int id1, int id2, double val) {
    int nel = data->nbuckets*data->nbuckets;
    data->a[s][id1*nel+id2] += val;
}
int class_get_data_n (navlcm_class_data_t *data, int s, int id1, int id2) {
    int nel = data->nbuckets*data->nbuckets;
    return data->n[s][id1*nel+id2];
}
void class_set_data_n (navlcm_class_data_t *data, int s, int id1, int id2, int val) {
    int nel = data->nbuckets*data->nbuckets;
    data->n[s][id1*nel+id2] = val;
}

void class_inc_data_n (navlcm_class_data_t *data, int s, int id1, int id2, int val) {
    int nel = data->nbuckets*data->nbuckets;
    data->n[s][id1*nel+id2] += val;
}

/* the cell_t data structure represents a cell in the match matrix.
 * the structure exists only for debugging/statistical analysis purposes.
 */
cell_t*** classifier_reset (navlcm_class_data_t *data, cell_t ***cells)
{
    if (!data)
        return cells;

    dbg (DBG_CLASS, "[class] reset classifier");

    int size = data->nbuckets * data->nbuckets;

    for (int i=0;i<data->ntables;i++) {
        data->maxn[i] = 0;
        data->maxa[i] = 0.0;
        for (int j=0;j<size*size;j++) {
            data->n[i][j] = 0;
            data->a[i][j] = 0.0;
            data->cos[i][j] = 0.0;
            data->sin[i][j] = 0.0;
        }
    }
    if (!cells)
        return NULL;
    
    // destroy the cells
    cell_t_destroy (data, cells);

    // init the cells
    return cell_t_init (data);

}

void classifier_destroy (navlcm_class_data_t *data)
{
    if (!data)
        return;

    free (data->maxn);
    free (data->maxa);
    free (data->n);
    free (data->a);
    free (data->cos);
    free (data->sin);

    for (int i=0;i<data->nsensors;i++)
        free (data->tabindx[i]);
    free (data->tabindx);
    free (data->tab2sensor);
}

navlcm_class_data_t *classifier_init_tables () 
{
    navlcm_class_data_t *data = (navlcm_class_data_t *)malloc(sizeof(navlcm_class_data_t));

    data->maxn = NULL;
    data->maxa = NULL;
    data->n = NULL;
    data->a = NULL;
    data->cos = NULL;
    data->sin = NULL;
    data->tabindx = NULL;
    data->tab2sensor = NULL;

    return data;
}

/* nbuckets is the number of subdivision of the cameras. 
 * nbuckets = 1 means no subdivision */
navlcm_class_data_t *classifier_init_tables (int nsensors, int nbuckets)
{
    navlcm_class_data_t *data = classifier_init_tables ();

    int ntables = nsensors * (nsensors+1)/2;
    int size = nbuckets * nbuckets;

    data->nsensors = nsensors;
    data->ntables = ntables;
    data->nbuckets = nbuckets;
    data->nel = size*size;

    data->maxn = (int*)malloc(ntables*sizeof(int));
    data->maxa = (double*)malloc(ntables*sizeof(double));

    data->n = (int**)malloc(ntables*sizeof(int*));
    data->a = (double**)malloc(ntables*sizeof(double*));
    data->cos = (double**)malloc(ntables*sizeof(double*));
    data->sin = (double**)malloc(ntables*sizeof(double*));

    for (int i=0;i<ntables;i++) {
        data->maxn[i] = 0;
        data->maxa[i] = 0.0;
        data->n[i] = (int*)malloc(size*size*sizeof(int));
        data->a[i] = (double*)malloc(size*size*sizeof(double));
        data->cos[i] = (double*)malloc(size*size*sizeof(double));
        data->sin[i] = (double*)malloc(size*size*sizeof(double));

        for (int j=0;j<size*size;j++) {
            data->n[i][j] = 0;
            data->a[i][j] = 0.0;
            data->cos[i][j] = 0.0;
            data->sin[i][j] = 0.0;
        }
    }

    data->tabindx = (int**)malloc(nsensors*sizeof(int*));
    for (int i=0;i<nsensors;i++)
        data->tabindx[i] = (int*)malloc(nsensors*sizeof(int));

    int count = 0;
    for (int i=0;i<nsensors;i++) {
        for (int j=0;j<nsensors;j++) {
            if ( i <= j) {
                data->tabindx[i][j] = count;
                count++;
            } else {
                data->tabindx[i][j] = data->tabindx[j][i];
            }
        }
    }

    data->nconv = 2*data->ntables;

    data->tab2sensor = (int*)malloc(data->nconv*sizeof(int));
    for (int i=0;i<data->ntables;i++) {
        int id1,id2;
        classifier_get_index_from_table (i, data->nsensors, &id1, &id2, data);
        data->tab2sensor[2*i] = id1;
        data->tab2sensor[2*i+1] = id2;
    }

    return data;
}

int classifier_get_index_from_table (int index, int nsensors, int *sid1, 
                                     int *sid2, navlcm_class_data_t *data)
{
    assert (data);

    for (int i=0;i<nsensors;i++) {
        for (int j=i;j<nsensors;j++) {
            if (data->tabindx[i][j] == index) {
                *sid1 = i;
                *sid2 = j;
                return 0;
            }
        }
    }

    *sid1 = *sid2 = 0;
    return -1;
}

int classifier_get_table_index (int sid1, int sid2, int *transpose,
                                navlcm_class_data_t *data)
{
    if (sid1 > sid2) {
        *transpose = 1;
        return classifier_get_table_index (sid2, sid1, transpose, data);
    }

    return data->tabindx[sid1][sid2];
}

int classifier_image_coord_to_bucket_id (float col, float row, int width, 
                                         int height, int nbuckets)
{
    int r = (int)(1.0*row / height * nbuckets);
    int c = (int)(1.0*col / width * nbuckets);
    
    return r * nbuckets + c;
}


void classifier_bucket_id_to_image_coord (int id, int width, int height, int nbuckets, double *col, double *row)
{
    int r = (int)(id / nbuckets);
    int c = id - r*nbuckets;

    *row = (1.0 * r + .5) * height / nbuckets;
    *col = (1.0 * c + .5) * width / nbuckets;
}

// 
void classifier_stat (navlcm_class_data_t *data, double *mean, double *stdev)
{    
    int size = data->nbuckets * data->nbuckets;
    *mean = 0.0;
    int n = 0;

    for (int i=0;i<data->ntables;i++) {
        for (int j=0;j<size*size;j++) {
            if (data->n[i][j] > 0) {
                *mean += data->a[i][j];
                n++;
            }
        }
    }

    *mean /= n;

    *stdev = 0.0;
    for (int i=0;i<data->ntables;i++) {
        for (int j=0;j<size*size;j++) {
            if (data->n[i][j] > 0) {
                *stdev += (*mean-data->a[i][j])*(*mean-data->a[i][j]);
            }
        }
    }

    *stdev = sqrt(*stdev/n);
}

/* clean up classifier
 */
void classifier_cleanup (navlcm_class_data_t *data, int minvotes)
{
    // less than <minvotes> is noise
    for (int i=0;i<data->ntables;i++) {
        for (int j=0;j<data->nel;j++) {
            if (data->n[i][j] < minvotes) {
                data->a[i][j] = 0.0;
                data->n[i][j] = 0;
            }
        }
    }
}

/* force diagonal values
 */
void classifier_force_diagonal (navlcm_class_data_t *data)
{
    if (!data)
        return;
    
    int size = data->nbuckets*data->nbuckets;

    for (int i=0;i<data->ntables;i++) {
        int sid1, sid2;
        classifier_get_index_from_table (i, data->nsensors, &sid1, &sid2, data);
        if (sid1 != sid2)
            continue;
        
        for (int c=0;c<size;c++) {
            data->n[i][c*size+c] = data->maxn[i];
            data->a[i][c*size+c] = 0.0;
        }
    }
}

/* fill in a diagonal matrix using dyn programming
 */
void dyn_fill_classifier (navlcm_class_data_t *data, int sensor1, int sensor2)
{
    // determine
    int transpose;
    int index = classifier_get_table_index (sensor1, sensor2, &transpose, data);
    int index2 = classifier_get_table_index (sensor2, sensor2, &transpose, data);
    
    int size = data->nbuckets*data->nbuckets;
    int count = 0;

    // for each element a_jl in the table, look for the optimal k so that a_jl = a_jk + a_kl
    // optimal = maximize the min of number of votes
    for (int j=0;j<size;j++) {
        for (int l=0;l<size;l++) {

            // don't replace if the value looks good
            if (class_get_data_n (data, index, j, l) >= CLASS_MIN_VOTES)
                continue;

            // otherwise, look for optimal k != l
            int best_k = -1;
            int min_vote = -1;
            for (int k=0;k<size;k++) {
                
                if (sensor1 == sensor2 && k==l)
                    continue;
                
                int mvotes = MIN(data->n[index][j*size+k], data->n[index2][k*size+l]);
                
                if (mvotes < CLASS_MIN_VOTES)
                    continue;

                if (best_k == -1 || min_vote < mvotes) {
                    best_k = k;
                    min_vote = mvotes;
                }
            }

            // skip if no good k was found
            if (best_k == -1)
                continue;
            
            // otherwise, update the table
            class_set_data_n(data, index, j, l, min_vote);

            if (data->trans_mode)
                class_set_data (data,index, j, l, class_get_data (data,index, j, best_k) + 
                                class_get_data (data,index2, best_k, l));
            else
                class_set_data (data,index, j, l, clip_value (class_get_data (data,index, j, best_k) + 
                                                              class_get_data (data, index2, best_k, l), -PI, PI, 1E-6));
            count++;

        }
    }
                
    dbg (DBG_CLASS, "[class] filled table (%d,%d) with %d values (%.1f %%)",
         sensor1, sensor2, count, 100.0*count/(size*size));
}

/* fill in classifier using recursive method.
 */
void dyn_fill_classifier (navlcm_class_data_t *data)
{
    // first fill in diagonal matrices
    for (int i=0;i<data->nsensors;i++)
        dyn_fill_classifier (data, i, i);

    // then fill in other matrices
    for (int i=0;i<data->nsensors;i++)
        for (int j=0;j<i;j++)
            dyn_fill_classifier (data, i, j);
}


/*set the value for a cell in the table
 */
void set_table (navlcm_class_data_t *data, int index, int id1, int id2,
                double value, int nval)
{
    // set cell entry
    class_set_data (data, index, id1, id2, value);
    
    class_set_data_n (data, index, id1, id2, nval);
    
    // set maximum values for the table
    if (data->maxn[index] < nval)
        data->maxn[index] = nval;
    
    if (data->maxa[index] < fabs (value))
        data->maxa[index] = fabs (value);
}

/* update a table with an entry
 */
void update_table (navlcm_class_data_t *data, int index, int id1, int id2, 
                   double value, int mode)
{
    int nel = data->nbuckets*data->nbuckets;
    int current_n = class_get_data_n(data, index, id1, id2);
    class_set_data_n(data, index, id1, id2, current_n + 1);
    
    if (mode == CLASS_ROT_MODE) {
        data->cos[index][id1*nel+id2] += cos(value);
        data->sin[index][id1*nel+id2] += sin(value);
        
        class_set_data (data, index, id1, id2, atan2(
                    data->sin[index][id1*nel+id2],
                    data->cos[index][id1*nel+id2]));
    } else {
        
        double new_val = 1.0 * current_n * class_get_data (
               data, index, id1, id2) / (current_n+1) +
               1.0 * value / (current_n+1);
        
        class_set_data (data, index, id1, id2, new_val);
    }
    
    if (data->maxn[index] < current_n+1)
        data->maxn[index] = current_n+1;
    
    if (data->maxa[index] < fabs (class_get_data (data, index, id1, id2)))
        data->maxa[index] = fabs (class_get_data (data, index, id1, id2));
}


/* add a set of matches to the classifier. mode is 0 (rotation) or 1 (translation)
 */    
cell_t*** classifier_add_matches (navlcm_class_data_t *data, int width, 
                                  int height, cell_t ***cells,
                                  navlcm_feature_match_set_t *ma, 
                                  navlcm_dead_reckon_t dr, int mode)
{
    for (int i=0;i<ma->num;i++) {
        if (ma->el[i].num == 0)
            continue;

        int sid1 = ma->el[i].src.sensorid;
        int sid2 = ma->el[i].dst[0].sensorid;
        double col1 = ma->el[i].src.col;
        double col2 = ma->el[i].dst[0].col;
        double row1 = ma->el[i].src.row;
        double row2 = ma->el[i].dst[0].row;

        // convert image position into bucket id
        int id1 = classifier_image_coord_to_bucket_id (col1, row1, 
                                                       width, height, 
                                                       data->nbuckets);

        int id2 = classifier_image_coord_to_bucket_id (col2, row2, 
                                                       width, height, 
                                                       data->nbuckets);

        // increment the corresponding bucket
        int transpose = 0;
        int index = classifier_get_table_index (sid1, sid2, &transpose, data);
        if (index < 0 || index >= data->ntables) {
            dbg (DBG_CLASS, "failed to fetch table for ids %d and %d", sid1, sid2);
            continue;
        }

        // swap
        double value = clip_value (dr.angle, -PI, PI, 1E-6);

        if (mode == CLASS_TRA_MODE)
            value = dr.trans;

        if (transpose) {
            int tmp = id2;
            id2 = id1;
            id1 = tmp;
            value *= -1.0;
        }

        // save data in cells
        //
        cells = cell_t_insert (cells, index, id1, id2, value);
        //if (sid1 == sid2)
        //    cells = cell_t_insert (cells, index, id2, id1, value);

        // update table
        //
        //update_table (data, index, id1, id2, value, mode);
        //if (sid1 == sid2) 
        //    update_table (data, index, id2, id1, -value, mode);

    }

    return cells;

}

/* read the training data written in memory using classifier_add_matches
 * filter the cells and populates the tables
 */
void classifier_update_tables (navlcm_class_data_t *data, 
                               cell_t ***cells, int mode)
{
    // reset temp dir
#ifdef CLASS_DEBUG
    dbg (DBG_CLASS, "[class] re-creating directory %s...", CLASS_TMP_DIR);
    remove_dir (CLASS_TMP_DIR);
    create_dir (CLASS_TMP_DIR);
#endif

    // perform filtering on each cell
    //
    dbg (DBG_CLASS, "[class] filtering cells...");

    int size = data->nbuckets * data->nbuckets;
    FILE *fp;

    // threshold standard deviation for rotation classifier
    // is angular resolution of a bucket for a FOV of 120 deg.
    double ang_std_thresh = 120.0 * PI / (180.0 * data->nbuckets);
    
    dbg (DBG_CLASS, "[class] threshold std = %.1f deg.",
         ang_std_thresh * 180.0/PI);

    for (int n=0;n<data->ntables;n++) {
        
#ifdef CLASS_DEBUG
        dbg (DBG_CLASS, "[class] table %d out %d", n+1, data->ntables);
#endif
        
        // nmax is the top number of votes/cell for the current table
        int nmax = 0;

        for (int i=0;i<size;i++) {
            for (int j=0;j<size;j++) {
                
                // set to zero
                set_table (data, n, i, j, 0.0, 0);

                double *vals = cells[n][i][j].vals;
                int nval = cells[n][i][j].n;

                // skip if nothing in this cell
                if (!vals) {
                    continue;
                }

                // write out cell (debugging)
#ifdef CLASS_DEBUG
                char fname[256];
                sprintf (fname, "%s/%02d-%02d-%02d.dat", 
                         CLASS_TMP_DIR, n, i, j);
                fp = fopen (fname, "w");
                if (fp) {
                    vect_write (fp, vals, nval);
                    fclose (fp);
                } else {
                    dbg (DBG_ERROR, "[class] failed to open %s", fname);
                }
#endif
                
                // histogram data
                double minv, maxv;
                int *hist = histogram_build (vals, nval, HIST_SIZE,
                                             &minv, &maxv);
                if (!hist) {
                    continue;
                }

                // find peak in the histogram
                int hist_max;
                histogram_peak (hist, HIST_SIZE, &hist_max);

                double *new_vals = NULL;
                int nnew_val=0;

                // filter values supported by less than 10% of hist_max
                //
                for (int r=0;r<nval;r++) {
                    // compute bucket index in histogram
                    int indx = math_round((vals[r]-minv) * 
                                          HIST_SIZE / (maxv-minv));
                    indx = MIN (HIST_SIZE-1, MAX (0, indx));
                    if (1.0*hist[indx] > .10 * hist_max) {
                        new_vals = (double*)realloc(new_vals,
                                    (nnew_val+1)*sizeof(double));
                        new_vals[nnew_val] = vals[r];
                        nnew_val++;
                    }
                }

                // replace old values
                free (vals);
                cells[n][i][j].vals = new_vals;
                cells[n][i][j].n = nnew_val;
                vals = new_vals;
                nval = nnew_val;

                if (nval == 0) {
                    free (hist);
                    continue;
                }

                // compute vector mean. If translation mode, this is the
                // usual mean; if rotation mode, this is the cos-sin mean.
                double mean = 0.0;
                double stdev = 0.0;

                if (mode == CLASS_TRA_MODE) {
                    mean = vect_mean_double (vals, nval);
                    stdev = vect_stdev (vals, nval, mean);
                } else if (mode == CLASS_ROT_MODE) {
                    mean = vect_mean_sincos (vals, nval);
                    stdev = vect_stdev_sincos (vals, nval, mean);
                }

#ifdef CLASS_DEBUG

                // write out the updated cell (debugging)
                char oname[256];
                sprintf (oname, "%s.filter", fname);
                fp = fopen (oname, "w");
                if (fp) {
                    vect_write (fp, vals, nval);
                    fclose (fp);
                }

                // write out the stdev (debugging)
                char vname[256];
                sprintf (vname, "%s.std", fname);
                fp = fopen (vname, "w");
                fprintf (fp, "%.4f %.4f %d\n", mean, stdev, nval);
                fclose (fp);
#endif

                // free data
                free (hist);

                // test standard deviation
                //
                gboolean data_ok = FALSE;

                if (mode == CLASS_ROT_MODE) {
                    data_ok = stdev < ang_std_thresh;
                }

                if (!data_ok)
                    continue;

                // set value in table
                set_table (data, n, i, j, mean, nval);
                
                // update nmax
                nmax = MAX (nmax, nval);
            }
        }

        // filter out cells that have less than 2% of <nmax> votes
        int count=0;
        for (int i=0;i<size;i++) {
            for (int j=0;j<size;j++) {
                
                if (1.0 * class_get_data_n (data, n, i, j) < .02 * nmax) {
                    
                    set_table (data, n, i, j, 0.0, 0);
                    count++;
                } else {
                    char tname[256];
                    sprintf (tname, "%s/%02d-%02d-%02d.dat.std",
                             CLASS_TMP_DIR, n, i, j);
                    fp = fopen (tname, "a");
                    fprintf (fp, "saved\n");
                    fclose (fp);
                }
            }
        }

        dbg (DBG_CLASS, "[class] nmax = %d. threshold: %.2f "
             "filtered %d cells.", nmax, .02 * nmax, count);
    }
}

/* save a classifier to a binary file
 * the tables are stored in <filename>
 * the cells are stored in <filename>.cells
 */
void classifier_save_to_file (const char *filename, 
                              cell_t ***cells,
                              navlcm_class_data_t *data)
{
    size_t size=0;
    FILE *fp = fopen (filename, "wb");
    if (!fp) {
        dbg (DBG_ERROR, "[class] failed to open %s in wb mode.", filename);
        return;
    }

    // header
    size = fwrite (&data->nsensors, sizeof(int32_t), 1, fp);
    size = fwrite (&data->ntables, sizeof(int32_t), 1, fp);
    size = fwrite (&data->nbuckets, sizeof(int32_t), 1, fp);
    size = fwrite (&data->nel, sizeof(int32_t), 1, fp);
    
    // tables data
    for (int i=0;i<data->ntables;i++) {
        for (int j=0;j<data->nel;j++) {
            size = fwrite (&data->n[i][j], sizeof(int32_t), 1, fp);
        }
    }
    for (int i=0;i<data->ntables;i++) {
        for (int j=0;j<data->nel;j++) {
            size = fwrite (&data->a[i][j], sizeof(double), 1, fp);
        }
    }
    for (int i=0;i<data->ntables;i++) {
        for (int j=0;j<data->nel;j++) {
            size = fwrite (&data->cos[i][j], sizeof(double), 1, fp);
        }
    }
    for (int i=0;i<data->ntables;i++) {
        for (int j=0;j<data->nel;j++) {
            size = fwrite (&data->sin[i][j], sizeof(double), 1, fp);
        }
    }
    for (int i=0;i<data->ntables;i++) {
        size = fwrite (&data->maxn[i], sizeof(int32_t), 1, fp);
    }
    for (int i=0;i<data->ntables;i++) {
        size = fwrite (&data->maxa[i], sizeof(double), 1, fp);
    }
    fclose (fp);

    // cells data
    if (!cells) {
        dbg (DBG_ERROR, "[class] no cell data to write to file %s", filename);
        return;
    }

    char fname[256];
    sprintf (fname, "%s.cells", filename);
   
    fp = fopen (fname, "wb");

    dbg (DBG_CLASS, "[class] writing cell data to file %s", fname);

    int sz = data->nbuckets * data->nbuckets;
    
    for (int n=0;n<data->ntables;n++) {
        for (int i=0;i<sz;i++) {
            for (int j=0;j<sz;j++) {
                
                // number of values in the cell
                int num = cells[n][i][j].n;

                size = fwrite (&num, sizeof(int), 1, fp);
                
                // cell values
                size = fwrite (cells[n][i][j].vals, sizeof(double), num, fp);
            }
        }
    }

    fclose (fp);
    
}

void classifier_save_to_ascii_file (navlcm_class_data_t *data, 
                                    const char *basename)
{
    // sanity check
    if (!data) {
        dbg (DBG_ERROR, "[class] no data to save.");
        return;
    }

    // check that directory exists
    if (!dir_exists (basename)) {
        dbg (DBG_ERROR, "[class] directory %s does not exist.", basename);
        return;
    }

    dbg (DBG_CLASS, "[class] saving classifier to ascii file in %s", basename);

    int size = data->nbuckets*data->nbuckets;

    for (int i=0;i<data->ntables;i++) {
            char name[1024];
            if (data->trans_mode)
                sprintf (name, "%s/class-trans-%02d.dat", basename, i);
            else
                sprintf (name, "%s/class-rot-%02d.dat", basename, i);
            
            FILE *fp = fopen (name, "w");

            for (int r=0;r<size;r++) {
                for (int c=0;c<size;c++) {
                    fprintf (fp, "%.5f ", data->a[i][r*size+c]);
                }
                fprintf (fp, "\n");
            }

            fclose (fp);
    }
    for (int i=0;i<data->ntables;i++) {
            char name[1024];
            if (data->trans_mode)
                sprintf (name, "%s/class-trans-n-%02d.dat", basename, i);
            else
                sprintf (name, "%s/class-rot-n-%02d.dat", basename, i);
            
            FILE *fp = fopen (name, "w");

            for (int r=0;r<size;r++) {
                for (int c=0;c<size;c++) {
                    fprintf (fp, "%d ", data->n[i][r*size+c]);
                    
                }
                fprintf (fp, "\n");
            }

            fclose (fp);
    }
}

/* load classifier tables from a binary file
 * <filename> contains the table data
 * <filename>.cells contains the cell data
 * Warning: this methods assumes that <cells> have been initialized
 */
navlcm_class_data_t *classifier_load_from_file (const char *filename,
                                                  cell_t ***cells)
{
    size_t size = 0;
    FILE *fp = fopen (filename, "rb");
    if (!fp) {
        dbg (DBG_ERROR, "[class] failed to load file %s", filename);
        return NULL;
    }

    int nsensors, ntables, nbuckets, nel;

    // header
    size = fread (&nsensors, sizeof(int32_t), 1, fp);
    size = fread (&ntables, sizeof(int32_t), 1, fp);
    size = fread (&nbuckets, sizeof(int32_t), 1, fp);
    size = fread (&nel, sizeof(int32_t), 1, fp);

    // table data
    navlcm_class_data_t *data = classifier_init_tables (nsensors, nbuckets);
    
    for (int i=0;i<data->ntables;i++) {
        for (int j=0;j<data->nel;j++) {
            size = fread (&data->n[i][j], sizeof(int32_t), 1, fp);
        }
    }
    for (int i=0;i<data->ntables;i++) {
        for (int j=0;j<data->nel;j++) {
            size = fread (&data->a[i][j], sizeof(double), 1, fp);
        }
    }
    for (int i=0;i<data->ntables;i++) {
        for (int j=0;j<data->nel;j++) {
            size = fread (&data->cos[i][j], sizeof(double), 1, fp);
        }
    }
    for (int i=0;i<data->ntables;i++) {
        for (int j=0;j<data->nel;j++) {
            size = fread (&data->sin[i][j], sizeof(double), 1, fp);
        }
    }
    for (int i=0;i<data->ntables;i++) {
        size = fread (&data->maxn[i], sizeof(int32_t), 1, fp);
    }
    for (int i=0;i<data->ntables;i++) {
        size = fread (&data->maxa[i], sizeof(double), 1, fp);
    }
    fclose (fp);

    dbg (DBG_CLASS, "[class] loaded file %s", filename);

    // cell data
    char fname[256];
    sprintf (fname, "%s.cells", filename);
    fp = fopen (fname, "rb");
    if (!fp) {
        dbg (DBG_ERROR, "[class] failed to open file %s in rb mode.", fname);
        return data;
    }

    int sz = data->nbuckets * data->nbuckets;

    dbg (DBG_CLASS, "[class] reading cell data from %s", fname);

    for (int n=0;n<data->ntables;n++) {
        
        for (int i=0;i<sz;i++) {
            for (int j=0;j<sz;j++) {

                unsigned int num;
                size = fread (&num, sizeof(int), 1, fp);
                
                double *vals = (double*)malloc(num*sizeof(double));
                assert (fread (vals, sizeof(double), num, fp) == num);
                
                assert (vals);

                fflush(stdout);
                cells[n][i][j].n    = num;
                cells[n][i][j].vals = vals;
            }
        }
    }

    fclose (fp);

    return data;
}

/* classifier query: given two image positions, look up the 
 * classifier value and return TRUE if a value was found
 */
gboolean classifier_query (navlcm_class_data_t *data,
                           int width, int height, int sid1, int sid2,
                           double col1, double col2,
                           double row1, double row2,
                           double *val, int *nval)
{
    assert (val && nval);
    assert (width > 0 && height > 0);

    // convert image position into bucket id
    int id1 = classifier_image_coord_to_bucket_id (col1, row1, 
                                     width, height, data->nbuckets);
    int id2 = classifier_image_coord_to_bucket_id (col2, row2,
                                     width, height, data->nbuckets);

    
    // search the corresponding bucket
    int transpose = 0;
    int index = classifier_get_table_index (sid1, sid2, &transpose, data);
    if (index < 0 || index >= data->ntables) {
        dbg (DBG_ERROR, "failed to fetch table for ids %d and %d", 
             sid1, sid2);
        return FALSE;
    }

    // swap
    if (transpose) {
        int tmp = id2;
        id2 = id1;
        id1 = tmp;
    }
    
    *val =  class_get_data (data, index, id1, id2);
    *nval = class_get_data_n (data, index, id1, id2);
        
    if (transpose) { *val = - *val; }
    
    // skip if classifier query failed
    //
    if (*nval == 0)
        return FALSE;
    
    return TRUE;
}

/* classifier query: given a set of feature matches <ma> and a classifier <data>
 * return a list of probable dead_reckon values
 */
navlcm_dead_reckon_t *classifier_query (navlcm_class_data_t *data, 
                                          int width, int height,
                                          navlcm_feature_match_set_t *ma, 
                                          int *n, int mode, int count)
{
    // number of successful queries
    *n = 0;
    
    assert (data);

    // sanity check
    if (width == 0 || height == 0) {
        dbg (DBG_ERROR, "[class] running classifier query with width = %d, height = %d", width, height);
        return NULL;
    }

    navlcm_dead_reckon_t *out = NULL;
    int mark = 0;

    // for each feature match, query the classifier and add a vote in the list
    //
    for (int i=0;i<ma->num;i++) {
        if (ma->el[i].num == 0) continue;

        int sid1 = ma->el[i].src.sensorid;
        int sid2 = ma->el[i].dst[0].sensorid;
        double col1 = ma->el[i].src.col;
        double col2 = ma->el[i].dst[0].col;
        double row1 = ma->el[i].src.row;
        double row2 = ma->el[i].dst[0].row;

        double val=0; int nval=0;
        
        gboolean ok = classifier_query (data, width, height,
                                        sid1, sid2, col1, col2, 
                                        row1, row2, &val, &nval);

        if (!ok)
            continue;

        // update output list
        //
        navlcm_dead_reckon_t dr;
        dr.angle = dr.trans = 0.0;
        dr.angle_error = dr.trans_error = 0.0;
        dr.sensor = sid1;
        dr.type = mode;
        dr.weight = nval;
        dr.utime = 0;
        
        if (mode == CLASS_ROT_MODE)
            dr.angle = val;
        else
            dr.trans = val;

        out = (navlcm_dead_reckon_t *)realloc (out, 
               (mark+1)*sizeof(navlcm_dead_reckon_t));
        out[mark] = dr;
        mark++;
    }

    *n = mark;

    return out;
}

/* normalize the distribution of dead reckons over cameras
 */
navlcm_dead_reckon_t *classifier_normalize_dead_reckon_dist (navlcm_dead_reckon_t *drs, int n, 
                                                               int *new_n, int nsensors)
{
    // initialize counters
    int count[nsensors];
    for (int i=0;i<nsensors;i++)
        count[i] = 0;

    // initialize pointers
    int *pointers[nsensors];
    for (int i=0;i<nsensors;i++) {
        pointers[i] = NULL;
    }
    
    int maxcount = 0;

    // fill in pointers and counters
    for (int i=0;i<n;i++) {
        int sensor = drs[i].sensor;
        count[sensor]++;
        pointers[sensor] = (int*)realloc(pointers[sensor], count[sensor]*sizeof(int));
        pointers[sensor][count[sensor]-1] = i;
        maxcount = MAX (maxcount, count[sensor]);
    }

    *new_n = n;

    // add new elements randomly
    for (int i=0;i<nsensors;i++) {
        for (int j=0;j<maxcount-count[i];j++) {
            int idx = MAX(0, MIN(count[i]-1, (int)((1.0*rand()/RAND_MAX*count[i]))));
            int pdx = pointers[i][idx];
            drs = (navlcm_dead_reckon_t *)realloc (drs, (*new_n+1)*sizeof(navlcm_dead_reckon_t));
           // printf("drs[%d] <- drs[%d]  (max %d)\n", *new_n, pdx, n); fflush (stdout);
            drs[*new_n] = drs[pdx];
            *new_n = *new_n + 1;
        }
    }
                                                     
    // free
    for (int i=0;i<nsensors;i++)
        free (pointers[i]);

    return drs;
}

/* Rotation guidance using RANSAC 
 * We do <nruns> runs. At each run, we pick <ninliers> candidates
 * we compute the mean value, then the error and return the best run
 */
double classifier_ransac (double *data, int n, int mode, int nruns, int ninliers, double *error)
{
    *error = 1E6;

    if (n == 0) {
        dbg (DBG_ERROR, "[class] not enough data to run RANSAC (n=0)");
        return 0.0;
    }

    dbg (DBG_CLASS, "[class] running RANSAC on %d/%d set with %d runs", ninliers, n, nruns);

    double best_val = 0.0;
    int best_run = -1;

    for (int run=0; run < nruns; run++) {

        // pick a set of indices randomly
        int *indices = math_random_indices (ninliers, 0, n-1);

        if (!indices) {
            dbg (DBG_ERROR, "[class] not enough data to run RANSAC.");
            return data[0];
        }

        // compute mean value
        double mean = 0.0;

        if (mode == CLASS_TRA_MODE) {
            for (int i=0;i<ninliers;i++) {
                mean += data[indices[i]];
            }
            mean /= ninliers;
        } else {
            double ssin = 0.0, scos = 0.0;
            for (int i=0;i<ninliers;i++) {
                ssin += sin (data[indices[i]]);
                scos += cos (data[indices[i]]);
            }
            mean = atan2 (ssin, scos);
        }

        // compute error over all other voters
        double err = 0.0;
        int count = 0;
        if (mode == CLASS_TRA_MODE) {
            for (int i=0;i<n;i++) {
                if (math_find_index (indices, n, i) >= 0)
                    continue;
                err += (data[i] - mean)*(data[i] - mean);
                count++;
            }
            err = sqrt (err) / count;
        } else {
            for (int i=0;i<n;i++) {
                if (math_find_index (indices, n, i) >= 0)
                    continue;
                double delta = diff_angle_plus_minus_pi (data[i], mean);
                err += delta * delta;
                count++;
            }
            err = sqrt (err) / count;
        }

        // keep value if better
        if (err < *error) {
            *error = err;
            best_val = mean;
            best_run = run;
        }

        free (indices);
    }
        
    dbg (DBG_CLASS, "[class] RANSAC found best val = %.3f at run %d/%d. error = %.3f", best_val, best_run, nruns, *error);

    return best_val;
}

/* extract a single dead_reckon value from a list of dead_reckon values
 */
navlcm_dead_reckon_t 
classifier_extract_single_dead_reckon (navlcm_dead_reckon_t *drs, int n, 
                                       int mark, double *error, int mode)
{
    navlcm_dead_reckon_t dr = navlcm_dead_reckon_t_create ();
    dr.type = mode;

    if (n == 0)
        return dr;

    GTimer *timer = g_timer_new ();

    // create a vector of values
    double *vals = (double*)malloc(n*sizeof(double));
    for (int i=0;i<n;i++) {
        vals[i] = mode == CLASS_ROT_MODE ? drs[i].angle : drs[i].trans;
    }

    // compute the mean and stdev
    //
    if (mode == CLASS_ROT_MODE) {
        dr.angle       = vect_mean_sincos (vals, n);
        dr.angle_error = vect_stdev_sincos (vals, n, dr.angle);
        *error         = dr.angle_error;
    } else if (mode == CLASS_TRA_MODE) {
        dr.trans       = vect_mean_double (vals, n);
        dr.trans_error = vect_stdev (vals, n, dr.trans);
        *error         = dr.trans_error;
    } 

    dbg (DBG_CLASS, "[class] n = %d, error = %.3f", n, *error);

    // compute elapsed time
    gulong usecs;
    dbg (DBG_CLASS, "[class] extract single dead reckon elapsed time: "
         "%.3f secs.", g_timer_elapsed (timer, &usecs));
    dbg (DBG_CLASS, "[class] extracted: angle = %.3f deg.", 
         dr.angle * 180 / PI);

    g_timer_destroy (timer);

    return dr;
}

/* given a set of multi-matches and a classifier,
 * filter out matches that correspond to a user rotation
 * greater than some threshold
 * if strict is set to true and the classifier returns no entry
 * then the match is discarded
 */
navlcm_feature_match_set_t *classifier_filter_matches (
                 navlcm_class_data_t *data, int width, int height, 
                 navlcm_feature_match_set_t *matches,
                 double angle_thresh_deg, gboolean strict)
{
    // sanity check
    if (!matches)  return NULL;
    
    GTimer *timer = g_timer_new();

    double angle_thresh = angle_thresh_deg * PI / 180.0;

    // match pointer
    navlcm_feature_match_t *match = matches->el;

    // new set of matches
    navlcm_feature_match_set_t *new_set = (navlcm_feature_match_set_t *)
        malloc(sizeof(navlcm_feature_match_set_t));

    new_set->num = 0;
    new_set->el = NULL;

    // loop through matches
    for (int n=0;n<matches->num;n++) {

        if (match->num == 0)  {match++; continue;}

        int sid1 = match->src.sensorid;
        double col1 = match->src.col;
        double row1 = match->src.row;

        navlcm_feature_match_t *new_match = 
            navlcm_feature_match_t_create (&match->src);

        // filter current match
        for (int k=0;k<match->num;k++) {

            int sid2 = match->dst[0].sensorid;
            double col2 = match->dst[0].col;
            double row2 = match->dst[0].row;

            double angle = 0.0; int nval = 0;
            gboolean ok = classifier_query (data, width, height,
                                            sid1, sid2, col1, col2, 
                                            row1, row2, &angle, &nval);
            if (!ok && strict)  continue;
            
            // keep match
            if (!ok || fabs (angle) < angle_thresh) {
                
                new_match = navlcm_feature_match_t_insert (new_match,
                                                             match->dst + k,
                                                             match->dist[k]);
                
            }
        }

        // add match to set
        navlcm_feature_match_set_t_insert (new_set, new_match);
        navlcm_feature_match_t_destroy (new_match);

        // move to next match
        match++;
    }

    gulong usecs;
    double secs = g_timer_elapsed (timer, &usecs);
    
    dbg (DBG_CLASS, "[class] filtered matches in %.3f secs."
         " %d matches --> %d matches.", secs, matches->num, new_set->num);

    g_timer_destroy (timer);

    // destroy old matches
    navlcm_feature_match_set_t_destroy (matches);
    
    // return new matches
    return new_set;
}

/* integrate a series of dead reckoning values
 */
double dead_reckon_list_to_odometry (navlcm_dead_reckon_t *drs, int ndrs, int nsensors)
{
    double mean_dist[nsensors];
    int rcount[nsensors];
    for (int i=0;i<nsensors;i++) {
        mean_dist[i] = 0.0;
        rcount[i] = 0;
    }
    
    for (int i=0;i<ndrs;i++) {
        assert (drs[i].sensor != -1);
        mean_dist[drs[i].sensor] += drs[i].trans;
        rcount[drs[i].sensor]++;
    }
    
    double mdist = 0.0;
    int mcount = 0;
    for (int i=0;i<nsensors;i++) {
        if (rcount[i]>0) {
            mean_dist[i] /= rcount[i];
            mcount++;
            mdist += mean_dist[i];
        }
    }
    
    if (mcount>0)
        mdist /= mcount;

    return mdist;
}

/* save odometry to file
 */
void save_odometry_to_file (odometry_t *odometry, int num_odometry, const char *filename)
{
    size_t size=0;

    FILE *fp = fopen (filename, "wb");
    if (!fp) {
        dbg (DBG_ERROR, "[class] failed to open %s in wb mode", filename);
        return;
    }

    size = fwrite (&num_odometry, sizeof(int), 1, fp);
    
    for (int i=0;i<num_odometry;i++) {
        size = fwrite(&odometry[i].dist, sizeof(double), 1, fp);
    }

    fclose (fp);

    dbg (DBG_CLASS, "[class] saved %d odometry to %s", num_odometry, filename);
}

/* read odometry from file
 */
odometry_t *read_odometry_from_file (int *num_odometry, const char *filename)
{
    size_t size=0;

    FILE *fp = fopen (filename, "rb");
    if (!fp) {
        dbg (DBG_ERROR, "[class] failed to open %s in rb mode", filename);
        return NULL;
    }

    size = fread (num_odometry, sizeof(int), 1, fp);
    
    odometry_t *odometry = (odometry_t*)malloc (*num_odometry * sizeof(odometry_t));

    for (int i=0;i<*num_odometry;i++) {
        odometry_t tr;
        size = fread(&tr.dist, sizeof(double), 1, fp);
        odometry[i] = tr;
    }

    fclose (fp);

    dbg (DBG_CLASS, "[class] read %d odometry to %s", *num_odometry, filename);

    return odometry;
}


cell_t*** cell_t_init (navlcm_class_data_t *data)
{
    cell_t ***cells = (cell_t***)malloc (data->ntables*sizeof(cell_t**));
    
    int size = data->nbuckets * data->nbuckets;

    for (int n=0;n<data->ntables;n++) {
        
        cells[n] = (cell_t**)malloc(size*sizeof(cell_t*));
        
        for (int j=0;j<size;j++) {
            cells[n][j] = (cell_t*)malloc(size*sizeof(cell_t));
            
            for (int i=0;i<size;i++) {
                fflush(stdout);
                cells[n][j][i].vals = NULL;
                cells[n][j][i].n = 0;
            }
        }
    }
    
    return cells;
}

void cell_t_destroy (navlcm_class_data_t *data, 
                     cell_t ***cells)
{
    if (!cells)
        return;

    int size = data->nbuckets * data->nbuckets;

    for (int n=0;n<data->ntables;n++) {
        
        for (int j=0;j<size;j++) {
            
            for (int i=0;i<size;i++) {
                
                free (cells[n][j][i].vals);
            }
            
            free (cells[n][j]);
        }

        free (cells[n]);
    }

    free (cells);
    
    cells = NULL;
}

cell_t*** cell_t_insert (cell_t ***cells, int n, int i, int j, double val)
{
    cells[n][i][j].vals = (double*)realloc(cells[n][i][j].vals,
                              (cells[n][i][j].n+1)*sizeof(double));

    cells[n][i][j].vals[cells[n][i][j].n] = val;
    
    cells[n][i][j].n++;
    
    return cells;
}

/* this set of methods represent the match matrix as a hash table
 */
void classifier_insert_prob_hash (GHashTable *hash, 
                    navlcm_feature_match_set_t *matches,
                    int width, int height, int nbuckets)
{
    if (!hash || !matches) return;

    GTimer *timer = g_timer_new ();
    
    // for each match, determine the key (quartet) and insert value in the table
    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *match = matches->el + i;
        if (match->num == 0) continue;

        navlcm_feature_t *src = &match->src;
        navlcm_feature_t *dst = match->dst;

        quartet_t key;
        key.a = src->sensorid;
        key.b = classifier_image_coord_to_bucket_id (src->col, src->row, width, height, nbuckets);
        key.c = dst->sensorid;
        key.d = classifier_image_coord_to_bucket_id (dst->col, dst->row, width, height, nbuckets);

        // query hash table
        gpointer data = g_hash_table_lookup (hash, &key);

        int num=1;
        if (data) { 
            num = GPOINTER_TO_INT(data)+1;
        }

        // insert in hash table
        g_hash_table_replace (hash, m_quartetdup (&key), GINT_TO_POINTER (num));
    }

    double secs = g_timer_elapsed (timer, NULL);

    dbg (DBG_CLASS, "hash table size: %d (%d insertion in %.3f secs.)", 
            g_hash_table_size (hash), matches->num, secs);
}

int classifier_sum_values_prob_hash (GHashTable *hash)
{
    if (!hash) return 0;

    // retrieve every values
    GList *values = g_hash_table_get_values (hash);

    // sum them up
    int total=0;
    GList *ptr;
    for (ptr=values;ptr!=NULL;ptr=g_list_next(ptr)){
        total+=GPOINTER_TO_INT(ptr);
    }

    g_list_free (values);

    return total;
}

int classifier_sum_values_prob_hash (GHashTable *hash, int sid, int index)
{
    int total=0;
    if (!hash) return 0;

    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init (&iter, hash);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
        quartet_t *q = (quartet_t*)key;
        guint val = GPOINTER_TO_INT (value);
        if ((int)q->a != (int)sid || (int)q->b != (int)index) continue;
        total+=val;
    }
    return total;
}


gboolean classifier_test_prob_hash (GHashTable *hash, 
                                double col1, double row1,
        int sid1, double col2, double row2, int sid2, int width, int height, 
        int nbuckets, int thresh)
{
    if (!hash) return FALSE;    

    quartet_t key;
    key.a = sid1;
    key.b = classifier_image_coord_to_bucket_id (col1, row1, width, height, nbuckets);
    key.c = sid2;
    key.d = classifier_image_coord_to_bucket_id (col2, row2, width, height, nbuckets);

    // hard-coded ok if same sensor, same cell
    if (key.a == key.c && key.b == key.d) return TRUE;

    // lookup the table
    gpointer data = g_hash_table_lookup (hash, &key);

    if (!data) {return FALSE;}

    int num = GPOINTER_TO_INT(data);

    if (num < thresh) {return FALSE;}

    return TRUE;
}

navlcm_feature_match_set_t* classifier_filter_prob_hash (GHashTable *hash, 
        navlcm_feature_match_set_t* matches, int width, int height, int nbuckets)
{
    if (!hash || !matches) return matches;

    // create a new set
    navlcm_feature_match_set_t *new_set = navlcm_feature_match_set_t_create ();

    // parse input set
    for (int i=0;i<matches->num;i++) {
        navlcm_feature_match_t *match = matches->el + i;
        if (match->num==0) continue;
        // create a new match
        navlcm_feature_match_t *copy = navlcm_feature_match_t_create 
            (&match->src);
        for (int k=0;k<match->num;k++) {
            if (!classifier_test_prob_hash (hash, match->src.col, match->src.row,
                match->src.sensorid, match->dst[k].col, match->dst[k].row, 
                match->dst[k].sensorid, width, height, nbuckets, 20)) // filter at 2%
            continue;
            copy = navlcm_feature_match_t_insert (copy, match->dst+k, match->dist[k]);
        }
        if (copy->num == 0) 
            navlcm_feature_match_t_destroy (copy);
        else {
            navlcm_feature_match_set_t_insert (new_set, copy);
            navlcm_feature_match_t_destroy (copy);
        }
    }
    
    dbg (DBG_CLASS, "filtering %d cells: %d kept.", matches->num, new_set->num);

    // destroy old matches
    navlcm_feature_match_set_t_destroy (matches);

    return new_set;
}

void classifier_save_to_file_prob_hash (GHashTable *hash, const char *filename)
{
    if (!hash) return;

    size_t size=0;
    FILE *fp = fopen (filename, "wb");
    if (!fp) {
        dbg (DBG_ERROR, "failed to open %s in wb mode.", filename);
        return;
    }

    GHashTableIter iter;
    gpointer key, value;

    int count=0;
    g_hash_table_iter_init (&iter, hash);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
        //printf ("count: %d\n",count); fflush(stdout);
        quartet_t *q = (quartet_t*)key;
        guint val = GPOINTER_TO_INT (value);
        //printf ("quartet: %d,%d,%d,%d  value: %d\n", q->a, q->b, q->c, q->d, val); fflush(stdout);
        size = fwrite (q, sizeof(quartet_t), 1, fp);
        size = fwrite (&val, sizeof(guint), 1, fp);
        count++;
    }

    dbg (DBG_CLASS, "saved hash table (%d elements) to %s.", g_hash_table_size(hash),
            filename);

    fclose (fp);
}

void classifier_read_from_file_prob_hash (GHashTable *hash, const char *filename)
{
    if (!hash) return;
    size_t size=0;

    // empty the hash
    g_hash_table_remove_all (hash);

    // read hash table from file
    FILE *fp = fopen (filename, "rb");
    if (!fp) { 
        dbg (DBG_ERROR, "failed to open %s in rb mode.", filename);
        return;
    }

    while (!feof (fp)) {
        quartet_t key; guint value;
        size = fread (&key, sizeof(quartet_t), 1, fp);
        size = fread (&value, sizeof(guint), 1, fp);
        g_hash_table_replace (hash, m_quartetdup (&key), GINT_TO_POINTER (value));
    }

    fclose (fp);

    dbg (DBG_CLASS, "read hash table with %d elements from %s", g_hash_table_size(hash),
            filename);

}

GHashTable *classifier_normalize_prob_hash (GHashTable *hash)
{
    if (!hash) return NULL;

    GTimer *timer = g_timer_new();

    dbg (DBG_CLASS, "normalizing prob hash table...");

    // output normalized hash table (will contain % values as integers)
    GHashTable *norm = g_hash_table_new_full (m_quartet_hash, m_quartet_equal, (GDestroyNotify)m_quartet_free, NULL);

    // for each key (sid1, index1, sid2, index2) compute the 
    // probability distribution of matches from (sid1,index1)
    // over all possible cells
    
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init (&iter, hash);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
        quartet_t *q = (quartet_t*)key;
        guint val = GPOINTER_TO_INT (value);

        // first, compute the sum of values
        int total=classifier_sum_values_prob_hash (hash, q->a, q->b);

        assert (total>0);

        // normalize
        val = math_round (1000.0 * val / total);

        // insert in the output table
        g_hash_table_insert (norm, m_quartetdup(q), GINT_TO_POINTER(val));
    }

    // destroy the input hash table
    g_hash_table_destroy (hash);

    gulong usecs;
    gdouble secs = g_timer_elapsed (timer, &usecs);
    g_timer_destroy (timer);
    dbg (DBG_CLASS, "normalization done in %.3f secs.", secs);

    return norm;
}

void classifier_print_prob_hash (GHashTable *hash)
{
    if (!hash) return;

    GHashTableIter iter;
    gpointer key, value;

    dbg (DBG_CLASS, "printing to /tmp/hash.txt ...");

    FILE *fp = fopen ("/tmp/hash.txt", "w");
    assert (fp);

    g_hash_table_iter_init (&iter, hash);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
        quartet_t *q = (quartet_t*)key;
        guint val = GPOINTER_TO_INT (value);
        if (val < 1) continue;
        fprintf(fp, "[%d,%d] -- %02d %% -- [%d,%d]\n", q->a, q->b, val, q->c, q->d);
                
    }

    fclose (fp);

    sort_file ("/tmp/hash.txt", -1);
}

