#ifndef __DRAW_H__
#define __DRAW_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// a lot of this code taken from the Python Imaging Library, which is
//                 Copyright (c) 1996-2004 by Fredrik Lundh
//                 Copyright (c) 1997-2004 by Secret Labs AB.
//

/* like (a * b + 127) / 255), but much faster on most platforms */
#define	MULDIV255(a, b, tmp)\
     	(tmp = (a) * (b) + 128, ((((tmp) >> 8) + (tmp)) >> 8))

#define	DRAW_BLEND(mask, in1, in2, tmp1, tmp2)\
	(MULDIV255(in1, mask, tmp1) + MULDIV255(in2, 255 - mask, tmp2))

#define DRAW_SET_PIXEL_RGB(out, color, tmp1, tmp2) \
{ \
    out[0] = DRAW_BLEND(UINT32_A(color),out[0],UINT32_R(color),tmp1,tmp2); \
    out[1] = DRAW_BLEND(UINT32_A(color),out[1],UINT32_G(color),tmp1,tmp2); \
    out[2] = DRAW_BLEND(UINT32_A(color),out[2],UINT32_B(color),tmp1,tmp2); \
}

#define UINT32_R(i) ((i>>16)&0xFF)
#define UINT32_G(i) ((i>>8)&0xFF)
#define UINT32_B(i) (i&0xFF)
#define UINT32_A(i) ((i>>24)&0xFF)

// draw a single point
static inline void
draw_point_rgb(uint8_t *img, 
        int x, int y, uint32_t color,
        int w, int h, int rowstride)
{
    uint32_t tmp1, tmp2;
    uint8_t *out = &img[ y*rowstride + x*3 ];
    if( x>=0 && x<w && y>=0 && y<h ) {
        DRAW_SET_PIXEL_RGB( out, color, tmp1, tmp2 );
    }
}

// like draw_point_rgb, but no bounds checking
static inline void
draw_point_rgb_nc(uint8_t *img, 
        int x, int y, uint32_t color, 
        int rowstride)
{
    uint32_t tmp1, tmp2;
    uint8_t *out = &img[ y*rowstride + x*3 ];
    DRAW_SET_PIXEL_RGB( out, color, tmp1, tmp2 );
}

void draw_hline_rgb(uint8_t *img, 
        int x0, int y0, int x1, uint32_t color,
        int w, int h, int rowstride);

void draw_line_rgb(uint8_t *img, int x0, int y0, int x1, int y1, 
        uint32_t color,
        int w, int h, int rowstride);

void draw_polygon_rgb(uint8_t *img, 
        int npoints, const int *points, uint32_t color, int fill,
        int w, int h, int rowstride);

static inline
void draw_rect_rgb(uint8_t *img, int x0, int y0, int x1, int y1, 
        uint32_t color, int fill,
        int w, int h, int rowstride)
{
    if( fill ) {
        int points[8] = { x0, y0, 
                          x1, y0,
                          x1, y1,
                          x0, y1 };
        draw_polygon_rgb( img, 4, points, color, fill, w, h, rowstride );
    } else {
        draw_line_rgb(img, x0, y0, x1, y0, color, w, h, rowstride);
        draw_line_rgb(img, x1, y0, x1, y1, color, w, h, rowstride);
        draw_line_rgb(img, x1, y1, x0, y1, color, w, h, rowstride);
        draw_line_rgb(img, x0, y1, x0, y0, color, w, h, rowstride);
    }
}

void draw_ellipse_rgb(uint8_t *img, int x0, int y0, int x1, int y1, 
        uint32_t color, int fill, int op, int w, int h, int rowstride);

void draw_image_rgb(uint8_t *dst, int dst_stride, int dw, int dh,
        uint8_t *src, int src_stride, int sw, int sh,
        int dx, int dy, int sx, int sy, int w, int h);

// ==================== grayscale ==================

// draw a single point
static inline void
draw_point_gray8(uint8_t *img, 
        int x, int y, uint8_t intensity,
        int w, int h, int rowstride)
{
    if( x>=0 && x<w && y>=0 && y<h ) {
        img[ y*rowstride + x ] = intensity;
    }
}

void draw_hline_gray8(uint8_t *img, 
        int x0, int y0, int x1, uint8_t intensity,
        int w, int h, int rowstride);

void draw_line_gray8(uint8_t *img, int x0, int y0, int x1, int y1, 
        uint8_t intensity,
        int w, int h, int rowstride);

void draw_polygon_gray8(uint8_t *img, 
        int npoints, const int *points, uint8_t intensity, int fill,
        int w, int h, int rowstride);

static inline
void draw_rect_gray8(uint8_t *img, int x0, int y0, int x1, int y1, 
        uint8_t intensity, int fill,
        int w, int h, int rowstride)
{
    if( fill ) {
        int points[8] = { x0, y0, 
                          x1, y0,
                          x1, y1,
                          x0, y1 };
        draw_polygon_gray8(img, 4, points, intensity, fill, w, h, rowstride);
    } else {
        draw_line_gray8(img, x0, y0, x1, y0, intensity, w, h, rowstride);
        draw_line_gray8(img, x1, y0, x1, y1, intensity, w, h, rowstride);
        draw_line_gray8(img, x1, y1, x0, y1, intensity, w, h, rowstride);
        draw_line_gray8(img, x0, y1, x0, y0, intensity, w, h, rowstride);
    }
}

void draw_ellipse_gray8(uint8_t *img, int x0, int y0, int x1, int y1, 
        uint8_t intensity, int fill, int op, int w, int h, int rowstride);

void draw_image_gray8(uint8_t *dst, int dst_stride, int dw, int dh,
        uint8_t *src, int src_stride, int sw, int sh,
        int dx, int dy, int sx, int sy, int w, int h);

#ifdef __cplusplus
}
#endif

#endif
