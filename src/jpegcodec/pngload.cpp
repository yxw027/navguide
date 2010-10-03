/*
 * Guillaume Cottenceau (gc at mandrakesoft.com)
 *
 * Copyright 2002 MandrakeSoft
 *
 * This software may be freely redistributed under the terms of the GNU
 * public license.
 *
 */


#include "pngload.h"
#include <png.h>

void
get_png_info(const char* filename, int *width, int *height, int *channels )
{
    png_byte header[8];	// 8 is the maximum size that can be checked

    /* open file and test for it being a png */
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        dbg(DBG_ERROR,"[read_png_file] File %s could not "
            "be opened for reading", filename);
        return;
    }
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8)) {
        dbg(DBG_ERROR,"[read_png_file] File %s is not "
            "recognized as a PNG file", filename);
        return;
    }
    
    /* initialize stuff */
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    
    if (!png_ptr) {
        dbg(DBG_ERROR,"[read_png_file] png_create_read_struct failed");
        return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        dbg(DBG_ERROR,"[read_png_file] png_create_info_struct failed");
        return;
    }
    
    if (setjmp(png_jmpbuf(png_ptr))) {
        dbg(DBG_ERROR,"[read_png_file] Error during init_io");
        return;
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    
    png_read_info(png_ptr, info_ptr);
    
    *width = (int)info_ptr->width;
    *height = (int)info_ptr->height;
    png_byte color_type = info_ptr->color_type;

    if ( color_type == PNG_COLOR_TYPE_RGB ) 
        *channels = 3;
    else
        *channels = 1;

    fclose(fp);

}

#if 0
ImgColor* 
load_png(const char* filename, int *width, int *height, int *channels )
{
    png_byte header[8];	// 8 is the maximum size that can be checked
    int x,y;

    // open file and test for it being a png
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        dbg(DBG_ERROR,"[read_png_file] File %s could not "
            "be opened for reading", filename);
        return NULL;
    }
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8)) {
        dbg(DBG_ERROR,"[read_png_file] File %s is not "
            "recognized as a PNG file", filename);
        return NULL;
    }
    
    /* initialize stuff */
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    
    if (!png_ptr) {
        dbg(DBG_ERROR,"[read_png_file] png_create_read_struct failed");
        return NULL;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        dbg(DBG_ERROR,"[read_png_file] png_create_info_struct failed");
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        dbg(DBG_ERROR,"[read_png_file] Error during init_io");
        return NULL;
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    
    png_read_info(png_ptr, info_ptr);
    
    *width = (int)info_ptr->width;
    *height = (int)info_ptr->height;
    png_byte color_type = info_ptr->color_type;
    png_byte bit_depth = info_ptr->bit_depth;
    
    dbg(DBG_INFO, "[read_png_file] width: %d  height: %d  color type: %d  bit_depth: %d\n", *width, *height,\
        color_type, bit_depth);

    //int number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);
    
    
    /* read file */
    if (setjmp(png_jmpbuf(png_ptr)))
        dbg(DBG_ERROR,"[read_png_file] Error during read_image");
    
    png_bytep *row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * *height);
    for (y=0; y<*height; y++)
        row_pointers[y] = (png_byte*) malloc(info_ptr->rowbytes);
    
    png_read_image(png_ptr, row_pointers);
    
    // copy to image color structure
    ImgColor *img = new ImgColor( *width, *height );
    
    if ( color_type == 2 ) {
        // RGB color type
        for (y=0;y<*height;y++) {
            for (x=0;x<*width;x++) {
                img->SetPixel( x, y, row_pointers[y][3*x+2],row_pointers[y][3*x+1], row_pointers[y][3*x]);
            }
        }
    } else if ( color_type == 1 ) {
        // grayscale
        for (y=0;y<*height;y++) {
            for (x=0;x<*width;x++) {
                img->SetPixel( x, y, row_pointers[y][x],row_pointers[y][x], row_pointers[y][x]);
            }
        }
    }

    fclose(fp);
    
    // free memory
    for (y=0; y<*height; y++)
        free ( row_pointers[y] );
    free( row_pointers );

    return img;
    
}
#endif
//int x, y;

//int width, height;
//png_byte color_type;
//png_byte bit_depth;

//png_structp png_ptr;
//png_infop info_ptr;
//int number_of_passes;
//png_bytep * row_pointers;

/*
void
write_png( ImgColor *img, const char *filename )
{
    write_png (img->IplImage(), img->Width(), img->Height(), filename);
}
*/
void 
write_png (unsigned char *img, int width, int height, int nchannels, const char *filename)
{
    int bit_depth = 8;
    int color_type = nchannels == 3 ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_GRAY; // to be changed to the correct value!

    /* create file */
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        dbg(DBG_ERROR,"[write_png_file] File %s could not be opened for writing", filename);
        return;
    }


    /* initialize stuff */
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    
    if (!png_ptr) {
        dbg(DBG_ERROR,"[write_png_file] png_create_write_struct failed");
        png_destroy_write_struct (&png_ptr, NULL);
        return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_info_struct (png_ptr, &info_ptr);
        dbg(DBG_ERROR,"[write_png_file] png_create_info_struct failed");
        return;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct (&png_ptr, NULL);
        dbg(DBG_ERROR,"[write_png_file] Error during init_io");
    }

    png_init_io(png_ptr, fp);
    
    /* write header */
    
    if (setjmp(png_jmpbuf(png_ptr))) {

        png_destroy_info_struct (png_ptr, &info_ptr);
        png_destroy_write_struct (&png_ptr, NULL);

        dbg(DBG_ERROR,"[write_png_file] Error during writing header");
        
        return;
    }
    
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 bit_depth, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    
    png_write_info(png_ptr, info_ptr);
        
    int x,y;
    png_bytep *row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    unsigned char *ptr = img;

    for (y=0; y<height; y++) {
        row_pointers[y] = (png_byte*) malloc(width*sizeof(png_byte)*nchannels);
        for (x=0;x<width;x++) {
           
            for (int k=0;k<nchannels;k++) {
            row_pointers[y][nchannels*x+k] = (png_byte)(*ptr);
            ptr++;
            }
        }
    }

    /* write bytes */
    if (setjmp(png_jmpbuf(png_ptr))) {

        dbg(DBG_ERROR,"[write_png_file] Error during writing bytes");
    }
    
    png_write_image(png_ptr, row_pointers);
        
    /* end write */
    if (setjmp(png_jmpbuf(png_ptr)))
        dbg(DBG_ERROR,"[write_png_file] Error during end of write");
    
    png_write_end(png_ptr, NULL);
    
    /* cleanup heap allocation */
    for (y=0; y<height; y++) {
        free( row_pointers[y]);
    }
    free( row_pointers );

    png_destroy_info_struct (png_ptr, &info_ptr);
    png_destroy_write_struct (&png_ptr, NULL);
    
    fclose(fp);
}

#if 0
void
write_png( ImgGray *img, const char *filename )
{
    int width = img->Width();
    int height = img->Height();
    int bit_depth = 8;
    int color_type = PNG_COLOR_TYPE_GRAY; // to be changed to the correct value!

    /* create file */
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        dbg(DBG_ERROR,"[write_png_file] File %s could not be opened for writing", filename);
        return;
    }
    
    /* initialize stuff */
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    
    if (!png_ptr) {
        dbg(DBG_ERROR,"[write_png_file] png_create_write_struct failed");
        return;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        dbg(DBG_ERROR,"[write_png_file] png_create_info_struct failed");
        return;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
        dbg(DBG_ERROR,"[write_png_file] Error during init_io");
    
    png_init_io(png_ptr, fp);
    
    /* write header */
    
    if (setjmp(png_jmpbuf(png_ptr)))
        dbg(DBG_ERROR,"[write_png_file] Error during writing header");
    
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 bit_depth, color_type, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    
    png_write_info(png_ptr, info_ptr);
        
    int x,y;
    png_bytep *row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    for (y=0; y<height; y++) {
        row_pointers[y] = (png_byte*) malloc(width*sizeof(png_byte));
        for (x=0;x<width;x++) {
            
            row_pointers[y][x] = (png_byte)(img->GetPixel(x,y));
        }
    }

    /* write bytes */
    if (setjmp(png_jmpbuf(png_ptr)))
        dbg(DBG_ERROR,"[write_png_file] Error during writing bytes");
    
    png_write_image(png_ptr, row_pointers);
        
    /* end write */
    if (setjmp(png_jmpbuf(png_ptr)))
        dbg(DBG_ERROR,"[write_png_file] Error during end of write");
    
    png_write_end(png_ptr, NULL);
    
    /* cleanup heap allocation */
    for (y=0; y<height; y++) {
        free( row_pointers[y]);
    }
    free( row_pointers );

    fclose(fp);
}
#endif

