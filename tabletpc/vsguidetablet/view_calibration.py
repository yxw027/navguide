import cStringIO as StringIO
import struct
import random
import gtk
import pango
import math
from PIL import Image
import png

class view_calibration_t (object):

    def __init__(self, widget, width, height, bpp):
        self.width = width
        self.height = height
        self.bpp = bpp
        self.widget = widget
        self.pixmap = None
        self.colormap = None
        self.test_calibration = None
        self.cancel_box = None
        self.main_box = None
        self.test_display_box = None
        self.calibration_step = 0

    def is_in_box (self, x, y, box):
        return box and box[0] <= x and x <= box[0] + box[2] and box[1] <= y and y <= box[1] + box[3]

    def next_step (self):
        self.calibration_step = (self.calibration_step + 1) % 5

    def reset (self):
        self.calibration_step = 0

    def on_key_press (self, x, y):
        if self.is_in_box (x, y, self.test_calibration_box):
            return 0
        if self.is_in_box (x, y, self.cancel_box):
            return 1
        if self.is_in_box (x, y, self.main_box):
            return 2
        if self.is_in_box (x, y, self.test_display_box):
            return 3 
        return -1

    def render (self):
        # create a pixmap
        self.colormap = gtk.gdk.colormap_get_system()
        self.pixmap = gtk.gdk.Pixmap (None, self.width, self.height, self.bpp)
        pangoc = self.widget.get_pango_context ()

        # create a graphics context
        wgc = gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(65535,65535,65535)));
        rgc = gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(65535,0,0)));
        bgc = gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(0,0,0)));
        ggc = gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(0,65535,0)));
        lgc = gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(0,0,65535)));
        ygc = gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(65535,65535,0)));

        # white background
        self.pixmap.draw_rectangle (wgc, True, 0, 0, self.width, self.height)
      
        # font setup
        font = pango.FontDescription ("normal 20")
        pgl = pango.Layout (pangoc)
        pgl.set_font_description (font)
        pgl.set_alignment (pango.ALIGN_CENTER)

        # test calibration
        x = 20
        y = 300
        imrgb = Image.open ('gears.png')
        self.pixmap.draw_rgb_image (bgc, x, y, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
        self.test_calibration_box = [x, y, imrgb.size[0], imrgb.size[1]]

        # test display
        x = 20
        y = 20
        imrgb = Image.open ('screen.png')
        self.pixmap.draw_rgb_image (bgc, x, y, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
        self.test_display_box = [x, y, imrgb.size[0], imrgb.size[1]]

        # cancel
        x = self.width - 100
        y = 300
        imrgb = Image.open ('cancel.png')
        self.pixmap.draw_rgb_image (bgc, x, y, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
        self.cancel_box = [x, y, imrgb.size[0], imrgb.size[1]]

        # calibration instruction
        if self.calibration_step == 0:
            imrgb = Image.open ('start-calibration.png')
        if self.calibration_step == 1:
            imrgb = Image.open ('rotate-in-place.png')
        if self.calibration_step == 2:
            imrgb = Image.open ('walk-upstairs.png')
        if self.calibration_step == 3:
            imrgb = Image.open ('walk-downstairs.png')
        if self.calibration_step == 4:
            imrgb = Image.open ('calibration-done.png')
        x = self.width/2 - imrgb.size[0]/2
        y = 20
        self.pixmap.draw_rgb_image (bgc, x, y, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
        self.main_box = [x, y, imrgb.size[0], imrgb.size[1]]

        # set widget from pixbuf
        if self.widget:
            self.widget.set_from_pixmap (self.pixmap, None)

