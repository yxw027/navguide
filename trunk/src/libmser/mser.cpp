/* file:        mser.mex.c
** description: Maximally Stable Extremal Regions
** author:      Andrea Vedaldi
**/

/* AUTORIGHTS
Copyright (C) 2006 Regents of the University of California
All rights reserved

Written by Andrea Vedaldi (UCLA VisionLab).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the University of California, Berkeley nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/** @file
 ** @brief Maximally Stable Extremal Regions - MEX implementation
 **/

#include "mser.h"

/* predicate used to sort pixels by increasing intensity */
int 
cmp_pair(void const* a, void const* b) 
{
  mser_pair_t* pa = (mser_pair_t*) a;
  mser_pair_t* pb = (mser_pair_t*) b;
  return pa->value - pb->value ;
}

/* advance N-dimensional subscript */
void
adv(int const* dims, int ndims, int* subs_pt)
{
  int d = 0 ;
  while(d < ndims) {
    if( ++subs_pt[d]  < dims[d] ) return ;
    subs_pt[d++] = 0 ;
  }
}

/*
navlcm_blob_t
mser_fit_ellipse (int *index, int n, int width, int marker)
{
    int *cols = (int*)malloc(n*sizeof(int));
    int *rows = (int*)malloc(n*sizeof(int));
    for (int i=0;i<n;i++) {
        cols[i] = index[i]%width;
        rows[i] = index[i]/width;
    }

    // compute centroid
    double cc = 0.0, rr = 0.0;
    for (int i=0;i<n;i++) {
        cc += cols[i];
        rr += rows[i];
    }
    
    cc /= n;
    rr /= n;
    
    // compute zero order moment
    double M00 = n;

    // compute first order moment
    double M11 = 0.0;
    for (int i=0;i<n;i++) 
        M11 += (1.0*cols[i]-cc)*(1.0*rows[i]-rr);

    // compute second order moments
    double M20 = 0.0, M02 = 0.0;
    for (int i=0;i<n;i++) {
        M20 += (1.0*cols[i]-cc)*(1.0*cols[i]-cc);
        M02 += (1.0*rows[i]-rr)*(1.0*rows[i]-rr);
    }

    // compute shape orientation
    double sy = 2*M11;
    double sx = M20 - M02;
    double orient = .5 * atan2 (sy, sx);
    
    // compute length in primary directions
    //
    double *xp = (double*)malloc(n*sizeof(double));
    double *yp = (double*)malloc(n*sizeof(double));
    for (int i=0;i<n;i++) {
        xp[i] =  (1.0*cols[i]-cc)*cos(orient) + (1.0*rows[i]-rr)*sin(orient);
        yp[i] = -(1.0*cols[i]-cc)*sin(orient) + (1.0*rows[i]-rr)*cos(orient);
    }
    
    double mean_x = 0.0, mean_y = 0.0;
    for (int i=0;i<n;i++) {
        mean_x += xp[i];
        mean_y += yp[i];
    }
    mean_x /= n;
    mean_y /= n;
    
    double stdev_x = 0.0, stdev_y = 0.0;
    for (int i=0;i<n;i++) {
        stdev_x += (xp[i]-mean_x)*(xp[i]-mean_x);
        stdev_y += (yp[i]-mean_y)*(yp[i]-mean_y);
    }
    stdev_x = sqrt (stdev_x / n);
    stdev_y = sqrt (stdev_y / n);
    
    // create new blob
    navlcm_blob_t blob;
    blob.col = cc;
    blob.row = rr;
    blob.orient = orient;
    blob.lenx = stdev_x;
    blob.leny = stdev_y;
    blob.marker = marker;
    blob.cols = cols;
    blob.rows = rows;
    blob.num = n;

    free (xp);
    free (yp);

    return blob;
}
*/

/* erfill */
int *
mser_fill (val_t *src, int width, int height, int er_idx, int *num)
{

  int i ;
  int k, nel, ndims ; 
  int dims[2] ;
  val_t const * I_pt ;
  int last = 0 ;
  int last_expanded = 0 ;
  val_t value = 0 ;
  int *out = NULL;
  *num = 0;

  int*   subs_pt ;       /* N-dimensional subscript                 */
  int*   nsubs_pt ;      /* diff-subscript to point to neigh.       */
  idx_t* strides_pt ;    /* strides to move in image array          */
  val_t* visited_pt ;    /* flag                                    */
  idx_t* members_pt ;    /* region members                          */

  /* get dimensions */
  nel   = width*height;
  ndims = 2;
  dims[0]  = width; dims[1] = height;
  I_pt  = src;

  /* allocate stuff */
  subs_pt    = (int*)malloc( sizeof(int)      * ndims ) ;
  nsubs_pt   = (int*)malloc( sizeof(int)      * ndims ) ;
  strides_pt = (idx_t*)malloc( sizeof(idx_t)    * ndims ) ;
  visited_pt = (val_t*)malloc( sizeof(val_t)    * nel   ) ;
  members_pt = (idx_t*)malloc( sizeof(idx_t)    * nel   ) ;

  /* compute strides to move into the N-dimensional image array */
  strides_pt [0] = 1 ;
  for(k = 1 ; k < ndims ; ++k) {
    strides_pt [k] = strides_pt [k-1] * dims [k-1] ;
  }
  
  /* load first pixel */
  memset(visited_pt, 0, sizeof(val_t) * nel) ;
  {
    int idx = er_idx;
    if( idx < 0 || idx > nel-1 ) {
      fprintf (stderr, "ER=%d out of range [1,%d]",idx,nel) ; 
    }
    members_pt [last++] = idx;//idx - 1 ;
  }

  value = I_pt[ members_pt[0] ]  ;

  /* -----------------------------------------------------------------
   *                                                       Fill region
   * -------------------------------------------------------------- */
  while(last_expanded < last) {
    
    /* pop next node xi */
    idx_t index = members_pt[last_expanded++] ;
    
    /* convert index into a subscript sub; also initialize nsubs 
       to (-1,-1,...,-1) */
    {
      idx_t temp = index ;
      for(k = ndims-1 ; k >=0 ; --k) {
        nsubs_pt [k] = -1 ;
        subs_pt  [k] = temp / strides_pt [k] ;
        temp         = temp % strides_pt [k] ;
      }
    }
    
    /* process neighbors of xi */
    while( true ) {
      int done_all_neighbors = 0;
      int good = true ;
      idx_t nindex = 0 ;
      
      /* compute NSUBS+SUB, the correspoinding neighbor index NINDEX
         and check that the pixel is within image boundaries. */
      for(k = 0 ; k < ndims && good ; ++k) {
        int temp = nsubs_pt [k] + subs_pt [k] ;
        good &= 0 <= temp && temp < dims[k] ;
        nindex += temp * strides_pt [k] ;
      }      
      
      /* process neighbor
         1 - the pixel is within image boundaries;
         2 - the pixel is indeed different from the current node
         (this happens when nsub=(0,0,...,0));
         3 - the pixel has value not greather than val
         is a pixel older than xi
         4 - the pixel has not been visited yet
      */
      if(good 
         && nindex != index 
         && I_pt [nindex] <= value
         && ! visited_pt [nindex] ) {
        
        /* mark as visited */
        visited_pt [nindex] = 1 ;
        
        /* add to list */
        members_pt [last++] = nindex ;
      }
      
      /* move to next neighbor */      
      k = 0 ;
      while(++ nsubs_pt [k] > 1) {
        nsubs_pt [k++] = -1 ;
        if(k == ndims) {done_all_neighbors=1; break;}
      }

      if (done_all_neighbors)
          break;
    } /* next neighbor */


  } /* goto pop next member */

  /*
   * Save results
   */
  *num = last;
  out = (int*)malloc(last*sizeof(int));
  for (i = 0 ; i < last ; ++i) {
    out[i] = members_pt[i];
  }
  
  /* free stuff */
  free( members_pt ) ;
  free( visited_pt ) ;
  free( strides_pt ) ;
  free( nsubs_pt   ) ;
  free( subs_pt    ) ;

  return out;
}


/* driver */
void
mser_compute (val_t *src, int width, int height, val_t in_delta)
{
    //  enum {IN_I=0, IN_DELTA} ;
    //  enum {OUT_REGIONS=0, OUT_ELL, OUT_PARENTS, OUT_AREA} ;

  dbg (DBG_INFO, "[mser] image size: %d x %d", width, height);

  int i ;
  idx_t rindex = 0 ;
  int k ; 

  /* configuration */
  int verbose = 1 ;      /* be verbose                              */
  int small_cleanup= 1 ; /* remove very small regions               */
  int big_cleanup  = 1 ; /* remove very big regions                 */
  int bad_cleanup  = 0 ; /* remove very bad regions                 */
  int dup_cleanup  = 1 ; /* remove duplicates                       */
  val_t delta ;          /* stability delta                         */

  /* node value denoting a void node */
  idx_t const node_is_void = 0xffffffff ;

  int*   subs_pt ;       /* N-dimensional subscript                 */
  int*   nsubs_pt ;      /* diff-subscript to point to neigh.       */
  idx_t* strides_pt ;    /* strides to move in image array          */
  idx_t* visited_pt ;    /* flag                                    */

  int nel ;              /* number of image elements (pixels)       */
  int ner = 0 ;          /* number of extremal regions              */
  int nmer = 0 ;         /* number of maximally stable              */
  int ndims ;            /* number of dimensions                    */
  int dims[2] ;          /* dimensions                              */
  int njoins = 0 ;       /* number of join ops                      */

  val_t const* I_pt ;    /* source image                            */
  mser_pair_t*   pairs_pt ;   /* scratch buffer to sort pixels           */
  mser_node_t*   forest_pt ;  /* the extremal regions forest             */
  region_t* regions_pt ; /* list of extremal regions found          */ 

  /* ellipses fitting */
  acc_t* acc_pt ;        /* accumulator to integrate region moments */
  acc_t* ell_pt ;        /* ellipses parameters                     */
  int    gdl ;           /* number of parameters of an ellipse      */
  idx_t* joins_pt ;      /* sequence of joins                       */
  
  /** -----------------------------------------------------------------
   **                                               Check the arguments
   ** -------------------------------------------------------------- */
  /*
  if (nin != 2) {
    mexErrMsgTxt("Two arguments required.") ;
  } else if (nout > 4) {
    mexErrMsgTxt("Too many output arguments.");
  }
  
  if(mxGetClassID(in[IN_I]) != mxUINT8_CLASS) {
    mexErrMsgTxt("I must be of class UINT8") ;
  }

  if(!uIsScalar(in[IN_DELTA])) {
    mexErrMsgTxt("DELTA must be scalar") ;
  }

  delta = 0 ;
  switch(mxGetClassID(in[IN_DELTA])) {
  case mxUINT8_CLASS :
    delta = * (val_t*) mxGetData(in[IN_DELTA]) ;
    break ;
    
  case mxDOUBLE_CLASS :
    {
      double x = *mxGetPr(in[IN_DELTA])  ;
      if(x < 0.0) {
        mexErrMsgTxt("DELTA must be non-negative") ;
      }
      delta = (val_t) x ;      
    }
    break ;
    
  default :
    mexErrMsgTxt("DELTA must be of class DOUBLE or UINT8") ;
  }
  */
  /* get dimensions */
  
  delta = in_delta;
  nel   = width*height;//mxGetNumberOfElements(in[IN_I]) ;
  ndims = 2;//mxGetNumberOfDimensions(in[IN_I]) ;
  dims[0] = width;
  dims[1] = height;//mxGetDimensions(in[IN_I]) ;
  I_pt  = src;//mxGetData(in[IN_I]) ;

  /* allocate stuff */
  subs_pt    = (int*)malloc( sizeof(int)      * ndims ) ;
  nsubs_pt   = (int*)malloc( sizeof(int)      * ndims ) ;
  strides_pt = (idx_t*)malloc( sizeof(idx_t)    * ndims ) ;
  visited_pt = (idx_t*)malloc( sizeof(idx_t)    * nel   ) ;
  regions_pt = (region_t*)malloc( sizeof(region_t) * nel   ) ;
  pairs_pt   = (mser_pair_t*)malloc( sizeof(mser_pair_t)   * nel   ) ;
  forest_pt  = (mser_node_t*)malloc( sizeof(mser_node_t)   * nel   ) ;
  joins_pt   = (idx_t*)malloc( sizeof(idx_t)    * nel   ) ;
  
  /* compute strides to move into the N-dimensional image array */
  strides_pt [0] = 1 ;
  for(k = 1 ; k < ndims ; ++k) {
    strides_pt [k] = strides_pt [k-1] * dims [k-1] ;
  }

  /* sort pixels by increasing intensity*/
  if (verbose)
      dbg (DBG_INFO, "sorting pixels ... ") ;

#ifndef USE_BUCKETSORT
  for(i = 0 ; i < nel ; ++i) {
    pairs_pt [i].value = I_pt [i] ;
    pairs_pt [i].index = i ;
  }
  qsort(pairs_pt, nel, sizeof(mser_pair_t), cmp_pair) ;
#else
  {
    int unsigned buckets [256] ;
    memset(buckets, 0, sizeof(int unsigned)*256) ;
    for(i = 0 ; i < nel ; ++i) {
      val_t v = I_pt [i] ;
      ++ buckets[v] ;
    }
    for(i = 1 ; i < 256 ; ++i) {
      buckets[i] += buckets[i-1] ;
    }
    for(i = nel ; i >= 1 ; ) {
      val_t v = I_pt [--i] ;
      int j = -- buckets [v] ;
      pairs_pt [j].value = v ;
      pairs_pt [j].index = i ;
    }
  }
#endif
  if (verbose) dbg (DBG_INFO, "done") ;

  /* initialize the forest with all void nodes */
  for(i = 0 ; i < nel ; ++i) {
    forest_pt [i].parent = node_is_void  ;
  }
  
  /* number of ellipse free parameters */
  gdl = ndims*(ndims+1)/2 + ndims ;

  /* -----------------------------------------------------------------
   *                                     Compute extremal regions tree
   * -------------------------------------------------------------- */
  if (verbose) dbg (DBG_INFO, "Computing extremal regions ... ") ;
  for(i = 0 ; i < nel ; ++i) {
    
    int done_all_neighbors = 0;

    /* pop next node xi */
    idx_t index = pairs_pt [i].index ;    
    val_t value = pairs_pt [i].value ;

    /* this will be needed later */
    rindex = index ;
    
    /* push it into the tree */
    forest_pt [index] .parent   = index ;
    forest_pt [index] .shortcut = index ;
    forest_pt [index] .area     = 1 ;
#ifdef USE_RANK_UNION
    forest_pt [index] .height   = 1 ;
#endif
    
    /* convert index into a subscript sub; also initialize nsubs 
       to (-1,-1,...,-1) */
    {
      idx_t temp = index ;
      for(k = ndims-1 ; k >=0 ; --k) {
        nsubs_pt [k] = -1 ;
        subs_pt  [k] = temp / strides_pt [k] ;
        temp         = temp % strides_pt [k] ;
      }
    }
    
    /* process neighbors of xi */
    while( true ) {
      int good = true ;
      idx_t nindex = 0 ;
      
      /* compute NSUBS+SUB, the correspoinding neighbor index NINDEX
         and check that the pixel is within image boundaries. */
      for(k = 0 ; k < ndims && good ; ++k) {
        int temp = nsubs_pt [k] + subs_pt [k] ;
        good &= 0 <= temp && temp < dims[k] ;
        nindex += temp * strides_pt [k] ;
      }      
      
      /* keep going only if
         1 - the neighbor is within image boundaries;
         2 - the neighbor is indeed different from the current node
             (this happens when nsub=(0,0,...,0));
         3 - the nieghbor is already in the tree, meaning that
             is a pixel older than xi.
      */
      if(good && 
         nindex != index &&
         forest_pt[nindex].parent != node_is_void ) {
        
        idx_t nrindex = 0, nvisited ;
        val_t nrvalue = 0 ;

#ifdef USE_RANK_UNION
        int height  = forest_pt [ rindex] .height ;
        int nheight = forest_pt [nrindex] .height ;
#endif
        
        /* RINDEX = ROOT(INDEX) might change as we merge trees, so we
           need to update it after each merge */
        
        /* find the root of the current node */
        /* also update the shortcuts */
        nvisited = 0 ;
        while( forest_pt[rindex].shortcut != rindex ) {          
          visited_pt[ nvisited++ ] = rindex ;
          rindex = forest_pt[rindex].shortcut ;
        }      
        while( nvisited-- ) {
          forest_pt [ visited_pt[nvisited] ] .shortcut = rindex ;
        }
        
        /* find the root of the neighbor */
        nrindex  = nindex ;
        nvisited = 0 ;
        while( forest_pt[nrindex].shortcut != nrindex ) {          
          visited_pt[ nvisited++ ] = nrindex ;
          nrindex = forest_pt[nrindex].shortcut ;
        }      
        while( nvisited-- ) {
          forest_pt [ visited_pt[nvisited] ] .shortcut = nrindex ;
        }
        
        /*
          Now we join the two subtrees rooted at
          
            RINDEX = ROOT(INDEX) and NRINDEX = ROOT(NINDEX).
          
          Only three things can happen:
          
          a - ROOT(INDEX) == ROOT(NRINDEX). In this case the two trees
              have already been joined and we do not do anything.
          
          b - I(ROOT(INDEX)) == I(ROOT(NRINDEX)). In this case index
              is extending an extremal region with the same
              value. Since ROOT(NRINDEX) will NOT be an extremal
              region of the full image, ROOT(INDEX) can be safely
              addedd as children of ROOT(NRINDEX) if this reduces
              the height according to union rank.
               
          c - I(ROOT(INDEX)) > I(ROOT(NRINDEX)) as index is extending
              an extremal region, but increasing its level. In this
              case ROOT(NRINDEX) WILL be an extremal region of the
              final image and the only possibility is to add
              ROOT(NRINDEX) as children of ROOT(INDEX).
        */

        if( rindex != nrindex ) {
          /* this is a genuine join */
          
          nrvalue = I_pt [nrindex] ;
          if( nrvalue == value 
#ifdef USE_RANK_UNION             
              && height < nheight
#endif
              ) {
            /* ROOT(INDEX) becomes the child */
            forest_pt[rindex] .parent   = nrindex ;
            forest_pt[rindex] .shortcut = nrindex ;          
            forest_pt[nrindex].area    += forest_pt[rindex].area ;

#ifdef USE_RANK_UNION
            forest_pt[nrindex].height = MAX(nheight, height+1) ;
#endif

            joins_pt[njoins++] = rindex ;

          } else {
            /* ROOT(index) becomes parent */
            forest_pt[nrindex] .parent   = rindex ;
            forest_pt[nrindex] .shortcut = rindex ;
            forest_pt[rindex]  .area    += forest_pt[nrindex].area ;

#ifdef USE_RANK_UNION
            forest_pt[rindex].height = MAX(height, nheight+1) ;
#endif
            if( nrvalue != value ) {            
              /* nrindex is extremal region: save for later */
              forest_pt[nrindex].region = ner ;
              regions_pt [ner] .index      = nrindex ;
              regions_pt [ner] .parent     = ner ;
              regions_pt [ner] .value      = nrvalue ;
              regions_pt [ner] .area       = forest_pt [nrindex].area ;
              regions_pt [ner] .area_top   = nel ;
              regions_pt [ner] .area_bot   = 0 ;            
              ++ner ;
            }
            
            /* annote join operation for post-processing */
            joins_pt[njoins++] = nrindex ;
          }
        }
        
      } /* neighbor done */
      
      /* move to next neighbor */      
      k = 0 ;
      while(++ nsubs_pt [k] > 1) {
        nsubs_pt [k++] = -1 ;
        if(k == ndims) {done_all_neighbors=1 ; break;}
      }
      if (done_all_neighbors) break;
    } /* next neighbor */
    //done_all_neighbors : ;
  } /* next pixel */    
    
  /* the root of the last processed pixel must be a region */
  forest_pt [rindex].region = ner ;
  regions_pt [ner] .index      = rindex ;
  regions_pt [ner] .parent     = ner ;
  regions_pt [ner] .value      = I_pt      [rindex] ;
  regions_pt [ner] .area       = forest_pt [rindex] .area ;
  regions_pt [ner] .area_top   = nel ;
  regions_pt [ner] .area_bot   = 0 ;            
  ++ner ;

  if (verbose)  dbg (DBG_INFO, "done. Extremal regions: %d", ner) ;
  
  /* -----------------------------------------------------------------
   *                                            Compute region parents
   * -------------------------------------------------------------- */
  for( i = 0 ; i < ner ; ++i) {
    idx_t index  = regions_pt [i].index ;    
    val_t value  = regions_pt [i].value ;
    int   j      = i ;

    while(j == i) {
      idx_t pindex = forest_pt [index].parent ;
      val_t pvalue = I_pt [pindex] ;

      /* top of the tree */
      if(index == pindex) {
        j = forest_pt[index].region ;
        break ;
      }

      /* if index is the root of a region, either this is still
         i, or it is the parent region we are looking for. */
      if(value < pvalue) {
        j = forest_pt[index].region ;
      }

      index = pindex ;
      value = pvalue ;
    }
    regions_pt[i]. parent = j ;
  }
  
  /* -----------------------------------------------------------------
   *                                 Compute areas of tops and bottoms
   * -------------------------------------------------------------- */

  /* We scan the list of regions from the bottom. Let x0 be the current
     region and be x1 = PARENT(x0), x2 = PARENT(x1) and so on.
     
     Here we do two things:
     
     1) Look for regions x for which x0 is the BOTTOM. This requires
        VAL(x0) <= VAL(x) - DELTA < VAL(x1).
        We update AREA_BOT(x) for each of such x found.
        
     2) Look for the region y which is the TOP of x0. This requires
          VAL(y) <= VAL(x0) + DELTA < VAL(y+1)
          We update AREA_TOP(x0) as soon as we find such y.
        
  */

  for( i = 0 ; i < ner ; ++i) {
    /* fix xi as the region, then xj are the parents */
    idx_t parent = regions_pt [i].parent ;
    int   val0   = regions_pt [i].value ;
    int   val1   = regions_pt [parent].value ;
    int   val    = val0 ;
    idx_t j = i ;

    while(true) {
      int valp = regions_pt [parent].value ;

      /* i is the bottom of j */
      if(val0 <= val - delta && val - delta < val1) {
        regions_pt [j].area_bot  =
          MAX(regions_pt [j].area_bot, regions_pt [i].area) ;
      }
      
      /* j is the top of i */
      if(val <= val0 + delta && val0 + delta < valp) {
        regions_pt [i].area_top = regions_pt [j].area ;
      }
      
      /* stop if going on is useless */
      if(val1 <= val - delta && val0 + delta < val)
        break ;

      /* stop also if j is the root */
      if(j == parent)
        break ;
      
      /* next region upward */
      j      = parent ;
      parent = regions_pt [j].parent ;
      val    = valp ;
    }
  }

  /* -----------------------------------------------------------------
   *                                                 Compute variation
   * -------------------------------------------------------------- */
  for(i = 0 ; i < ner ; ++i) {
    int area     = regions_pt [i].area ;
    int area_top = regions_pt [i].area_top ;
    int area_bot = regions_pt [i].area_bot ;    
    regions_pt [i].variation = 
      (float)(area_top - area_bot) / (float)area ;

    /* initialize .mastable to 1 for all nodes */
    regions_pt [i].maxstable = 1 ;
  }

  /* -----------------------------------------------------------------
   *                     Remove regions which are NOT maximally stable
   * -------------------------------------------------------------- */
  nmer = ner ;
  for(i = 0 ; i < ner ; ++i) {
    idx_t parent = regions_pt [i]      .parent ;
    float var    = regions_pt [i]      .variation ;
    float pvar   = regions_pt [parent] .variation ;
    idx_t loser ;

    /* decide which one to keep and put that in loser */
    if(var < pvar) loser = parent ; else loser = i ;
    
    /* make loser NON maximally stable */
    if(regions_pt [loser].maxstable) --nmer ;
    regions_pt [loser].maxstable = 0 ;
  }

  if (verbose)  dbg (DBG_INFO, "Maximally stable regions: %d (%.1f%%)", 
                       nmer, 100.0 * (double) nmer / ner) ;

  /* -----------------------------------------------------------------
   *                                               Remove more regions
   * -------------------------------------------------------------- */

  /* it is critical for correct duplicate detection to remove regions
     from the bottom (smallest one first) */

  if( big_cleanup || small_cleanup || bad_cleanup || dup_cleanup ) {
    int nbig   = 0 ;
    int nsmall = 0 ;
    int nbad   = 0 ;
    int ndup   = 0 ;

    /* scann all extremal regions */
    for(i = 0 ; i < ner ; ++i) {
      
      /* process only maximally stable extremal regions */
      if(! regions_pt [i].maxstable) continue ;
      
      int remove_this_region = 0;

      if( bad_cleanup   && regions_pt[i].variation >= 1.0f  ) {
        ++nbad ;
        remove_this_region = 1;
      }

      if( big_cleanup   && regions_pt[i].area      >  nel/2 ) {
        ++nbig ;
        remove_this_region = 1;
      }

      if( small_cleanup && regions_pt[i].area      <  25    ) {
        ++nsmall ;
        remove_this_region = 1;
      }
      
      /* 
       * Remove duplicates 
       */
      if( dup_cleanup ) {
        int parent = regions_pt [i].parent ;
        int area, parea ;
        float change ;
        
        /* the search does not apply to root regions */
        if(parent != i) {
          
          /* search for the maximally stable parent region */
          while(! regions_pt[parent].maxstable) {
            int next = regions_pt[parent].parent ;
            if(next == parent) break ;
            parent = next ;
          }
          
          /* compare with the parent region; if the current and parent
             regions are too similar, keep only the parent */
          area   = regions_pt [i].area ;
          parea  = regions_pt [parent].area ;
          change = (float)(parea - area)/area ;

          if(change < 0.5)  {
            ++ndup ;
            remove_this_region = 1;
          }

        } /* drop duplicates */ 
      }

      if (remove_this_region) {
        regions_pt[i].maxstable = false ;
        --nmer ;      
      }
    } /* next region to cleanup */

    if(verbose) {
      dbg (DBG_INFO, "  Bad regions:        %d", nbad   ) ;
      dbg (DBG_INFO, "  Small regions:      %d", nsmall ) ;
      dbg (DBG_INFO, "  Big regions:        %d", nbig   ) ;
      dbg (DBG_INFO, "  Duplicated regions: %d", ndup   ) ;
    }
  }

  if (verbose)  dbg (DBG_INFO, "Cleaned-up regions: %d (%.1f%%)", 
                       nmer, 100.0 * (double) nmer / ner) ;

  /* -----------------------------------------------------------------
   *                                                      Fit ellipses
   * -------------------------------------------------------------- */
  int midx = 1 ;
  int d, index, j ;
  
  if (verbose)  dbg (DBG_INFO, "Fitting ellipses...") ;
  
  /* enumerate maxstable regions */
  for(i = 0 ; i < ner ; ++i) {      
      if(! regions_pt [i].maxstable) continue ;
      regions_pt [i].maxstable = midx++ ;
  }
  
  /* allocate space */
  acc_pt = (acc_t*)malloc(sizeof(acc_t) * nel) ;
  ell_pt = (acc_t*)malloc(sizeof(acc_t) * gdl * nmer) ; 
  
  /* clear accumulators */
  memset(ell_pt, 0, sizeof(int) * gdl * nmer) ;
  
  /* for each gdl */
  for(d = 0 ; d < gdl ; ++d) {
    /* initalize parameter */
    memset(subs_pt, 0, sizeof(int) * ndims) ;
      
    if(d < ndims) {
        if (verbose)  dbg (DBG_INFO, " mean %d",d) ;
        for(index = 0 ; index < nel ; ++ index) {
          acc_pt[index] = subs_pt[d] ;
          adv(dims, ndims, subs_pt) ;
        }

    } else {

      /* decode d-ndims into a (i,j) pair */
      i = d-ndims ; 
      j = 0 ;
      while(i > j) {
        i -= j + 1 ;
        j ++ ;
      }

      if (verbose)  dbg (DBG_INFO, " corr (%d,%d)",i,j) ;
      /* add x_i * x_j */
      for(index = 0 ; index < nel ; ++ index){
        acc_pt[index] = subs_pt[i]*subs_pt[j] ;
        adv(dims, ndims, subs_pt) ;
      }
    }

    /* integrate parameter */
    for(i = 0 ; i < njoins ; ++i) {      
      idx_t index  = joins_pt[i] ;
      idx_t parent = forest_pt [ index ].parent ;
      acc_pt[parent] += acc_pt[index] ;
    }
    
    /* save back to ellpises */
    for(i = 0 ; i < ner ; ++i) {
      idx_t region = regions_pt [i].maxstable ;

      /* skip if not extremal region */
      if(region-- == 0) continue ;
      ell_pt [d + gdl*region] = acc_pt [ regions_pt[i].index ] ;
    }

    /* next gdl */
  }
  free(acc_pt) ;
  free(joins_pt);
  
  /* -----------------------------------------------------------------
   *                                                Save back and exit
   * -------------------------------------------------------------- */

  /*
   * Save extremal regions
   */
  /*
  {
    int dims[2] ;
    int unsigned * pt ;
    dims[0] = nmer ;
    out[OUT_REGIONS] = mxCreateNumericArray(1,dims,mxUINT32_CLASS,mxREAL);
    pt = mxGetData(out[OUT_REGIONS]) ;
    for (i = 0 ; i < ner ; ++i) {
      if( regions_pt[i].maxstable ) {
        // adjust for MATLAB index compatibility
        *pt++ = regions_pt[i].index + 1 ;
      }
    }
  }
  */

  /* convert to lcmtype
   *
  navlcm_blob_list_t out;
  out.num = nmer;
  out.el = (navlcm_blob_t*)malloc(nmer*sizeof(navlcm_blob_t));
  */

  /* keep this commented out.
   *
  for (int i=0;i<nmer;i++) {
      out.el[i].area = 10;
      out.el[i].x0 = out.el[i].y0 = out.el[i].s11 = out.el[i].s12 = out.el[i].s22 = 0.0;
      out.el[i].fill = (int*)malloc(10*sizeof(int));
      memset (out.el[i].fill, 0, 10*sizeof(int));
      }*/
 
  dbg (DBG_INFO, "[mser] creating structure with %d nmer", nmer);

  double * pt = (double*)malloc(gdl*sizeof(double));
  
  for(index = 0 ; index < ner ; ++index) {
      
    idx_t region = regions_pt [index] .maxstable ;      
    int   N      = regions_pt [index] .area ;
      
    if(region-- == 0) continue ;

    // save fitted ellipses
    for(d = 0  ; d  < gdl ; ++d) {

      //  printf ("d: %d\n", d); fflush (stdout);

      pt[d] = (double) ell_pt[gdl*region  + d] / N ;

      if(d < ndims) {
        // don't adjust for MATLAB coordinate frame convention 
        // pt[d] += 1 ; 
      } else {
        // remove squared mean from moment to get variance 
        i = d - ndims ; 
        j = 0 ;
        while(i > j) {
          i -= j + 1 ;
          j ++ ;
        }
        pt[d] -= (pt[i]-1)*(pt[j]-1) ;
      }
    }

      // copy to structure
      // see http://vision.ucla.edu/~vedaldi/code/mser/mser.html
      //printf ("1\n");

      // save filled region
      //      int mynum = 2;

      //      navlcm_mser_t mser;
      //printf ("%d\n", index); fflush (stdout);
      //printf("[mser] %d/%d computing fill region with index %d.\n", region, nmer, (int)regions_pt[index].index);
      //printf ("2\n"); fflush (stdout);
      //
      //navlcm_mser_t mser;
      int  mser_area = 0;
      int *fill = mser_fill (src, width, height, regions_pt [index] .index, &mser_area);

      //assert (mser.area > 0);
      //mser.x0 = pt[1];
      //mser.y0 = pt[0];
      //mser.s11 = pt[4];
      //mser.s12 = pt[3];
      //mser.s22 = pt[2];

      // fit an ellipse
      /*
       * navlcm_blob_t blob = mser_fit_ellipse (fill, mser_area, width, region);
      
      out.el[region] = blob;
        */

      free (fill);
    //    pt += gdl ;
  }

  free (pt);
  free(ell_pt) ;
  /*
  if(nout >= 3)  {
    int unsigned * pt ;
    out[OUT_PARENTS] = mxCreateNumericArray(ndims,dims,mxUINT32_CLASS,mxREAL) ;
    pt = mxGetData(out[OUT_PARENTS]) ;
    for(i = 0 ; i < nel ; ++i) {
      *pt++ = forest_pt[i].parent ;
    }
  }

  if(nout >= 4) {
    int dims[2] ;
    int unsigned * pt ;
    dims[0] = 3 ;
    dims[1]= ner ;
    out[OUT_AREA] = mxCreateNumericArray(2,dims,mxUINT32_CLASS,mxREAL);
    pt = mxGetData(out[OUT_AREA]) ;
    for( i = 0 ; i < ner ; ++i ) {
      *pt++ = regions_pt [i]. area_bot ;
      *pt++ = regions_pt [i]. area ;
      *pt++ = regions_pt [i]. area_top ;      
    }
   }
  */

  /* free stuff */
  free( forest_pt  ) ;
  free( pairs_pt   ) ;
  free( regions_pt ) ;
  free( visited_pt ) ;
  free( strides_pt ) ;
  free( nsubs_pt   ) ;
  free( subs_pt    ) ;

  dbg (DBG_INFO, "[mser] full done.");

  return;// out;
}
