#!/usr/bin/python2.5
import gtk
import math
from PIL import Image
from cStringIO import StringIO

from image_t import image_t

def draw_cross (widget, width, height, bpp):
    if widget is None:
        return
    # create a pixmap
    colormap = gtk.gdk.colormap_get_system()
    pixmap = gtk.gdk.Pixmap (None, width, height, bpp)
    # create a graphics context
    rgc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(65535,0,0)));
    bgc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(0,0,0)));
    # black background
    pixmap.draw_rectangle (bgc, True, 0, 0, width, height)
    # red lines
    pixmap.draw_line (rgc, 0, 0, width, height)
    pixmap.draw_line (rgc, 0, height, width, 0)
    # set widget from pixbuf
    widget.set_from_pixmap (pixmap, None)

def draw_compass (widget, angle_rad, angle_rad_2, width, height, bpp):
    if widget is None:
        return
    w = width 
    h = height
    depth = bpp
    # create a pixmap
    colormap = gtk.gdk.colormap_get_system()
    pixmap = gtk.gdk.Pixmap (None, w, h, bpp)
    # create a graphics context
    rgc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(65535,0,0)));
    ygc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(65535,65535,0)));
    bgc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(0,0,0)));
    wgc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(65535, 65535, 65535)));
    # black background
    pixmap.draw_rectangle (bgc, True, 0, 0, w, h)
    # white circle
    #wgc.set_line_attributes (2, gtk.gdk.LINE_SOLID, gtk.gdk.CAP_BUTT, gtk.gdk.JOIN_MITER);
    pixmap.draw_arc (wgc, False, w/10, h/10, 8*w/10, 8*h/10, 0, 360 * 64);
    ccx = w/2
    ccy = h/2
    ccrx = 4*w/10
    ccry = 4*h/10
    # ticks
    for i in range(0,360, 15):
        x2 = int(ccx + ccrx * math.cos (math.radians (i)))
        y2 = int(ccy - ccry * math.sin (math.radians (i)))
        x1 = int(ccx + .9 * ccrx * math.cos (math.radians (i)))
        y1 = int(ccy - .9 * ccry * math.sin (math.radians (i)))
        pixmap.draw_line (wgc, x1, y1, x2, y2)
    # draw arrow 1
#    x1 = int(ccx + ccrx * math.cos (angle_rad_2+math.pi/2))#
#    y1 = int(ccy - ccry * math.sin (angle_rad_2+math.pi/2))#
#    x2 = int(ccx - ccrx/7 * math.sin (angle_rad_2+math.pi/2))#
#    y2 = int(ccy - ccry/7 * math.cos (angle_rad_2+math.pi/2))#
#    x3 = int(ccx + ccrx/7 * math.sin (angle_rad_2+math.pi/2))#
#    y3 = int(ccy + ccry/7 * math.cos (angle_rad_2+math.pi/2))#
#    pixmap.draw_polygon (ygc, False, [(x1, y1), (x2, y2), (x3, y3)])#
    # draw arrow 2
    x1 = int(ccx + ccrx * math.cos (angle_rad+math.pi/2))
    y1 = int(ccy - ccry * math.sin (angle_rad+math.pi/2))
    x2 = int(ccx - ccrx/7 * math.sin (angle_rad+math.pi/2))
    y2 = int(ccy - ccry/7 * math.cos (angle_rad+math.pi/2))
    x3 = int(ccx + ccrx/7 * math.sin (angle_rad+math.pi/2))
    y3 = int(ccy + ccry/7 * math.cos (angle_rad+math.pi/2))
    pixmap.draw_polygon (rgc, True, [(x1, y1), (x2, y2), (x3, y3)])

    # set widget from pixbuf
    widget.set_from_pixmap (pixmap, None)

def create_image_from_data (data, width, height, bitsperpixel):
    img = image_t ()
    img.width = width
    img.height = height
    img.size = width * height * bitsperpixel / 8
    img.data = data
    if data is None:
        img.data = "0" * img.size
    img.nmetadata = 0
    img.metadata = []
    img.utime = 0
    if bitsperpixel == 8:
        img.pixelformat = image_t.PIXEL_FORMAT_GRAY
    else:
        img.pixelformat = image_t.PIXEL_FORMAT_RGB
    img.row_stride = img.size / img.height
    return img

def draw_progress (widget, progress_pct, width, height, bpp):
    if widget is None:
        return
    w = width 
    h = height
    depth = bpp
    # create a pixmap
    colormap = gtk.gdk.colormap_get_system()
    pixmap = gtk.gdk.Pixmap (None, w, h, bpp)
    # create a graphics context
    ggc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(6500, 0, 655)));
    lgc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(0, 0, 65535)));
    bgc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(0,0,0)));
    wgc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(65535, 65535, 65535)));
    # dark blue background
    pixmap.draw_rectangle (ggc, True, 0, 0, w, h)
    # blue progress
    pixmap.draw_rectangle (lgc, True, 0, 0, int(progress_pct/100.0*w), h)
    # black bars
    for i in range (0,w,w/20):
        pixmap.draw_line (bgc, i, 0, i, h)
    # set widget from pixbuf
    widget.set_from_pixmap (pixmap, None)


def draw_jpeg_image (image, widget):
    im = Image.open (StringIO (image.data))
    imrgb = im.convert ("RGB")
    # decompress
    #(data, width, height, bitsperpixel) = jpeg.decompress (image.image)
    # copy to pixbuf
    pixbuf = gtk.gdk.pixbuf_new_from_data (imrgb.tostring(), gtk.gdk.COLORSPACE_RGB, False, 8, image.width, image.height, 3*image.width)
    # copy pixbuf to image
    widget.set_from_pixbuf (pixbuf)

