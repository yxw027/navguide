#ifndef _MSER_H__
#define _MSER_H__

#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<string.h>
#include<assert.h>

#include <common/dbg.h>

#include <common/mathutil.h>

#define USE_BUCKET_SORT
#define USE_RANK_UNION

typedef char unsigned val_t ;
typedef int  unsigned idx_t ;
typedef long long int unsigned acc_t ;

/* pairs are used to sort the pixels */
typedef struct 
{
  val_t value ; 
  idx_t  index ;
} mser_pair_t ;

/* forest node */
typedef struct
{
  idx_t parent ;   /**< parent pixel         */
  idx_t shortcut ; /**< shortcut to the root */
  idx_t region ;   /**< index of the region  */
  int   area ;     /**< area of the region   */
#ifdef USE_RANK_UNION
  int   height ;    /**< node height         */
#endif
} mser_node_t ;

/* extremal regions */
typedef struct
{
  idx_t parent ;     /**< parent region                           */
  idx_t index ;      /**< index of root pixel                     */
  val_t value ;      /**< value of root pixel                     */
  int   area ;       /**< area of the region                      */
  int   area_top ;   /**< area of the region DELTA levels above   */
  int   area_bot  ;  /**< area of the region DELTA levels below   */
  float variation ;  /**< variation                               */
  int   maxstable ;  /**< max stable number (=0 if not maxstable) */
} region_t ;

void
mser_compute (val_t *src, int width, int height, val_t in_delta);

#endif
