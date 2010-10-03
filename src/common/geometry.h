#ifndef __dgc_geometry_h__
#define __dgc_geometry_h__

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===== 2 dimensional structures =====

// int

// float

#ifndef _point2d_t_h // XXX hack... _point2d_t_h is defined in
                     // lcmtypes/point2d_t.h
// double
typedef struct _point2d {
    double x;
    double y;
} point2d_t;
#endif

typedef point2d_t vec2d_t;

typedef struct _polygon2d_t {
    int npoints;
    point2d_t *points;
} polygon2d_t;

///**
// * quad2d_t:
// *
// * can also be casted as a polygon2d_t
// */
//typedef struct _quad2d_t {
//    int npoints;
//    point2d_t points[4];
//} quad2d_t;

// ===== 3 dimensional strucutres =====

// =========== constructors, destructors ==========

static inline point2d_t *
point2d_new (double x, double y) 
{
    point2d_t *self = (point2d_t*) malloc (sizeof (point2d_t));
    self->x = x;
    self->y = y;
    return self;
}

static inline point2d_t *
point2d_copy (const point2d_t *point) {
    point2d_t *self = (point2d_t*) malloc (sizeof (point2d_t));
    self->x = point->x;
    self->y = point->y;
    return (self);
}

static inline void point2d_free (point2d_t *self) { free (self); }

static inline polygon2d_t *
polygon2d_new (int npoints)
{
    polygon2d_t *self = (polygon2d_t*) malloc (sizeof (polygon2d_t));
    self->npoints = npoints;
    self->points = (point2d_t*) calloc (npoints, sizeof(point2d_t));
    return self;
}

static inline polygon2d_t *
polygon2d_new_from_points (const point2d_t *points, int npoints)
{
    polygon2d_t *self = (polygon2d_t*) malloc (sizeof (polygon2d_t));
    self->npoints = npoints;
    self->points = (point2d_t*) malloc (npoints * sizeof(point2d_t));
    memcpy (self->points, points, npoints * sizeof (point2d_t));
    return self;
}

static inline polygon2d_t *
polygon2d_new_from_glist (const GList *points)
{
    polygon2d_t *self = (polygon2d_t*) malloc (sizeof (polygon2d_t));
    self->npoints = g_list_length ( (GList*) points);
    self->points = (point2d_t*) malloc (self->npoints * sizeof (point2d_t));
    const GList *piter;
    int i = 0;
    for (piter=points; piter; piter=piter->next) {
        point2d_t *p = (point2d_t*) piter->data;
        self->points[i].x = p->x;
        self->points[i].y = p->y;
        i++;
    }
    return self;
}

static inline polygon2d_t *
polygon2d_copy (const polygon2d_t *poly)
{
    polygon2d_t *self = (polygon2d_t*) malloc (sizeof (polygon2d_t));
    self->npoints = poly->npoints;
    self->points = (point2d_t*) malloc (self->npoints * sizeof (point2d_t));
    memcpy (self->points, poly->points, self->npoints * sizeof (point2d_t));
    return self;
}

static inline void
polygon2d_free (polygon2d_t *self) { free (self->points); free (self); }

// =========== functions ===========

/**
 * geom_handedness_2d:
 *
 * Computes the handedness (clockwise or counterclockwise) of the line segments
 * (%p0 - %p1) - (%p1 - %p2).
 *
 * Returns: -1 if the segments are CCW (left handed)
 *           1 if CW (right handed)
 *           0 if colinear.
 */
static inline int
geom_handedness_2d (const point2d_t *p0, const point2d_t *p1, 
        const point2d_t *p2)
{
    point2d_t v0 = {
        p1->x - p0->x,
        p1->y - p0->y
    };
    point2d_t v1 = {
        p2->x - p1->x,
        p2->y - p1->y
    };
    double det = v1.x * v0.y - v1.y * v0.x;
    if(det < 0) return -1;
    if(det > 0) return 1;
    return 0;
}

/**
 * geom_line_seg_intersect_test_2d:
 *
 * Returns: 1 if the line segments (p0 - p1) and (p2 - p3) intersect.
 */
static inline int
geom_line_seg_intersect_test_2d (const point2d_t *p0, const point2d_t *p1,
                                 const point2d_t *p2, const point2d_t *p3)
{
    if ((geom_handedness_2d(p0, p1, p2) != geom_handedness_2d(p0, p1, p3)) &&
        (geom_handedness_2d(p2, p3, p0) != geom_handedness_2d(p2, p3, p1)))
        return 1;
    return 0;
}

/**
 * geom_point_inside_convex_polygon_2d:
 *
 * Returns: 1 if the point %p0 is completely inside the specified polygon
 * %poly, and 0 otherwise
 */
static inline int
geom_point_inside_convex_polygon_2d (const point2d_t *p, 
        const polygon2d_t *poly)
{
    if (poly->npoints < 3) return 0;
    int h = geom_handedness_2d (p, &poly->points[0], &poly->points[1]);
    for (int i=1; i<poly->npoints - 1; i++) {
        if (geom_handedness_2d (p, &poly->points[i], &poly->points[i+1]) != h)
            return 0;
    }
    return geom_handedness_2d (p, &poly->points[poly->npoints-1], 
            &poly->points[0]) == h;
}

/**
 * geom_line_seg_inside_convex_polygon_2d:
 *
 * Returns: 1 if the line segment [%p0, %p1] is completely inside the specified
 * polygon %poly, and 0 otherwise
 */
static inline int
geom_line_seg_inside_convex_polygon_2d (const point2d_t *p0, 
        const point2d_t *p1, const polygon2d_t *poly)
{
    return geom_point_inside_convex_polygon_2d (p0, poly) &&
           geom_point_inside_convex_polygon_2d (p1, poly);
}

/**
 * geom_line_seg_polygon_intersect_test_2d:
 *
 * Runtime O(n) where n is the number of points in the polygon
 *
 * Returns: 1 if the line segment [%p0, %p1] intersects the polygon %poly, and
 * 0 otherwise
 */
static inline int
geom_line_seg_polygon_intersect_test_2d (const point2d_t *p0,
        const point2d_t *p1, const polygon2d_t *poly)
{
    if (poly->npoints < 2) return 0;
    for (int i=0; i<poly->npoints-1; i++) {
        if (geom_line_seg_intersect_test_2d (p0, p1, 
                    &poly->points[i], &poly->points[i+1])) return 1;
    }
    return geom_line_seg_intersect_test_2d (p0, p1,
            &poly->points[poly->npoints-1], &poly->points[0]);
}

static inline int
geom_polygon_inside_convex_polygon_2d (const polygon2d_t *inner,
        const polygon2d_t *outer)
{
    for (int i=0; i<inner->npoints; i++) {
        if (geom_point_inside_convex_polygon_2d (&inner->points[i], outer))
            return 1;
    }
    return 0;
}

/**
 * geom_convex_polygon_convex_polygon_intersect_test_2d:
 *
 * Brute force intersection test.
 *
 * Runtime O(n * m), where n and m are the number of points in %a and %b.
 *
 * Returns: 1 if the specified polygons %a and %b intersect, and 0 otherwise.
 */
static inline int
geom_convex_polygon_convex_polygon_intersect_test_2d (const polygon2d_t *a,
        const polygon2d_t *b)
{
    if (a->npoints == 0 || b->npoints == 0) return 0;

    for (int i=0; i<a->npoints-1; i++) {
        if (geom_line_seg_polygon_intersect_test_2d (&a->points[i], 
                    &a->points[i+1], b)) return 1;
    }
    if (geom_line_seg_polygon_intersect_test_2d (&a->points[a->npoints-1],
            &a->points[0], b)) return 1;

    if (geom_polygon_inside_convex_polygon_2d (a, b)) return 1;
    if (geom_polygon_inside_convex_polygon_2d (b, a)) return 1;
    return 0;
}

/**
 * geom_rotate_point_2d:
 *
 * rotates a point %p0 by %theta radians counterclockwise about the origin.
 * The result is stored in ouput parameter %result
 */
static inline void
geom_rotate_point_2d (const point2d_t *p0, double theta, point2d_t *result)
{
    double costheta = cos (theta);
    double sintheta = sin (theta);
    result->x = p0->x * costheta - p0->y * sintheta;
    result->y = p0->x * sintheta + p0->y * costheta;
}

static inline double
geom_point_point_distance_squared_2d (const point2d_t *a, const point2d_t *b)
{
    return (a->x-b->x)*(a->x-b->x) + (a->y-b->y)*(a->y-b->y);
}

static inline double
geom_point_point_distance_2d (const point2d_t *a, const point2d_t *b)
{
    return sqrt ((a->x-b->x)*(a->x-b->x) + (a->y-b->y)*(a->y-b->y));
}

// ==== conversion functions ====

/**
 * geom_point2d_array_from_double_array:
 *
 * converts an array of doubles to a an array of point2d_t structures.  The
 * array should be laid out with X coordinates in even cells and Y coordinates
 * in odd cells
 */
static inline void
geom_point2d_array_from_double_array (point2d_t *dst, const double *src, 
        int npoints)
{
    int i;
    for (i=0; i<npoints; i++) { dst[i].x = src[2*i+0]; dst[i].y = src[2*i+1]; }
}

static inline void
geom_point2d_array_to_double_array (double *dst, const point2d_t *src, 
        int npoints)
{
    int i;
    for (i=0; i<npoints; i++) { dst[2*i+0] = src[i].x; dst[2*i+1] = src[i].y; }
}

#ifdef __cplusplus
}
#endif

#endif
