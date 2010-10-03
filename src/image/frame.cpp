#include "frame.h"

frame_t::frame_t ()
{    
}

void
frame_t::init (int n, int w, int h, int fid)
{
    if (n < (int)img.size()) {

        if (img[n]->Width() == w && img[n]->Height() == h)
            return;
    }

    while ((int)img.size() <= n) {
        
        img.push_back(new ImgColor (w, h));
        img_g.push_back(new ImgGray(w, h));
    }

    if (img[n]->Width() != w || img[n]->Height() != h) {
        delete img[n];
        delete img_g[n];
        img[n] = new ImgColor (w, h);
        img_g[n] = new ImgGray(w, h);
    }
    
    id = fid;
}


void
frame_t::convert_grayscale(int sid)
{
    if (sid >= (int)img.size() || sid >= (int)img_g.size())
        return;
    
    if (!img[sid] || !img_g[sid])
        return;

    img[sid]->Grayscale( img_g[sid] );
}

void
frame_t::convert_grayscale()
{
    int i;
    
    for (i=0;i<n_img();i++) {
        convert_grayscale (i);
    }
}

void
frame_t::clear()
{
    int i;

    for (i=0;i<n_img();i++) {
        delete img[i];
        delete img_g[i];
        img[i] = NULL;
        img_g[i] = NULL;
    }

    img.clear();
    img_g.clear();
}

void frame_t::write (int sid, const char *filename)
{
    write_jpeg( img[sid], filename, get_width(sid), get_height(sid), 3);
}

int frame_t::get_width (int sid)
{
    if ( sid < 0 || sid >= n_img())
        return 0;
    
    return img[sid]->Width();
}

int frame_t::get_height (int sid)
{
    if ( sid < 0 || sid >= n_img())
        return 0;
    
    return img[sid]->Height();
}

int frame_t::get_max_width ()
{
    int w = 0;

    for (int i=0;i<n_img();i++) {
        if (img[i]->Width()>w)
            w = img[i]->Width();
    }

    return w;
}

int frame_t::get_max_height ()
{
    int w = 0;

    for (int i=0;i<n_img();i++) {
        if (img[i]->Height()>w)
            w = img[i]->Height();
    }

    return w;
}
