#include "ppmload.h"

void write_pgm (unsigned char *data, int width, int height, const char *fname)
{
    FILE *fp = fopen (fname, "wb");
    if (!fp) {
        dbg (DBG_ERROR, "[ppm_load] failed to open file %s in w mode", fname);
        return;
    }

    // header
    fprintf (fp, "P2\n"); // plain format is P2, P5 otherwise
    fprintf (fp, "%d %d\n", width, height);
    
    // maxval
    unsigned char *ptr = data;
    int maxval=0;
    for (int r=0;r<height;r++) {
        for (int c=0;c<width;c++) {
            maxval = maxval < *ptr ? *ptr : maxval;
            ptr++;
        }
    }

    fprintf (fp, "%d\n", maxval);

    // data
    ptr = data;
    for (int r=0;r<height;r++) {
        for (int c=0;c<width;c++) {
            fprintf (fp, "%d ", *ptr);
            ptr++;
            if ((c+1)%15 == 0) fprintf (fp, "\n"); // make sure that no line has more than 70 characters
        }
        fprintf (fp, "\n");
    }

    fclose (fp);
}

void write_ppm (unsigned char *data, int width, int height, const char *fname)
{
    FILE *fp = fopen (fname, "w");
    if (!fp) {
        dbg (DBG_ERROR, "[ppm_load] failed to open file %s in w mode", fname);
        return;
    }

    // header
    fprintf (fp, "P3\n"); //P3  for plain format, P6 otherwise
    fprintf (fp, "%d %d\n", width, height);

    // maxval
    unsigned char *ptr = data;
    int maxval=0;
    for (int r=0;r<height;r++) {
        for (int c=0;c<width;c++) {
            for (int k=0;k<3;k++) {
                maxval = maxval < *ptr ? *ptr : maxval;
                ptr++;
            }
        }
    }

    fprintf (fp, "%d\n", maxval);

    // data
    ptr = data;
    int count=0;
    for (int r=0;r<height;r++) {
        for (int c=0;c<width;c++) {
            for (int k=0;k<3;k++) {
                fprintf (fp, "%d ", *ptr);
                ptr++;
                count++;
                if (count%15 == 0) fprintf (fp, "\n"); // make sure that no line has more than 70 char
            }
        }
        fprintf (fp, "\n");
    }

    fclose (fp);
}

