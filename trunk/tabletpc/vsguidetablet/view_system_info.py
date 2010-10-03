import cStringIO as StringIO
import struct
import random
import gtk
import pango
import math
from PIL import Image
from system_info_t import system_info_t
from logger_info_t import logger_info_t
import png
import time

class view_system_info_t (object):

    def __init__(self, widget, width, height, bpp):
        self.width = width
        self.height = height
        self.bpp = bpp
        self.data = None  # a system_info_t structure
        self.logger_info = None
        self.widget = widget
        self.pixmap = None
        self.colormap = None
        self.sound_box = None # bounding box for various buttons
        self.startup_box = None
        self.shutdown_box = None
        self.record_box = None
        self.image_announce = {}
        self.features_announce = {}

    def init_random (self):
        random.seed ()
        self.data = system_info_t ()
        self.data.temp_critical = 70
        self.data.temp_cpu = 45
        self.data.battery_discharge_rate = 1200
        self.data.battery_remaining_mA = 2000
        self.data.battery_design_capacity_mA = 3000
        self.data.cpu_user = 64
        self.data.cpu_user_low = 0
        self.data.cpu_system = 10
        self.data.cpu_idle = 20
        self.data.memtotal = 2000000000
        self.data.memfree = 800000000
        self.data.swaptotal = 3000000000
        self.data.swapfree = 2800000000
        self.data.mem_usage = .22
        self.data.swap_usage = .01
        self.data.cpu_usage = .79
        self.data.disk_space_remaining_gb = 7.2
        self.data.sound_enabled = 0

        self.logger_info = logger_info_t ()
        self.logger_info.logfilename = "lcmlog-2009-10-14.10-12-24"
        self.logger_info.logfilesize_mb = 1280.4
        self.logger_info.logfilerate_mbs = .32
        self.logger_info.logging = 1

    def ratio_to_color (self, ratio):
        if ratio < .5:
            return gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(0,65535,0)))
        if ratio < .75:
            return gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(65535,32767,0)))
        return gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(65535,0,0)))
        
    def ratio_to_color_inverse (self, ratio):
        if ratio < .25:
            return gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(65535,0,0)))
        if ratio < .50:
            return gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(65535,32767,0)))
        return gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(0,65535,0)))
       
    def is_in_box (self, x, y, box):
        return box and box[0] <= x and x <= box[0] + box[2] and box[1] <= y and y <= box[1] + box[3]

    def on_key_press (self, x, y):
        if self.is_in_box (x, y, self.sound_box):
            return 0
        if self.is_in_box (x, y, self.shutdown_box):
            return 1
        if self.is_in_box (x, y, self.startup_box):
            return 2
        if self.is_in_box (x, y, self.record_box):
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

        # white background
        self.pixmap.draw_rectangle (wgc, True, 0, 0, self.width, self.height)
       
        if self.data is None:
            return

        x = 20
        y = 20
        boxw = (self.width - 2 * x) / 7
        boxh = self.height / 4

        font = pango.FontDescription ("normal 20")
        pgl = pango.Layout (pangoc)
        pgl.set_font_description (font)
        pgl.set_alignment (pango.ALIGN_CENTER)

        # cpu
        ratio = 1.0 * self.data.cpu_usage
        bc = int(ratio * boxh)
        yc = y + boxh - bc
        self.pixmap.draw_rectangle (self.ratio_to_color(ratio), True, x, yc, boxw, bc)
        self.pixmap.draw_rectangle (bgc, False, x, y, boxw, boxh)
        pgl.set_text ("CPU")
        self.pixmap.draw_layout (bgc, x + 20, y + boxh, pgl)
        pgl.set_text ("%d %%" % int(ratio * 100))
        self.pixmap.draw_layout (bgc, x + 20, y + boxh/2, pgl)

        # temperature
        x += int(1.2*boxw)
        ratio = 1.0 * self.data.temp_cpu / self.data.temp_critical
        bc = int (ratio * boxh)
        yc = y + boxh - bc
        self.pixmap.draw_rectangle (self.ratio_to_color (ratio), True, x, yc, boxw, bc)
        self.pixmap.draw_rectangle (bgc, False, x, y, boxw, boxh)
        pgl.set_text ("TEMP")
        self.pixmap.draw_layout (bgc, x + 20, y + boxh, pgl)
        pgl.set_text ("%d C" % int(self.data.temp_cpu))
        self.pixmap.draw_layout (bgc, x + 20, y + boxh/2, pgl)

        # memory
        x += int(1.2*boxw)
        ratio = 1.0 * self.data.mem_usage
        bc = int (ratio * boxh)
        yc = y + boxh - bc
        self.pixmap.draw_rectangle (self.ratio_to_color (ratio), True, x, yc, boxw, bc)
        self.pixmap.draw_rectangle (bgc, False, x, y, boxw, boxh)
        pgl.set_text ("MEM")
        self.pixmap.draw_layout (bgc, x + 20, y + boxh, pgl)
        pgl.set_text ("%d %%" % int(100*self.data.mem_usage))
        self.pixmap.draw_layout (bgc, x + 20, y + boxh/2, pgl)
        
        # swap
        x += int(1.2*boxw)
        ratio = 1.0 * self.data.swap_usage
        bc = int (ratio * boxh)
        yc = y + boxh - bc
        self.pixmap.draw_rectangle (self.ratio_to_color (ratio), True, x, yc, boxw, bc)
        self.pixmap.draw_rectangle (bgc, False, x, y, boxw, boxh)
        pgl.set_text ("SWAP")
        self.pixmap.draw_layout (bgc, x + 10, y + boxh, pgl)
        pgl.set_text ("%d %%" % int(100.0 * self.data.swap_usage))
        self.pixmap.draw_layout (bgc, x + 20, y + boxh/2, pgl)
        
        # battery
        x += int(1.2*boxw)
        ratio = 1.0 * self.data.battery_remaining_mA / self.data.battery_design_capacity_mA
        bc = int (ratio * boxh)
        yc = y + boxh - bc
        self.pixmap.draw_rectangle (self.ratio_to_color_inverse (ratio), True, x, yc, boxw, bc)
        self.pixmap.draw_rectangle (bgc, False, x, y, boxw, boxh)
        pgl.set_text ("BATT.")
        self.pixmap.draw_layout (bgc, x + 0, y + boxh, pgl)
        pgl.set_text ("%d %%" % int(ratio * 100))
        self.pixmap.draw_layout (bgc, x + 20, y + boxh/2, pgl)
        
        # disk space
        disk_space_crit_gb = 3.0
        x += int(1.2*boxw)
        ratio = 1.0 * max (0, disk_space_crit_gb - self.data.disk_space_remaining_gb) / disk_space_crit_gb;
        bc = int (ratio * boxh)
        yc = y + boxh - bc
        self.pixmap.draw_rectangle (self.ratio_to_color (ratio), True, x, yc, boxw, bc)
        self.pixmap.draw_rectangle (bgc, False, x, y, boxw, boxh)
        pgl.set_text ("DISK")
        self.pixmap.draw_layout (bgc, x + 10, y + boxh, pgl)
        pgl.set_text ("%.1f GB" % self.data.disk_space_remaining_gb)
        self.pixmap.draw_layout (bgc, x + 20, y + boxh/2, pgl)
      
        boxw = 2 * boxw
        y += int(1.4 * boxh)
        x = 20
        max_hz = 5.0
        boxh = self.height / 6

        t = time.time ()

        # video hz
        self.pixmap.draw_rectangle (bgc, False, x, y, boxw, boxh)
        nsensors = len (self.image_announce)
        for k,v in self.image_announce.iteritems ():
            wh = int(1.0*boxw/nsensors)
            xc = x + k * wh
            yh = int (1.0 * (5.0 - min(t-v,5.0))/5.0 * boxh)
            print "image announce %d: t = %f, now = %f, delta = %f yh = %d" % (k, v, t, t-v, yh)
            yc = y + boxh - yh
            self.pixmap.draw_rectangle (lgc, True, xc, yc, wh, yh)
            self.pixmap.draw_rectangle (bgc, False, xc, yc, wh, yh)
        pgl.set_text ("Video")
        self.pixmap.draw_layout (bgc, x + 20, y + boxh, pgl)
       
        # features hz
        x += int(1.2 * boxw)
        self.pixmap.draw_rectangle (bgc, False, x, y, boxw, boxh)
        for k,v in self.features_announce.iteritems():
            wh = int(1.0*boxw/nsensors)
            xc = x + k * wh
            yh = int (1.0 * (5.0 - min(t-v,5))/5.0 * boxh)
            yc = y + boxh - yh
            self.pixmap.draw_rectangle (lgc, True, xc, yc, wh, yh)
            self.pixmap.draw_rectangle (bgc, False, xc, yc, wh, yh)
        pgl.set_text ("Features")
        self.pixmap.draw_layout (bgc, x + 20, y + boxh, pgl)

        # log file info
        if self.logger_info and self.logger_info.logging:
            x += int(1.2 * boxw)
            imrgb = Image.open ('database.png')
            self.pixmap.draw_rgb_image (bgc, x, y, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
            font = pango.FontDescription ("normal 12")
            pgl.set_font_description (font)
            pgl.set_text ("%.1f MB" % self.logger_info.logfilesize_mb)
            self.pixmap.draw_layout (bgc, x, y + boxh + 10, pgl);

            x += int (1.6 * imrgb.size[0])
            boxw = 80
            max_rate_mbs = 5.0
            ratio = min (1.0, 1.0 * self.logger_info.logfilerate_mbs / max_rate_mbs)
            bc = int (ratio * boxh)
            yc = y + boxh - bc
            self.pixmap.draw_rectangle (self.ratio_to_color_inverse (ratio), True, x, yc, boxw, bc)
            self.pixmap.draw_rectangle (bgc, False, x, y, boxw, boxh)
            pgl.set_text ("%.1f MB/s" % self.logger_info.logfilerate_mbs)
            self.pixmap.draw_layout (bgc, x, y + boxh + 10, pgl);


        y += int (1.6 * boxh)
        x = 20

        # sound icon
        if self.data.sound_enabled:
            imrgb = Image.open ('sound-yes.png')
        else:
            imrgb = Image.open ('sound-no.png')
        self.pixmap.draw_rgb_image (bgc, x+2, y+2, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
        self.sound_box = [x+2, y+2, imrgb.size[0], imrgb.size[1]]

        # shutdown and startup icon
        x += int (1.2 * imrgb.size[0])
        imrgb = Image.open ('shutdown.png')
        self.pixmap.draw_rgb_image (bgc, x+2, y+2, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
        self.shutdown_box = [x+2, y+2, imrgb.size[0], imrgb.size[1]]

        x += int (1.2 * imrgb.size[0])
        imrgb = Image.open ('startup.png')
        self.pixmap.draw_rgb_image (bgc, x+2, y+2, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
        self.startup_box = [x+2, y+2, imrgb.size[0], imrgb.size[1]]

        # logging icon
        x += int (1.2 * imrgb.size[0])
        if self.logger_info:
            if self.logger_info.logging:
                imrgb = Image.open ('record_stop.png')
            else:
                imrgb = Image.open ('record.png')
            self.pixmap.draw_rgb_image (bgc, x+2, y+2, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
            self.record_box = [x+2, y+2, imrgb.size[0], imrgb.size[1]]
        
        # set widget from pixbuf
        if self.widget:
            self.widget.set_from_pixmap (self.pixmap, None)


    # sample code: here is how to read pixels from a png image file using the 
    # png module, but don't use this on the tablet because it's damn slow!
        #f = open ('shutdown.png', 'r')
        #r = png.Reader (f)
        #(width, height, data, meta) = r.read_flat()
        #self.pixmap.draw_rgb_image (bgc, x+2, y+2, width, height, gtk.gdk.RGB_DITHER_NONE, data)
        #f.close ()
    
