import cStringIO as StringIO
import struct
import random
import gtk
import pango
import math
from PIL import Image
import png

class view_panel_t (object):

    def __init__(self, widget, width, height, bpp):
        self.width = width
        self.height = height
        self.bpp = bpp
        self.widget = widget
        self.pixmap = None
        self.colormap = None
        self.start_exploration_box = None
        self.end_exploration_box = None
        self.set_node_box = None
        self.go_to_node_box = None
        self.label_this_place_box = None
        self.load_map_box = None

    def is_in_box (self, x, y, box):
        return box and box[0] <= x and x <= box[0] + box[2] and box[1] <= y and y <= box[1] + box[3]

    def on_key_press (self, x, y):
        if self.is_in_box (x, y, self.start_exploration_box):
            return 0
        if self.is_in_box (x, y, self.end_exploration_box):
            return 1
        if self.is_in_box (x, y, self.set_node_box):
            return 2
        if self.is_in_box (x, y, self.go_to_node_box):
            return 3
        if self.is_in_box (x, y, self.label_this_place_box):
            return 4
        if self.is_in_box (x, y, self.load_map_box):
            return 5
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

        # start exploration
        x = 20
        y = 10
        imrgb = Image.open ('start-exploration.png')
        self.pixmap.draw_rgb_image (bgc, x, y, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
        self.start_exploration_box = [x, y, imrgb.size[0], imrgb.size[1]]
        x += imrgb.size[0] + 10

        # end exploration
        imrgb = Image.open ('end-exploration.png')
        self.pixmap.draw_rgb_image (bgc, x, y, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
        self.end_exploration_box = [x, y, imrgb.size[0], imrgb.size[1]]
        x += imrgb.size[0] + 10

        # goto node
        imrgb = Image.open ('set-current-node.png')
        self.pixmap.draw_rgb_image (bgc, x, y, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
        self.set_node_box = [x, y, imrgb.size[0], imrgb.size[1]]
        x += imrgb.size[0] + 10

        # goto node
        imrgb = Image.open ('go-to-node.png')
        self.pixmap.draw_rgb_image (bgc, x, y, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
        self.go_to_node_box = [x, y, imrgb.size[0], imrgb.size[1]]
        x += imrgb.size[0] + 10

        # label this place
        x = 20
        y += imrgb.size[1] + 30
        imrgb = Image.open ('label-this-place.png')
        self.pixmap.draw_rgb_image (bgc, x, y, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
        self.label_this_place_box = [x, y, imrgb.size[0], imrgb.size[1]]
        x += imrgb.size[0] + 10

        # load map
        imrgb = Image.open ('load-map.png')
        self.pixmap.draw_rgb_image (bgc, x, y, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
        self.load_map_box = [x, y, imrgb.size[0], imrgb.size[1]]
        x += imrgb.size[0] + 10

        # set widget from pixbuf
        if self.widget:
            self.widget.set_from_pixmap (self.pixmap, None)

