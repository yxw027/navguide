import cStringIO as StringIO
import struct
import random
import gtk
import pango
import math
from PIL import Image
from class_param_t import class_param_t
from image_t import image_t
import png

class view_nav_info_t (object):

    def __init__(self, widget, width, height, bpp):
        self.width = width
        self.height = height
        self.bpp = bpp
        self.data = None  # a class_param_t structure
        self.widget = widget
        self.pixmap = None
        self.colormap = None
        self.left_img = None
        self.right_img = None
        self.user_lost_box = None
        self.unclear_guidance_box = None
        self.future_direction = [-1, -1, -1]

    def init_image_from_file (self, filename):
        im = Image.open (filename)
        if im:
            img = image_t ()
            img.width = im.size[0]
            img.height = im.size[1]
            img.data = im.tostring ()
            img.row_stride = 3 * img.width
            img.pixelformat = image_t.PIXEL_FORMAT_RGB
            img.size = len (img.data)
            return img
        return None

    def to_hours_mins_secs (self, secs):
        hours = int (secs / 3600)
        secs -= 3600 * hours
        mins = int (secs / 60)
        secs -= 60  * mins
        return (hours, mins, secs)

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

    def init_random (self):
        self.data = class_param_t ()
        self.left_img = self.init_image_from_file ('left_img_sample.png')
        self.right_img = self.init_image_from_file ('right_img_sample.png')
        self.data.utime = 0
        self.data.mode = 0
        self.data.orientation = [0.2, 1.]
        self.data.progress = .64
        self.data.translation_speed = 1.2
        self.data.rotation_speed = 0.3
        self.data.nodeid_now = 25
        self.data.nodeid_next = 26
        self.data.nodeid_end = 129
        self.data.path_size = 10
        self.data.path = [25, 26, 27, 123, 124, 125, 126, 127, 128, 129]
        self.data.confidence = .78
        self.data.eta_secs = 70
        self.data.eta_meters = 95
        self.data.psi_distance = 0
        self.data.psi_distance_thresh = 0
        self.data.map_filename = "2009-08-31.00.map"
        self.data.pdf_size = 0
        self.data.pdfind = None
        self.data.pdfval = None
        self.data.next_direction = class_param_t.MOTION_TYPE_RIGHT
        self.data.next_direction_meters = 20
        self.data.mission_success = 0
        self.data.wrong_way = 0
        self.future_direction[0] = class_param_t.MOTION_TYPE_RIGHT
        self.future_direction[1] = class_param_t.MOTION_TYPE_FORWARD
        self.future_direction[2] = class_param_t.MOTION_TYPE_UP
        
    def is_in_box (self, x, y, box):
        return box and box[0] <= x and x <= box[0] + box[2] and box[1] <= y and y <= box[1] + box[3]


    def on_key_press (self, x, y):
        if self.is_in_box (x, y, self.unclear_guidance_box):
            return 0
        if self.is_in_box (x, y, self.user_lost_box):
            return 1
        return -1

    def render_compass (self, angle_rad, x, y, width, height, pixmap, bgc, rgc):
        # circle
        pixmap.draw_arc (bgc, False, x+5, y+5, width-10, height-10, 0, 360 * 64)
        ccx = x + width/2
        ccy = y + height/2
        ccrx = (width-10)/2
        ccry = (height-10)/2
    
        # ticks
        for i in range(0,360, 15):
            x2 = int(ccx + ccrx * math.cos (math.radians (i)))
            y2 = int(ccy - ccry * math.sin (math.radians (i)))
            x1 = int(ccx + .9 * ccrx * math.cos (math.radians (i)))
            y1 = int(ccy - .9 * ccry * math.sin (math.radians (i)))
            pixmap.draw_line (bgc, x1, y1, x2, y2)
    
        # draw arrow 2
        x01 = int(ccx + ccrx * math.cos (angle_rad[0]+math.pi/2))
        y01 = int(ccy - ccry * math.sin (angle_rad[0]+math.pi/2))
        x02 = int(ccx - ccrx/7 * math.sin (angle_rad[0]+math.pi/2))
        y02 = int(ccy - ccry/7 * math.cos (angle_rad[0]+math.pi/2))
        x03 = int(ccx + ccrx/7 * math.sin (angle_rad[0]+math.pi/2))
        y03 = int(ccy + ccry/7 * math.cos (angle_rad[0]+math.pi/2))
        x11 = int(ccx + ccrx * math.cos (angle_rad[1]+math.pi/2))
        y11 = int(ccy - ccry * math.sin (angle_rad[1]+math.pi/2))
        x12 = int(ccx - ccrx/7 * math.sin (angle_rad[1]+math.pi/2))
        y12 = int(ccy - ccry/7 * math.cos (angle_rad[1]+math.pi/2))
        x13 = int(ccx + ccrx/7 * math.sin (angle_rad[1]+math.pi/2))
        y13 = int(ccy + ccry/7 * math.cos (angle_rad[1]+math.pi/2))
        #pixmap.draw_polygon (rgc, False, [(x11, y11), (x12, y12), (x13, y13)])
        pixmap.draw_polygon (rgc, True, [(x01, y01), (x02, y02), (x03, y03)])

    def render_next_direction (self, directions, x, y, width, height, pixmap,
            bgc, rgc, pgl):
        boxw = width - 10
        boxh = height
        boxx = x + 5
        boxy = y + 5
       
    #    pixmap.draw_rectangle (bgc, False, boxx, boxy, boxw, boxh)

        if self.data.mission_success:
            imrgb = Image.open ('icon-mission-success-100x100.png')
            pixmap.draw_rgb_image (bgc, boxx+boxw/2-50, boxy+boxh/2-imrgb.size[1]/2, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
            return
        if self.data.wrong_way:
            imrgb = Image.open ('icon-wrong-way-100x100.png')
            pixmap.draw_rgb_image (bgc, boxx+boxw/2-50, boxy+boxh/2-imrgb.size[1]/2 , imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
            return
        
        boxx -= 5

        for idx in range (0,3):
            next_dir = directions[idx];
            if next_dir is not -1:
                if next_dir == class_param_t.MOTION_TYPE_FORWARD:
                    imrgb = Image.open ('icon-straight-100x100.png')
                    pixmap.draw_rgb_image (bgc, boxx + 5, boxy + boxh/2-imrgb.size[1]/2, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
                if next_dir == class_param_t.MOTION_TYPE_LEFT:
                    imrgb = Image.open ('icon-left-turn-100x100.png')
                    pixmap.draw_rgb_image (bgc, boxx + 5, boxy + boxh/2-imrgb.size[1]/2, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
                if next_dir == class_param_t.MOTION_TYPE_RIGHT:
                    imrgb = Image.open ('icon-right-turn-100x100.png')
                    pixmap.draw_rgb_image (bgc, boxx + 5, boxy + boxh/2-imrgb.size[1]/2, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
                if next_dir == class_param_t.MOTION_TYPE_UP:
                    imrgb = Image.open ('icon-up-100x100.png')
                    pixmap.draw_rgb_image (bgc, boxx + 5, boxy + boxh/2-imrgb.size[1]/2, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
                if next_dir == class_param_t.MOTION_TYPE_DOWN:
                    imrgb = Image.open ('icon-down-100x100.png')
                    pixmap.draw_rgb_image (bgc, boxx + 5, boxy + boxh/2-imrgb.size[1]/2, imrgb.size[0], imrgb.size[1], gtk.gdk.RGB_DITHER_NONE, imrgb.tostring())
            boxx += 85;

        pixmap.draw_rectangle (rgc, False, x, boxy+5, 90, 90)

    def render_buttons (self, x, y, pixmap, bgc, rgc, ogc, ugc, wgc, pgl):
        buttonwidth = 240
        buttonheight = 80
        # new button
        pixmap.draw_rectangle (ugc, True, x+10, y+10, buttonwidth, buttonheight)
        pixmap.draw_rectangle (wgc, True, x, y, buttonwidth, buttonheight)
        pixmap.draw_rectangle (bgc, False, x, y, buttonwidth, buttonheight)
        pgl.set_text ("Unclear guidance")
        textwidth = pgl.get_pixel_size()[0]
        textheight = pgl.get_pixel_size()[1]
        self.pixmap.draw_layout (bgc, x + buttonwidth/2 - textwidth/2, y + buttonheight/2 - textheight/2, pgl)
        self.unclear_guidance_box = [x, y, buttonwidth, buttonheight]
        # new button
        x += 2 * buttonwidth + 45
        pixmap.draw_rectangle (ugc, True, x+10, y+10, buttonwidth, buttonheight)
        pixmap.draw_rectangle (wgc, True, x, y, buttonwidth, buttonheight)
        pixmap.draw_rectangle (bgc, False, x, y, buttonwidth, buttonheight)
        self.user_lost_box = [x, y, buttonwidth, buttonheight]
        pgl.set_text ("User lost")
        textwidth = pgl.get_pixel_size()[0]
        textheight = pgl.get_pixel_size()[1]
        self.pixmap.draw_layout (bgc, x + buttonwidth/2 - textwidth/2, y + buttonheight/2 - textheight/2, pgl)
        
    def render (self):
        # create a pixmap
        self.colormap = gtk.gdk.colormap_get_system()
        self.pixmap = gtk.gdk.Pixmap (None, self.width, self.height, self.bpp)
        pangoc = self.widget.get_pango_context ()

        # create a graphics context
        wgc = gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(65535,65535,65535)));   # white
        ugc = gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(15535,15535,15535)));   # grey
        rgc = gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(65535,0,0)));           # red
        bgc = gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(0,0,0)));               # black
        ggc = gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(0,65535,0)));           # green
        lgc = gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(0,0,65535)));           # blue
        ygc = gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(65535,65535,0)));       # yellow
        ogc = gtk.gdk.Drawable.new_gc (self.pixmap, self.colormap.alloc_color(gtk.gdk.Color(65535,32767,0)));       # orange

        # white background
        self.pixmap.draw_rectangle (wgc, True, 0, 0, self.width, self.height)
       
        if self.data is None:
            return

        font = pango.FontDescription ("normal 20")
        pgl = pango.Layout (pangoc)
        pgl.set_font_description (font)
        pgl.set_alignment (pango.ALIGN_CENTER)

        # images
        x = 10
        y = 5
        boxh = self.height / 3
        if self.left_img:
            print "drawing image %d x %d (size: %d)" % (self.left_img.width, self.left_img.height, self.left_img.size)
            self.pixmap.draw_rgb_image (bgc, x, y, self.left_img.width, self.left_img.height, gtk.gdk.RGB_DITHER_NONE, self.left_img.data)
            x += self.left_img.width + 10
            boxh = self.left_img.height - 30
        if self.right_img:
            self.pixmap.draw_rgb_image (bgc, x, y, self.right_img.width, self.right_img.height, gtk.gdk.RGB_DITHER_NONE, self.right_img.data)
            x += self.right_img.width + 10
            boxh = self.left_img.height - 30
           
        # confidence level
        boxw = self.width - 10 - x
        step = boxh / 10
        lineh = int (step / 1.8)
        for i in range (0, 10):
            linex = x + 5
            liney = y + step * i + 5
            linew = int (20 + (boxw-20) * (9 - i) * 1.0 / 9 * .9)
            self.pixmap.draw_rectangle (self.ratio_to_color_inverse (self.data.confidence), 9 - i < 10 * self.data.confidence, linex, liney, linew, lineh)
            self.pixmap.draw_rectangle (bgc, False, linex, liney, linew, lineh)
        pgl.set_text ("%d %%" % int (self.data.confidence*100.0))
        self.pixmap.draw_layout (bgc, x+boxw-80, y + int (boxh*.6), pgl);
        pgl.set_text ("confidence")
        self.pixmap.draw_layout (bgc, x + 15, y + boxh, pgl);
 #       self.pixmap.draw_rectangle (bgc, False, x, y, boxw, boxh + 30)
        y = y + boxh + 40

        boxw = (self.width-40)/3
        boxh = self.height - y - 10
        x = 10
       
        # box: information (ETA, etc.)
        #self.pixmap.draw_rectangle (bgc, False, x, y, boxw, boxh)
        self.pixmap.draw_rectangle (bgc, True, x+5, y+5, 80, 40)
        pgl.set_text ("ETA");
        self.pixmap.draw_layout (wgc, x + 10, y + 10, pgl);
        self.pixmap.draw_rectangle (ygc, True, x+5+80, y+5, boxw-85, 40)
        pgl.set_text ("%02d:%02d:%02d" % self.to_hours_mins_secs (self.data.eta_secs))
        self.pixmap.draw_layout (bgc, x + 90, y + 10, pgl);

        self.pixmap.draw_rectangle (bgc, True, x+5, y+5+40, 80, 40)
        pgl.set_text ("DIST");
        self.pixmap.draw_layout (wgc, x + 10, y + 10 + 40, pgl);
        self.pixmap.draw_rectangle (ygc, True, x+5+80, y+5+40, boxw-85, 40)
        pgl.set_text ("%d m." % int (self.data.eta_meters))
        self.pixmap.draw_layout (bgc, x + 90, y + 10 + 40, pgl);

        # render buttons
        self.render_buttons (x + 5, y + 70 + pgl.get_pixel_size()[1] + 10, self.pixmap, bgc, rgc, ogc, ugc, wgc, pgl)
        
        x += boxw + 10


        # compass
#        self.pixmap.draw_rectangle (bgc, False, x, y, boxw, boxh)
        self.render_compass (self.data.orientation, x, y, boxw, boxh, self.pixmap, bgc, rgc)
        x += boxw + 10

        # roadmap
        font = pango.FontDescription ("normal 14")
        pgl.set_font_description (font)
        self.render_next_direction (self.future_direction, x, y, boxw, 100,
                self.pixmap, bgc, rgc, pgl)
        x += boxw + 10

        # set widget from pixbuf
        if self.widget:
            self.widget.set_from_pixmap (self.pixmap, None)

