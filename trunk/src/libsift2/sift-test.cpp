#include <iostream>

#include <sys/timeb.h>    // ftime(), struct timeb

#include <cstdlib>
#include <assert.h>

#include <sift.hpp>

using namespace std;

typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

inline u32 timeGetTime()
{
#ifdef _WIN32
    _timeb t;
    _ftime(&t);
#else
    timeb t;
    ftime(&t);
#endif

    return (u32)(t.time*1000+t.millitm);
}

// code from david lowe
void SkipComments(FILE *fp)
{
    int ch;
    int ret;

    ret = fscanf(fp," "); // Skip white space.
    while ((ch = fgetc(fp)) == '#') {
        while ((ch = fgetc(fp)) != '\n'  &&  ch != EOF)
            ;
        ret = fscanf(fp," ");
    }
    ungetc(ch, fp); // Replace last character read.
}

// code from david lowe
float *ReadPGM(FILE *fp, int *o_width, int *o_height)
{
    if (!fp)
        return NULL;

    int char1, char2, width, height, max, c1, c2, c3, r, c;
    float * image = NULL;

    char1 = fgetc(fp);
    char2 = fgetc(fp);
    SkipComments(fp);
    c1 = fscanf(fp, "%d", &width);
    SkipComments(fp);
    c2 = fscanf(fp, "%d", &height);
    SkipComments(fp);
    c3 = fscanf(fp, "%d", &max);

    if (char1 != 'P' || char2 != '5' || c1 != 1 || c2 != 1 || c3 != 1 || max > 255) {
        cerr << "Input is not a standard raw 8-bit PGM file." << endl
             << "Use xv or pnmdepth to convert file to 8-bit PGM format." << endl;
        exit(1);
    }

    fgetc(fp);  // Discard exactly one byte after header.

    printf ("Loading image %d x %d\n", width, height);

    *o_width = width;
    *o_height = height;

    // Create floating point image with pixels in range [0,1].
    image = (float*)malloc(width*height*sizeof(float));

    int stride = width;

    for (r = 0; r < height; r++)
        for (c = 0; c < width; c++)
            image[r*stride+c] = ((float) fgetc(fp)) / 255.0;

    printf ("reading done.\n");

    //Check if there is another image in this file, as the latest PGM
    // standard allows for multiple images.
    SkipComments(fp);
    if (getc(fp) == 'P') {
        cerr << "ignoring other images" << endl;
        ungetc('P', fp);
        return ReadPGM(fp, o_width, o_height);
        //image->next = nextimage;
    }
    printf ("Loading done.\n");

    return image;
}

int main(int argc, char **argv)
{
    int width, height;
    navlcm_feature_list_t* keys = NULL;

    float * image = ReadPGM(stdin, &width, &height);
    if (!image) {
        fprintf (stderr, "failed to read image on stdin.\n");
        return 1;
    }

    int levels = 3;
    double sigma_init = 1.6;
    double peak_thresh = 0.12;
    int doubleimsize = 1;
    double resize = 1.0;

    float fproctime;

    u32 basetime = timeGetTime();

    keys = get_keypoints_2 (image, width, height, levels, sigma_init, peak_thresh, doubleimsize, resize);

    fproctime = (timeGetTime()-basetime)*0.001f;

    printf ("Found %d keys in %.3f seconds.\n", keys->num, fproctime);

    navlcm_feature_list_t_destroy (keys);

    return 0;
}
