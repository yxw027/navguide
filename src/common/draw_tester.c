// file: draw_tester.c
// auth: Albert Huang <albert@csail.mit.edu>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ppm.h"
#include "draw.h"


int main(int argc, char **argv)
{
    uint32_t w = 600;
    uint32_t h = 400;
    uint32_t rs = w * 3;

    uint8_t *img = (uint8_t*)malloc( h * rs );
    memset(img, 0, h*rs);

    draw_line_rgb(img, 0, 0, 600, 400, 0xFFFF00, w, h, rs);
    draw_hline_rgb(img, 0, 200, 200, 0xFF00FF, w, h, rs);
    draw_hline_rgb(img, 200, 300, 0, 0xFF00FF, w, h, rs);

    int triangle[] = { 300, 200,
                       300, 100,
                       400, 200  };
    draw_polygon_rgb(img, 3, triangle, 0x00FF00, 0, w, h, rs);

    draw_rect_rgb(img, 300, 300, 390, 390, 0xFFFFFF, 1, w, h, rs);

    draw_rect_rgb(img, 350, 250, 450, 330, 0x80FF0000, 1, w, h, rs);
    draw_rect_rgb(img, 400, 200, 500, 300, 0xa00000FF, 1, w, h, rs);

    draw_ellipse_rgb(img, 500, 100, 575, 150, 0x00FFFF, 1, 0,
            w, h, rs);

    draw_ellipse_rgb(img, 450, 25, 525, 100, 0x00FFFF, 0, 0,
            w, h, rs);

    draw_ellipse_rgb(img, -50,-50,50,50, 0xFFFF00, 1, 0, w, h, rs);

    FILE *fp = fopen("out.ppm", "wb");

    ppm_write( fp, img, w, h, w*3 );

    fclose( fp );

    free( img );

    printf("created out.ppm\n");

    return 0;
}
