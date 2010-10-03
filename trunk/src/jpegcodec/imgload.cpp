#include "imgload.h"
/*
ImgColor *LoadImage( const char *filename,  image_format_t type, \
                     int *width, int *height, int *channels )
{
    switch ( type ) {

    case EXT_JPG:
        return load_jpeg( filename, width, height, channels );
    case EXT_PNG:
        dbg(DBG_INFO, "loading png file : %s\n", filename);
        return load_png( filename, width, height, channels );
    default:
        dbg(DBG_ERROR,"Unknown image type %s!\n", filename);
        return NULL;
    }

    // this should never happen
    return NULL;
}

void SaveImage( ImgColor *img, const char *filename, image_format_t type )
{
    switch ( type ) {

    case EXT_JPG:
        write_jpeg( img, filename, img->Width(), img->Height(), 3 );
        return;
    case EXT_PNG:
        dbg(DBG_INFO,"Saving png image into %s\n", filename);
        write_png( img, filename );
        return;
    default:
        dbg(DBG_ERROR,"Unknown image type %s!\n", filename);
        return;
    }

    // this should never happen
    return;
 
}

void SaveImage( ImgGray *img, const char *filename, image_format_t type )
{
    switch ( type ) {

    case EXT_JPG:
        dbg(DBG_INFO, "Error: writing grayscale jpeg is not supported!\n");
        //write_jpeg( img, filename, img->Width(), img->Height() );
        return;
    case EXT_PNG:
        dbg(DBG_INFO,"Saving png image into %s\n", filename);
        write_png( img, filename );
        return;
    default:
        dbg(DBG_ERROR,"Unknown image type %s!\n", filename);
        return;
    }

    // this should never happen
    return;
 
}
*/

void
GetImageInfo( const char *filename, image_format_t type,
              int *width, int *height, int *channels )
{
    switch ( type ) {

    case EXT_JPG:
        return get_jpeg_info( filename, width, height, channels );
    case EXT_PNG:
        return get_png_info( filename, width, height, channels );
    default:
        dbg(DBG_ERROR,"Unknown image type %s!\n", filename);
        return;
    }

    // this should never happen
    return;
}
