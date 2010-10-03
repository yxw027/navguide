#ifndef __convexhull_h__
#define __convexhull_h__

#include "geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * convexhull_simple_polygon_2d: 
 * computes the convex hull of a non-degenerate simple polygon (i.e. non
 * self-intersecting polygon with nonzero area) 
 * 
 * does not detect self-intersection or degeneracy
 *
 * Has O(n) runtime.
 *
 * Returns: a newly allocated right-handed polygon, or NULL if the polygon has
 * fewer than 3 points.
 */ 
polygon2d_t * convexhull_simple_polygon_2d (const polygon2d_t *input); 

/**
 * convexhull_graham_scan_2d:
 * computes the convex hull of a set of points using the Graham Scan method.
 *
 * Has O(n log n) runtime, where n is the number of points in the input.
 *
 * Returns: a newly allocated polygon
 */
polygon2d_t * convexhull_graham_scan_2d (const polygon2d_t *input);

#ifdef __cplusplus
}
#endif

#endif
