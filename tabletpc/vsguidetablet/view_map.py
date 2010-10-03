import cStringIO as StringIO
import struct
import random
import gtk
import pango
import math
from PIL import Image
from ui_map_t import ui_map_t
from ui_map_node_t import ui_map_node_t
from ui_map_edge_t import ui_map_edge_t
from class_param_t import class_param_t

class view_map_t(object):

    def __init__(self, widget, width, height, widget_scale, width_scale, height_scale, node_radius, bpp):
        self.width = width
        self.height = height
        self.xo = 0.5
        self.yo = 0.5
        self.node_radius = node_radius
        self.scaling =.7 
        self.widget = widget
        self.widget_scale = widget_scale
        self.width_scale = width_scale
        self.height_scale = height_scale
        self.bpp = bpp
        self.label_enabled = False
        self.fancy_enabled = False
        self.data = None  # a ui_map_t structure
        self.class_param = None # a class_param_t structure
        self.hash = {}
        self.path = []
        self.dragging = False
        self.drag_xstart = 0
        self.drag_ystart = 0

    def zoom_in (self):
        if self.scaling < 11:
            self.scaling = self.scaling + 1
        self.render_all ()

    def zoom_out (self):
        if self.scaling > 1:
            self.scaling = self.scaling - 1
        self.render_all ()

    def drag_start (self, x, y):
        self.dragging = True
        self.drag_xstart = x
        self.drag_ystart = y
        
    def drag_stop (self, x, y):
        self.dragging = False
        dx = - (x - self.drag_xstart) / self.width
        dy = - (y - self.drag_ystart) / self.height
        oldx = self.xo
        oldy = self.yo
        self.xo = self.xo + dx / self.scaling
        self.yo = self.yo + dy / self.scaling
        if self.xo < 0 or self.xo > 1.0 or self.yo < 0 or self.yo > 1.0:
            self.xo = oldx
            self.yo = oldy
        self.render_all ()

    def init_random (self):
        random.seed ()
        n_nodes = 10
        self.data = ui_map_t ()
        self.data.n_nodes = 0
        self.data.nodes = []
        self.data.n_edges = 0
        self.data.edges = []

        for i in range(0,n_nodes):
            nd = ui_map_node_t ()
            nd.x = random.random ()
            nd.y = random.random ()
            nd.label = "node %d" % i
            nd.id = i
            
            self.data.nodes.append (nd)
            self.data.n_nodes += 1

        for i in range (0, n_nodes-1):
            ed = ui_map_edge_t ()
            ed.motion_type = int (random.random()*5)-1
            ed.id1 = i
            ed.id2 = i+1

            self.data.edges.append (ed)
            self.data.n_edges += 1

        self.class_param = class_param_t ()
        self.class_param.nodeid_now = n_nodes/5
        self.class_param.nodeid_end = 4*n_nodes/5

        self.update_hash ()

        self.path = []
        for i in range (self.class_param.nodeid_now, self.class_param.nodeid_end+1):
            self.path.append (i)

    def update_hash (self):
        self.hash = {}
        if self.data is None:
            return
        icount=0
        for it in self.data.nodes:
            self.hash[it.id] = icount
            icount = icount + 1

    def find_node_by_id (self, index):
        if index < 0 or not self.hash.has_key (index):
            return None
        else:
            return self.data.nodes[self.hash[index]]

    def toggle_label (self, status):
        self.label_enabled = status
        self.render_all ()

    def toggle_fancy (self, status):
        self.fancy_enabled = status
        self.render_all ()

    def center_start (self):
        if self.class_param is None:
            return
        el = self.find_node_by_id (self.class_param.nodeid_now)
        if el is not None:
            self.xo = el.x
            self.yo = el.y
        self.render_all ()

    def center_end (self):
        if self.class_param is None:
            return
        el = self.find_node_by_id (self.class_param.nodeid_end)
        if el is not None:
            self.xo = el.x
            self.yo = el.y
        self.render_all ()

    def move (self, direction):
        oldx = self.xo
        oldy = self.yo
        d = .25/self.scaling
        if direction == 0:
            self.xo = self.xo + d
            self.yo = self.yo
        if direction == 1:
            self.xo = self.xo + d
            self.yo = self.yo - d
        if direction == 2:
            self.xo = self.xo
            self.yo = self.yo - d
        if direction == 3:
            self.xo = self.xo - d
            self.yo = self.yo - d
        if direction == 4:
            self.xo = self.xo - d
            self.yo = self.yo
        if direction == 5:
            self.xo = self.xo - d
            self.yo = self.yo + d
        if direction == 6:
            self.xo = self.xo
            self.yo = self.yo + d
        if direction == 7:
            self.xo = self.xo + d
            self.yo = self.yo + d
        if self.xo < 0 or self.xo > 1.0 or self.yo < 0 or self.yo > 1.0:
            self.xo = oldx
            self.yo = oldy
        self.render_all ()

    def list_nodes_with_label (self):
        nodes = {}
        if self.data:
            for node in self.data.nodes: 
                if len (node.label) > 2:
                    nodes[node.id] = node.label
        return nodes

    def min_max_nodeid (self):
        min_nodeid = -1
        max_nodeid = -1
        if self.data:
            for node in self.data.nodes:
                if min_nodeid == -1 or node.id < min_nodeid:
                    min_nodeid = node.id
                if max_nodeid == -1 or node.id > max_nodeid:
                    max_nodeid = node.id
        return (min_nodeid, max_nodeid)

    def is_visible (self, x, y):
        return x >= 0 and x < self.width and y >= 0 and y < self.height

    def find_node_by_coords (self, x, y):
        rad = int( (self.node_radius))
        for el in self.data.nodes:
            nx = int ((el.x - self.xo) * self.scaling * self.width) + self.width/2
            ny = int ((el.y - self.yo) * self.scaling * self.height) + self.height/2
            if abs (nx-x) < rad and abs (ny-y) < rad:
                return el
        return None

    def render (self):
        # create a pixmap
        colormap = gtk.gdk.colormap_get_system()
        pixmap = gtk.gdk.Pixmap (None, self.width, self.height, self.bpp)
        pangoc = self.widget.get_pango_context ()

        # create a graphics context
        wgc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(65535,65535,65535))); # white
        rgc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(65535,0,0))); # red
        bgc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(0,0,0)));     # black
        ggc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(0,65535,0))); # green
        lgc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(0,0,65535))); # blue
        ygc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(65535,65535,0))); # yellow
        egc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(0,65535,65535))); # emeraude

        # white background
        pixmap.draw_rectangle (wgc, True, 0, 0, self.width, self.height)

        if self.data is None:
            return
       
        # draw edges
        for el in self.data.edges:
            if el.reverse:
                continue
            nd1 = self.find_node_by_id (el.id1)
            nd2 = self.find_node_by_id (el.id2)
            if nd1 is not None and nd2 is not None:
                x1 = int ((nd1.x - self.xo) * self.scaling * self.width) + self.width/2
                y1 = int ((nd1.y - self.yo) * self.scaling * self.height) + self.height/2
                x2 = int ((nd2.x - self.xo) * self.scaling * self.width) + self.width/2
                y2 = int ((nd2.y - self.yo) * self.scaling * self.height) + self.height/2
                if self.fancy_enabled:
                    bgc.set_line_attributes (3, gtk.gdk.LINE_SOLID, gtk.gdk.CAP_BUTT, gtk.gdk.JOIN_MITER);
                    rgc.set_line_attributes (3, gtk.gdk.LINE_SOLID, gtk.gdk.CAP_BUTT, gtk.gdk.JOIN_MITER);
                    ggc.set_line_attributes (3, gtk.gdk.LINE_SOLID, gtk.gdk.CAP_BUTT, gtk.gdk.JOIN_MITER);
                    ygc.set_line_attributes (3, gtk.gdk.LINE_SOLID, gtk.gdk.CAP_BUTT, gtk.gdk.JOIN_MITER);
                    egc.set_line_attributes (3, gtk.gdk.LINE_SOLID, gtk.gdk.CAP_BUTT, gtk.gdk.JOIN_MITER);

                    if el.motion_type == class_param_t.MOTION_TYPE_LEFT:
                        pixmap.draw_line (rgc, x1, y1, x2, y2)
                    if el.motion_type == class_param_t.MOTION_TYPE_RIGHT:
                        pixmap.draw_line (ggc, x1, y1, x2, y2)
                    if el.motion_type == class_param_t.MOTION_TYPE_UP:
                        pixmap.draw_line (ygc, x1, y1, x2, y2)
                    if el.motion_type == class_param_t.MOTION_TYPE_DOWN:
                        pixmap.draw_line (egc, x1, y1, x2, y2)
                    if el.motion_type == class_param_t.MOTION_TYPE_FORWARD:
                        pixmap.draw_line (bgc, x1, y1, x2, y2)
                    if el.motion_type == -1:
                        pixmap.draw_line (bgc, x1, y1, x2, y2)
                    
                else:
                    pixmap.draw_line (bgc, x1, y1, x2, y2)

        bgc.set_line_attributes (1, gtk.gdk.LINE_SOLID, gtk.gdk.CAP_BUTT, gtk.gdk.JOIN_MITER);
        rgc.set_line_attributes (1, gtk.gdk.LINE_SOLID, gtk.gdk.CAP_BUTT, gtk.gdk.JOIN_MITER);
        ggc.set_line_attributes (1, gtk.gdk.LINE_SOLID, gtk.gdk.CAP_BUTT, gtk.gdk.JOIN_MITER);
        ygc.set_line_attributes (1, gtk.gdk.LINE_SOLID, gtk.gdk.CAP_BUTT, gtk.gdk.JOIN_MITER);
        egc.set_line_attributes (1, gtk.gdk.LINE_SOLID, gtk.gdk.CAP_BUTT, gtk.gdk.JOIN_MITER);

        # draw nodes
        radius = int( (self.node_radius))
        for el in self.data.nodes:
            rad = radius
            if not self.label_enabled or len (el.label) < 2:
                rad = rad/2
            x = int ((el.x - self.xo) * self.scaling * self.width) + self.width/2
            y = int ((el.y - self.yo) * self.scaling * self.height) + self.height/2
            if self.is_visible (x,y):
                pixmap.draw_arc (bgc, False, x - rad, y - rad, 2 * rad, 2 * rad, 0, 360 * 64)
                if self.class_param and el.id == self.class_param.nodeid_now:
                    pixmap.draw_arc (lgc, True, x - rad, y - rad, 2 * rad, 2 * rad, 0, 360 * 64)
                if self.class_param and el.id == self.class_param.nodeid_end:
                    pixmap.draw_arc (rgc, True, x - rad, y - rad, 2 * rad, 2 * rad, 0, 360 * 64)
                
            # display label
            if self.label_enabled and self.is_visible (x, y):
                pgl = pango.Layout (pangoc)
                pgl.set_text (el.label)
                pixmap.draw_layout (bgc, x + int (1.5 * rad), y - int(1.5* rad), pgl)

        # draw path
        bgc.set_line_attributes (3, gtk.gdk.LINE_SOLID, gtk.gdk.CAP_BUTT, gtk.gdk.JOIN_MITER);
        prev = -1
        for el in self.path:
            v1 = self.find_node_by_id (prev)
            v2 = self.find_node_by_id (el)
            if v1 and v2:
                x1 = int ((v1.x - self.xo) * self.scaling * self.width) + self.width/2
                y1 = int ((v1.y - self.yo) * self.scaling * self.height) + self.height/2
                x2 = int ((v2.x - self.xo) * self.scaling * self.width) + self.width/2
                y2 = int ((v2.y - self.yo) * self.scaling * self.height) + self.height/2
                pixmap.draw_line (bgc, x1, y1, x2, y2)
            prev = el

        # set widget from pixbuf
        if self.widget:
            self.widget.set_from_pixmap (pixmap, None)

    def render_scale (self):
        colormap = gtk.gdk.colormap_get_system()
        pixmap = gtk.gdk.Pixmap (None, self.width_scale, self.height_scale, self.bpp)
        # create a graphics context
        wgc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(65535,65535,65535)));
        lgc = gtk.gdk.Drawable.new_gc (pixmap, colormap.alloc_color(gtk.gdk.Color(0,0,65535)));
        pixmap.draw_rectangle (wgc, True, 0, 0, self.width_scale, self.height_scale)
        # draw boxes
        for i in range (0,11):
            x1 = int(1.0*self.width_scale/5)
            ww = int(3.0*self.width_scale/5)
            y1 = int(self.height_scale - 1.0*2*i*self.height_scale/21)
            hh = int(self.height_scale/21)
            pixmap.draw_rectangle (lgc, i <= self.scaling, x1, y1, ww, hh)
        if self.widget_scale:
            self.widget_scale.set_from_pixmap (pixmap, None)

    def render_all (self):
        self.render ()
        self.render_scale ()
