// tons of code ripped from Python Imaging Library.  See comments at bottom

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "draw.h"

#define CEIL(v)  (int) ceil(v)
#define FLOOR(v) ((v) >= 0.0 ? (int) (v) : (int) floor(v))

#define ARC 0
#define CHORD 1
#define PIESLICE 2

typedef struct {
    /* edge descriptor for polygon engine */
    int d; // 1 if up, -1 if down
    int x0, y0;
    int xmin, ymin, xmax, ymax;
    float dx; // line slope
} Edge;

static inline void
add_edge(Edge *e, int x0, int y0, int x1, int y1)
{
    /* printf("edge %d %d %d %d\n", x0, y0, x1, y1); */

    if (x0 <= x1) e->xmin = x0, e->xmax = x1;
    else e->xmin = x1, e->xmax = x0;

    if (y0 <= y1) e->ymin = y0, e->ymax = y1;
    else e->ymin = y1, e->ymax = y0;
    
    if (y0 == y1) {
        e->d = 0;
        e->dx = 0.0;
    } else {
        e->dx = ((float)(x1-x0)) / (y1-y0);
        e->d = (y0==e->ymin) ? 1 : -1;
    }

    e->x0 = x0;
    e->y0 = y0;
}

static int
x_cmp(const void *x0, const void *x1)
{
    float diff = *((float*)x0) - *((float*)x1);
    if (diff < 0)
        return -1;
    else if (diff > 0)
        return 1;
    else
        return 0;
}

void
draw_hline_rgb(uint8_t *img, 
        int x0, int y0, int x1, uint32_t color,
        int w, int h, int rowstride)
{
    int tmp;
    unsigned int tmp1, tmp2;

    if (y0 >= 0 && y0 < h) {
        if (x0 > x1) tmp = x0, x0 = x1, x1 = tmp;

        if (x0 < 0) x0 = 0;
        else if (x0 >= w) return;

        if (x1 < 0) return;
        else if (x1 >= w) x1 = w-1;

        if (x0 <= x1) {
            uint8_t* out = &img[ y0*rowstride + x0*3 ];
            while (x0 <= x1) {
                DRAW_SET_PIXEL_RGB( out, color, tmp1, tmp2 );
                x0++; 
                out += 3;
            }
        }
    }
}

void 
draw_line_rgb(uint8_t *img, int x0, int y0, int x1, int y1, 
        uint32_t color,
        int w, int h, int rowstride)
{
    int i, n, e;
    int dx, dy;
    int xs, ys;

    /* normalize coordinates */
    dx = x1-x0;
    if (dx < 0) dx = -dx, xs = -1;
    else xs = 1;
    dy = y1-y0;
    if (dy < 0) dy = -dy, ys = -1;
    else ys = 1;

    n = (dx > dy) ? dx : dy;

    if (dx == 0) {
        /* vertical */
        for (i = 0; i < dy; i++) {
            draw_point_rgb(img, x0, y0, color, w, h, rowstride);
            y0 += ys;
        }
    } else if (dy == 0) {
        /* horizontal */
        draw_hline_rgb(img, x0, y0, x1, color, w, h, rowstride);
    } else if (dx > dy) {
        /* bresenham, horizontal slope */
        n = dx;
        dy += dy;
        e = dy - dx;
        dx += dx;

        for (i = 0; i < n; i++) {
            draw_point_rgb(img, x0, y0, color, w, h, rowstride);
            if (e >= 0) {
                y0 += ys;
                e -= dx;
            }
            e += dy;
            x0 += xs;
        }
    } else {
        /* bresenham, vertical slope */
        n = dy;
        dx += dx;
        e = dx - dy;
        dy += dy;

        for (i = 0; i < n; i++) {
            draw_point_rgb(img, x0, y0, color, w, h, rowstride);
            if (e >= 0) {
                x0 += xs;
                e -= dy;
            }
            e += dx;
            y0 += ys;
        }

    }
}

//void
//draw_polygon_rgb(uint8_t *img, 
//        int npoints, int *x, int *y, uint32_t color,
//        int w, int h, int rowstride)
//{
//    int i;
//    int x0, y0, x1, y1;
//    for( i=0; i<npoints; i++ ) {
//        x0 = x[i];
//        y0 = y[i];
//        x1 = x[(i+1)%npoints];
//        y1 = y[(i+1)%npoints];
//        printf("%d: %d %d %d %d\n", i, x0, y0, x1, y1);
//        draw_line_rgb( img, x0, y0, x1, y1, color, w, h, rowstride );
//    }
//}

static int
_fill_polygon_rgb(uint8_t *img,
        int npoints, Edge *e, uint32_t color, 
        int w, int h, int rowstride)
{
    int i, j;
    float *xx;
    int ymin, ymax;
    float y;

    if (npoints <= 0) return 0;

    /* Find upper and lower polygon boundary (within image) */
    ymin = e[0].ymin;
    ymax = e[0].ymax;
    for (i = 1; i < npoints; i++) {
        if (e[i].ymin < ymin) ymin = e[i].ymin;
        if (e[i].ymax > ymax) ymax = e[i].ymax;
    }

    if (ymin < 0) ymin = 0;
    if (ymax >= h) ymax = h-1;

    /* Process polygon edges */
    xx = malloc(npoints * sizeof(float));
    if (!xx) return -1;

    for (;ymin <= ymax; ymin++) {
        y = ymin+0.5F;
        for (i = j = 0; i < npoints; i++) {
            if (y >= e[i].ymin && y <= e[i].ymax) {
                if (e[i].d == 0)
                    draw_hline_rgb(img, e[i].xmin, ymin, e[i].xmax, color,
                            w, h, rowstride);
                else
                    xx[j++] = (y-e[i].y0) * e[i].dx + e[i].x0;
            }
        }
        if (j == 2) {
            if (xx[0] < xx[1])
                draw_hline_rgb(img, CEIL(xx[0]-0.5), ymin, FLOOR(xx[1]+0.5),
                        color, w, h, rowstride);
            else
                draw_hline_rgb(img, CEIL(xx[1]-0.5), ymin, FLOOR(xx[0]+0.5),
                        color, w, h, rowstride);
        } else {
            qsort(xx, j, sizeof(float), x_cmp);
            for (i = 0; i < j-1 ; i += 2)
                draw_hline_rgb(img, CEIL(xx[i]-0.5), ymin, FLOOR(xx[i+1]+0.5),
                        color, w, h, rowstride);
        }
    }

    free(xx);

    return 0;
}

void
draw_polygon_rgb(uint8_t *img, int count, const int* xy, 
        uint32_t color, int fill,
        int w, int h, int rowstride)
{
    int i, n;

    if (count <= 0) return;

    if (fill) {
        /* Build edge list */
        Edge* e = malloc(count * sizeof(Edge));
        for (i = n = 0; i < count-1; i++)
            add_edge(&e[n++], xy[i+i], xy[i+i+1], xy[i+i+2], xy[i+i+3]);
        if (xy[i+i] != xy[0] || xy[i+i+1] != xy[1])
            add_edge(&e[n++], xy[i+i], xy[i+i+1], xy[0], xy[1]);
        _fill_polygon_rgb(img, n, e, color, w, h, rowstride);
        free(e);
    } else {

        /* Outline */
        for (i = 0; i < count-1; i++)
            draw_line_rgb(img, xy[i+i], xy[i+i+1], xy[i+i+2], xy[i+i+3], 
                   color, w, h, rowstride);
//            draw->line(im, xy[i+i], xy[i+i+1], xy[i+i+2], xy[i+i+3], ink);
        draw_line_rgb(img, xy[i+i], xy[i+i+1], xy[0], xy[1], color,
                w, h, rowstride);

    }
}

static int
_draw_ellipse_rgb(uint8_t *img, int x0, int y0, int x1, int y1, 
        int start, int end, uint32_t color, int fill, int mode, int op,
        int img_width, int img_height, int rowstride)
{
    int i, n;
    int cx, cy;
    int w, h;
    int x = 0, y = 0;
    int lx = 0, ly = 0;
    int sx = 0, sy = 0;

    w = x1 - x0;
    h = y1 - y0;
    if (w < 0 || h < 0) return 0;

    cx = (x0 + x1) / 2;
    cy = (y0 + y1) / 2;

    while (end < start) end += 360;

    if (mode != ARC && fill) {
        /* Build edge list */
        Edge* e = malloc((end - start + 3) * sizeof(Edge));
        if( !e ) abort();

        n = 0;

        for (i = start; i <= end; i++) {
            x = FLOOR((cos(i*M_PI/180) * w/2) + cx + 0.5);
            y = FLOOR((sin(i*M_PI/180) * h/2) + cy + 0.5);
            if (i != start)
                add_edge(&e[n++], lx, ly, x, y);
            else
                sx = x, sy = y;
            lx = x, ly = y;
        }

        if (n > 0) {
            /* close and draw polygon */
            if (mode == PIESLICE) {
                if (x != cx || y != cy) {
                    add_edge(&e[n++], x, y, cx, cy);
                    add_edge(&e[n++], cx, cy, sx, sy);
                }
            } else {
                if (x != sx || y != sy)
                    add_edge(&e[n++], x, y, sx, sy);
            }
            _fill_polygon_rgb(img, n, e, color, 
                    img_width, img_height, rowstride);
        }
        free(e);
    } else {
        for (i = start; i <= end; i++) {
            x = FLOOR((cos(i*M_PI/180) * w/2) + cx + 0.5);
            y = FLOOR((sin(i*M_PI/180) * h/2) + cy + 0.5);
            if (i != start)
                draw_line_rgb(img, lx, ly, x, y, color, 
                    img_width, img_height, rowstride);
            else
                sx = x, sy = y;
            lx = x, ly = y;
        }

        if (i != start) {
            if (mode == PIESLICE) {
                if (x != cx || y != cy) {
                    draw_line_rgb(img, x, y, cx, cy, color,
                            img_width, img_height, rowstride);
                    draw_line_rgb(img, cx, cy, sx, sy, color,
                            img_width, img_height, rowstride);
                }
            } else if (mode == CHORD) {
                if (x != sx || y != sy)
                    draw_line_rgb(img, x, y, sx, sy, color,
                            img_width, img_height, rowstride);
            }
        }
    }

    return 0;
}

void
draw_arc_rgb(uint8_t *img, int x0, int y0, int x1, int y1,
        int start, int end, uint32_t color, int op,
        int w, int h, int rowstride)
{
    _draw_ellipse_rgb(img, x0, y0, x1, y1, start, end, color, 0, ARC, op,
            w, h, rowstride);
}

void draw_ellipse_rgb(uint8_t *img, int x0, int y0, int x1, int y1, 
        uint32_t color,
        int fill, int op,
        int w, int h, int rowstride)
{
    _draw_ellipse_rgb(img, x0, y0, x1, y1, 0, 360, color, fill, CHORD, op,
            w, h, rowstride);
}


void draw_image_rgb(uint8_t *dst, int dst_stride, int dw, int dh,
        uint8_t *src, int src_stride, int sw, int sh,
        int dx, int dy, int sx, int sy, int w, int h)
{
    int x, y;
    int xs, ys, xd, yd;
    for( y=0; y<h; y++ ) {
        ys = sy + y;
        yd = dy + y;

        if( ys < 0 || ys >= sh || yd < 0 || yd >= dh ) continue;

        for( x=0; x<w; x++ ) {
            xs = sx + x;
            xd = dx + x;
            if( xs < 0 || xs >= sw || xd < 0 || xd >= dw ) continue;

            dst[ yd*dst_stride + xd*3 + 0 ] = src[ ys*src_stride + xs*3 + 0 ];
            dst[ yd*dst_stride + xd*3 + 1 ] = src[ ys*src_stride + xs*3 + 1 ];
            dst[ yd*dst_stride + xd*3 + 2 ] = src[ ys*src_stride + xs*3 + 2 ];
        }
    }
}

// ====================== grayscale routines ======================

void
draw_hline_gray8(uint8_t *img, 
        int x0, int y0, int x1, uint8_t intensity,
        int w, int h, int rowstride)
{
    int tmp;
    if (y0 >= 0 && y0 < h) {
        if (x0 > x1) tmp = x0, x0 = x1, x1 = tmp;
        if (x0 < 0) x0 = 0;
        else if (x0 >= w) return;
        if (x1 < 0) return;
        else if (x1 >= w) x1 = w-1;
        if (x0 <= x1) {
            while (x0 <= x1) {
                img[ y0*rowstride+x0 ] = intensity;
                x0++; 
            }
        }
    }
}

void 
draw_line_gray8(uint8_t *img, int x0, int y0, int x1, int y1, 
        uint8_t intensity,
        int w, int h, int rowstride)
{
    int i, n, e;
    int dx, dy;
    int xs, ys;

    /* normalize coordinates */
    dx = x1-x0;
    if (dx < 0) dx = -dx, xs = -1;
    else xs = 1;
    dy = y1-y0;
    if (dy < 0) dy = -dy, ys = -1;
    else ys = 1;

    n = (dx > dy) ? dx : dy;

    if (dx == 0) {
        /* vertical */
        for (i = 0; i < dy; i++) {
            draw_point_gray8(img, x0, y0, intensity, w, h, rowstride);
            y0 += ys;
        }
    } else if (dy == 0) {
        /* horizontal */
        draw_hline_gray8(img, x0, y0, x1, intensity, w, h, rowstride);
    } else if (dx > dy) {
        /* bresenham, horizontal slope */
        n = dx;
        dy += dy;
        e = dy - dx;
        dx += dx;

        for (i = 0; i < n; i++) {
            draw_point_gray8(img, x0, y0, intensity, w, h, rowstride);
            if (e >= 0) {
                y0 += ys;
                e -= dx;
            }
            e += dy;
            x0 += xs;
        }
    } else {
        /* bresenham, vertical slope */
        n = dy;
        dx += dx;
        e = dx - dy;
        dy += dy;

        for (i = 0; i < n; i++) {
            draw_point_gray8(img, x0, y0, intensity, w, h, rowstride);
            if (e >= 0) {
                x0 += xs;
                e -= dy;
            }
            e += dx;
            y0 += ys;
        }

    }
}

static int
_fill_polygon_gray8(uint8_t *img,
        int npoints, Edge *e, uint8_t intensity, 
        int w, int h, int rowstride)
{
    int i, j;
    float *xx;
    int ymin, ymax;
    float y;

    if (npoints <= 0) return 0;

    /* Find upper and lower polygon boundary (within image) */
    ymin = e[0].ymin;
    ymax = e[0].ymax;
    for (i = 1; i < npoints; i++) {
        if (e[i].ymin < ymin) ymin = e[i].ymin;
        if (e[i].ymax > ymax) ymax = e[i].ymax;
    }

    if (ymin < 0) ymin = 0;
    if (ymax >= h) ymax = h-1;

    /* Process polygon edges */
    xx = malloc(npoints * sizeof(float));
    if (!xx) return -1;

    for (;ymin <= ymax; ymin++) {
        y = ymin+0.5F;
        for (i = j = 0; i < npoints; i++) {
            if (y >= e[i].ymin && y <= e[i].ymax) {
                if (e[i].d == 0)
                    draw_hline_gray8(img, e[i].xmin, ymin, e[i].xmax, 
                            intensity, w, h, rowstride);
                else
                    xx[j++] = (y-e[i].y0) * e[i].dx + e[i].x0;
            }
        }
        if (j == 2) {
            if (xx[0] < xx[1])
                draw_hline_gray8(img, CEIL(xx[0]-0.5), ymin, FLOOR(xx[1]+0.5),
                        intensity, w, h, rowstride);
            else
                draw_hline_gray8(img, CEIL(xx[1]-0.5), ymin, FLOOR(xx[0]+0.5),
                        intensity, w, h, rowstride);
        } else {
            qsort(xx, j, sizeof(float), x_cmp);
            for (i = 0; i < j-1 ; i += 2)
                draw_hline_gray8(img, CEIL(xx[i]-0.5), ymin, FLOOR(xx[i+1]+0.5),
                        intensity, w, h, rowstride);
        }
    }

    free(xx);

    return 0;
}

void
draw_polygon_gray8(uint8_t *img, int count, const int* xy, 
        uint8_t intensity, int fill,
        int w, int h, int rowstride)
{
    int i, n;

    if (count <= 0) return;

    if (fill) {
        /* Build edge list */
        Edge* e = malloc(count * sizeof(Edge));
        for (i = n = 0; i < count-1; i++)
            add_edge(&e[n++], xy[i+i], xy[i+i+1], xy[i+i+2], xy[i+i+3]);
        if (xy[i+i] != xy[0] || xy[i+i+1] != xy[1])
            add_edge(&e[n++], xy[i+i], xy[i+i+1], xy[0], xy[1]);
        _fill_polygon_gray8(img, n, e, intensity, w, h, rowstride);
        free(e);
    } else {

        /* Outline */
        for (i = 0; i < count-1; i++)
            draw_line_gray8(img, xy[i+i], xy[i+i+1], xy[i+i+2], xy[i+i+3], 
                   intensity, w, h, rowstride);
//            draw->line(im, xy[i+i], xy[i+i+1], xy[i+i+2], xy[i+i+3], ink);
        draw_line_gray8(img, xy[i+i], xy[i+i+1], xy[0], xy[1], intensity,
                w, h, rowstride);

    }
}

static int
_draw_ellipse_gray8(uint8_t *img, int x0, int y0, int x1, int y1, 
        int start, int end, uint8_t intensity, int fill, int mode, int op,
        int img_width, int img_height, int rowstride)
{
    int i, n;
    int cx, cy;
    int w, h;
    int x = 0, y = 0;
    int lx = 0, ly = 0;
    int sx = 0, sy = 0;

    w = x1 - x0;
    h = y1 - y0;
    if (w < 0 || h < 0) return 0;

    cx = (x0 + x1) / 2;
    cy = (y0 + y1) / 2;

    while (end < start) end += 360;

    if (mode != ARC && fill) {
        /* Build edge list */
        Edge* e = malloc((end - start + 3) * sizeof(Edge));
        if( !e ) abort();

        n = 0;

        for (i = start; i <= end; i++) {
            x = FLOOR((cos(i*M_PI/180) * w/2) + cx + 0.5);
            y = FLOOR((sin(i*M_PI/180) * h/2) + cy + 0.5);
            if (i != start)
                add_edge(&e[n++], lx, ly, x, y);
            else
                sx = x, sy = y;
            lx = x, ly = y;
        }

        if (n > 0) {
            /* close and draw polygon */
            if (mode == PIESLICE) {
                if (x != cx || y != cy) {
                    add_edge(&e[n++], x, y, cx, cy);
                    add_edge(&e[n++], cx, cy, sx, sy);
                }
            } else {
                if (x != sx || y != sy)
                    add_edge(&e[n++], x, y, sx, sy);
            }
            _fill_polygon_gray8(img, n, e, intensity, 
                    img_width, img_height, rowstride);
        }
        free(e);
    } else {
        for (i = start; i <= end; i++) {
            x = FLOOR((cos(i*M_PI/180) * w/2) + cx + 0.5);
            y = FLOOR((sin(i*M_PI/180) * h/2) + cy + 0.5);
            if (i != start)
                draw_line_gray8(img, lx, ly, x, y, intensity, 
                    img_width, img_height, rowstride);
            else
                sx = x, sy = y;
            lx = x, ly = y;
        }

        if (i != start) {
            if (mode == PIESLICE) {
                if (x != cx || y != cy) {
                    draw_line_gray8(img, x, y, cx, cy, intensity,
                            img_width, img_height, rowstride);
                    draw_line_gray8(img, cx, cy, sx, sy, intensity,
                            img_width, img_height, rowstride);
                }
            } else if (mode == CHORD) {
                if (x != sx || y != sy)
                    draw_line_gray8(img, x, y, sx, sy, intensity,
                            img_width, img_height, rowstride);
            }
        }
    }

    return 0;
}

void
draw_arc_gray8(uint8_t *img, int x0, int y0, int x1, int y1,
        int start, int end, uint8_t intensity, int op,
        int w, int h, int rowstride)
{
    _draw_ellipse_gray8(img, x0, y0, x1, y1, start, end, intensity, 0, ARC, op,
            w, h, rowstride);
}

void draw_ellipse_gray8(uint8_t *img, int x0, int y0, int x1, int y1, uint8_t intensity,
        int fill, int op,
        int w, int h, int rowstride)
{
    _draw_ellipse_gray8(img, x0, y0, x1, y1, 0, 360, intensity, fill, CHORD, op,
            w, h, rowstride);
}

void draw_image_gray8(uint8_t *dst, int dst_stride, int dw, int dh,
        uint8_t *src, int src_stride, int sw, int sh,
        int dx, int dy, int sx, int sy, int w, int h)
{
    int x, y;
    int xs, ys, xd, yd;
    for( y=0; y<h; y++ ) {
        ys = sy + y;
        yd = dy + y;

        if( ys < 0 || ys >= sh || yd < 0 || yd >= dh ) continue;

        for( x=0; x<w; x++ ) {
            xs = sx + x;
            xd = dx + x;
            if( xs < 0 || xs >= sw || xd < 0 || xd >= dw ) continue;

            dst[ yd*dst_stride + xd ] = src[ ys*src_stride + xs ];
        }
    }
}










//int
//ImagingDrawChord(Imaging im, int x0, int y0, int x1, int y1,
//               int start, int end, const void* ink, int fill, int op)
//{
//    return ellipse(im, x0, y0, x1, y1, start, end, ink, fill, CHORD, op);
//}
//
//int
//ImagingDrawPieslice(Imaging im, int x0, int y0, int x1, int y1,
//                    int start, int end, const void* ink, int fill, int op)
//{
//    return ellipse(im, x0, y0, x1, y1, start, end, ink, fill, PIESLICE, op);
//}

/*

The Python Imaging Library is

Copyright (c) 1997-2005 by Secret Labs AB
Copyright (c) 1995-2005 by Fredrik Lundh

By obtaining, using, and/or copying this software and/or its
associated documentation, you agree that you have read, understood,
and will comply with the following terms and conditions:

Permission to use, copy, modify, and distribute this software and its
associated documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies, and that both that copyright notice and this permission notice
appear in supporting documentation, and that the name of Secret Labs
AB or the author not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

SECRET LABS AB AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS.  IN NO EVENT SHALL SECRET LABS AB OR THE AUTHOR BE LIABLE FOR
ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

 */
