#include "imagepool.h"

ImagePool::ImagePool ()
{
}

ImagePool::~ImagePool ()
{
    _images.clear();
}

void ImagePool::LoadImage ( ImgColor *img, int frameid )
{
    std::pair<int,ImgColor*> pp;

    // check if image is already loaded
    //if ( GetImage(frameid) != NULL )
    //    return;

    printlog("Loading frame %d in pool (%d elements)", frameid, (int)_images.size());

    // pop first image if too many images in the pool
    if ( _images.size() > POOL_MAX_IMAGES )
        PopImage();

    // if not, add it to the pool
    pp.first = frameid;
    pp.second = new ImgColor ( img );

    assert( pp.second );

    _images.push_back( pp );
}

ImgColor* ImagePool::GetImage( int frameid )
{
    for (vector< std::pair<int,ImgColor*> >::iterator iter = _images.begin();
         iter != _images.end(); iter++ ) {

        if ( iter->first == frameid )
            return iter->second;
    }

    return NULL;
}

void ImagePool::Clear()
{
    while ( !_images.empty() )
        PopImage();

    _images.clear();
}

/* remove the first image of the pool */
void ImagePool::PopImage()
{
    if ( _images.empty() )
        return;

    dbg(DBG_INFO, "Removing image %d from pool\n", _images[0].first);

    delete _images[0].second;
    
    _images.erase( _images.begin() );
}
