#!/usr/bin/python2.5

import os
import sys

istablet = os.uname()[1].find("Nokia") is not -1

if istablet:
  import hildon
else:
  import pygtk
  
import gtk  
import os
import select
import traceback
import struct
import gobject
import signal
from PIL import Image
from cStringIO import StringIO
import gtk.glade
from time import localtime, strftime
import time

from drawing import *
from socket import *
from tablet_event_t import tablet_event_t
from image_t import image_t
from features_param_t import features_param_t
from generic_cmd_t import generic_cmd_t
from class_param_t import class_param_t
from phone_print_t import phone_print_t
from nav_order_t import nav_order_t
from dictionary_t import dictionary_t
from system_info_t import system_info_t
from logger_info_t import logger_info_t
from ui_map_t import ui_map_t
from ui_map_node_t import ui_map_node_t
from view_map import view_map_t
from view_system_info import view_system_info_t
from view_nav_info import view_nav_info_t
from view_panel import view_panel_t
from view_calibration import view_calibration_t
from codes import *

GLADE_FILE=os.path.expanduser("~/navguide/tabletpc/vsguidetablet/"
                              "vsguidetablet.glade")
BINDIR=os.path.expanduser("~/navguide/tabletpc/vsguidetablet/")
SCREENSHOT_PATH = '.'

if istablet:
  GLADE_FILE=os.path.expanduser("~/vsguidetablet/vsguidetablet.glade")
  BINDIR=os.path.expanduser("~/vsguidetablet/")
  SCREENSHOT_PATH = '/home/user/'

HOST_ADDR_LIST = { 'backpack' : '00:24:2C:B6:E0:6B', # backpack bluetooth device
    'splinter' : '00:0F:3D:05:74:78'} # splinter bluetooth device

HOMEDIR = '/home/user/vsguidetablet'
CPU_NAME = {'dedale0' : 'cpu0', 'dedale1' : 'cpu1', 'dedale2' : 'cpu2'}
RAM_NAME = {'dedale0' : 'ram0', 'dedale1' : 'ram1', 'dedale2' : 'ram2'}
TEMP_NAME = {'dedale0' : 'temp0', 'dedale1' : 'temp1', 'dedale2' : 'temp2'}
DISK_NAME = {'dedale0' : 'disk0', 'dedale1' : 'disk1', 'dedale2' : 'disk2'}
MACH_NAME = {'dedale0' : 'pcname0', 'dedale1' : 'pcname1', 'dedale2' : 'pcname2'}

class VSloop:
 
  def __init__(self):
    #self.printlog ("starting up")
    self.program = None
    if istablet:
      self.program = hildon.Program ()
      self.program.__init__ ()
    self.client = None
    self._connected = False
    
    # message handling variables
    self._data_buf = None
    self._data_channel = ""
    self._recv_mode = "channel"
    self._datalen_buf = ""
    self._datalen = 0
    self._data_received = 0
    self._screenshot_count = 0
    self.messages = []
    self.gate_labels = {} # an empty dictionary
    self.features_param = None
    self.map_list = None
    self.logger_info = None 
    self.heartbeat_time = .0
    if istablet:
	self.bpp = 16
    else:
	self.bpp = 24
    # make the hildon window and add to the program
    self.window = None
    if istablet:
      self.window = hildon.Window ()
    else:
      self.window = gtk.Window (gtk.WINDOW_TOPLEVEL)
    self.window.set_title ("Navguide")
    self.window.connect("destroy", gtk.main_quit)
    if istablet:
      self.program.add_window (self.window)

    # read glade file 
    self.wTree = gtk.glade.XML (GLADE_FILE)

    # status icon
    self.status_icon = self.wTree.get_widget ("status_icon")

    #reparent the vbox1 from glade to self.window
    self.vbox1 = self.wTree.get_widget("vbox1")
    self.reparent_loc(self.vbox1, self.window)

    #get menu from glade and reparent as common menu in hildon program
    self.menuGlade = self.wTree.get_widget("menu1")
    if istablet and self.menuGlade:
      self.program.set_common_menu(self.menuGlade)  
      
    # quit button
    self.menuItem_quit = self.wTree.get_widget("quit")
    if self.menuItem_quit:
      self.menuItem_quit.connect("clicked", gtk.main_quit)

    # calib_start button
    self.calib_start_button = self.wTree.get_widget ("calib_start")
    if (self.calib_start_button):
      self.calib_start_button.connect ("clicked", self.on_calib_start)
    
    # calib_process button
    self.calib_process_button = self.wTree.get_widget ("calib_process")
    if (self.calib_process_button):
      self.calib_process_button.connect ("clicked", self.on_calib_process)

    # calib_check button
    self.calib_check_button = self.wTree.get_widget ("calib_check")
    if (self.calib_check_button):
      self.calib_check_button.connect ("clicked", self.on_calib_check)

    # calib_stop check button
    self.calib_stop_check_button = self.wTree.get_widget ("calib_stop_check")
    if (self.calib_stop_check_button):
      self.calib_stop_check_button.connect ("clicked", self.on_calib_stop_check)

    # start record button
    self.start_record_button = self.wTree.get_widget ("start_record")
    if (self.start_record_button):
      self.start_record_button.connect ("clicked", self.on_start_record)

    # stop record button
    self.stop_record_button = self.wTree.get_widget ("stop_record")
    if (self.stop_record_button):
      self.stop_record_button.connect ("clicked", self.on_stop_record)

    # start system
    if self.wTree.get_widget ("start_system"):
        self.wTree.get_widget ("start_system").connect ("clicked", self.on_start_system)

    # stop system
    if self.wTree.get_widget ("stop_system"):
        self.wTree.get_widget ("stop_system").connect ("clicked", self.on_stop_system)

    # enable/disable features
    self.enable_features = self.wTree.get_widget ("enable_features")
    if (self.enable_features):
        self.enable_features.connect ("toggled", self.on_enable_features_toggled)

    # system reset menu
    self.reset_button = self.wTree.get_widget ("system_reset")
    if (self.reset_button):
      self.reset_button.connect ("activate", self.on_system_reset)
      
    # system shutdown menu
    self.shutdown_button = self.wTree.get_widget ("system_shutdown")
    if (self.shutdown_button):
      self.shutdown_button.connect ("activate", self.on_system_shutdown)
      
    # imu reset menu
    self.imu_reset_button = self.wTree.get_widget ("imu_reset")
    if (self.imu_reset_button):
      self.imu_reset_button.connect ("activate", self.on_imu_reset)

    # camera reset menu
    self.camera_reset_button = self.wTree.get_widget ("camera_reset")
    if (self.camera_reset_button):
      self.camera_reset_button.connect ("activate", self.on_camera_reset)
    
    # screenshot menu 
    self.screenshot_button = self.wTree.get_widget ("screenshot")
    if self.screenshot_button:
      self.screenshot_button.connect ("clicked", self.on_screenshot)

    # start homing
    homing_button = self.wTree.get_widget ("homing")
    if homing_button:
        homing_button.connect ("clicked", self.on_homing)

    # start replay
    replay_button = self.wTree.get_widget ("replay")
    if replay_button:
        replay_button.connect ("clicked", self.on_replay)

    # start exploration
    start_exp_button = self.wTree.get_widget ("start_exploration")
    if start_exp_button:
        start_exp_button.connect ("clicked", self.on_start_exploration)

    # add node
    add_node_button = self.wTree.get_widget ("map_add_node")
    if add_node_button:
        add_node_button.connect ("clicked", self.on_add_node)

    # new gate
    self.new_gate_button = self.wTree.get_widget ("new_gate")
    if self.new_gate_button:
      self.new_gate_button.connect ("clicked", self.on_new_gate)
    
    # feature type
    self.feature_type_combo = self.wTree.get_widget ("features_type")
    if self.feature_type_combo:
        self.feature_type_combo.connect ("changed", self.on_features_type_changed)

    # viewer display mode
    self.viewer_display_mode_combo = self.wTree.get_widget ("viewer_display_mode")
    if self.viewer_display_mode_combo:
        self.viewer_display_mode_combo.connect ("changed", self.on_viewer_display_mode_changed)

    # feature enable/disable
    self.feature_enable_combo = self.wTree.get_widget ("features_enabled")
    if self.feature_enable_combo:
        self.feature_enable_combo.connect ("changed", self.on_features_enabled_changed)

    # video mode
    self.video_mode_combo = self.wTree.get_widget ("video_mode")
    if self.video_mode_combo:
        self.video_mode_combo.connect ("changed", self.on_video_mode_changed)

    # demo level
    self.class_demo_level_combo = self.wTree.get_widget ("class_demo_level")
    if self.class_demo_level_combo:
        self.class_demo_level_combo.connect ("changed", self.on_class_demo_level_changed)

    # checkpoint revisit
    self.checkpoint_revisit_button = self.wTree.get_widget ("checkpoint_revisit")
    if self.checkpoint_revisit_button:
      self.checkpoint_revisit_button.connect ("clicked", self.on_checkpoint_revisit)
    
    # checkpoint create
    self.checkpoint_create_button = self.wTree.get_widget ("checkpoint_create")
    if self.checkpoint_create_button:
      self.checkpoint_create_button.connect ("clicked", self.on_checkpoint_create)

    # label this place
    self.map_apply_label_button = self.wTree.get_widget ("map_apply_label")
    if self.map_apply_label_button:
      self.map_apply_label_button.connect ("clicked", self.on_map_apply_label)

    # set label filter callback
    entry = self.wTree.get_widget ("label_filter")
    if entry:
        entry.connect ("activate", self.populate_label_window)

    # refresh video button
    if self.wTree.get_widget ("video_refresh"):
        self.wTree.get_widget ("video_refresh").connect ("clicked", self.on_video_refresh)

    # map
    if self.wTree.get_widget ("map_zoom_in"):
        self.wTree.get_widget ("map_zoom_in").connect ("clicked", self.on_map_zoom_in)
    if self.wTree.get_widget ("map_zoom_out"):
        self.wTree.get_widget ("map_zoom_out").connect ("clicked", self.on_map_zoom_out)
    if self.wTree.get_widget ("map_center_start"):
        self.wTree.get_widget ("map_center_start").connect ("clicked", self.on_map_center_start)
    if self.wTree.get_widget ("map_center_end"):
        self.wTree.get_widget ("map_center_end").connect ("clicked", self.on_map_center_end)
    if self.wTree.get_widget ("map_move_left"):
        self.wTree.get_widget ("map_move_left").connect ("clicked", self.on_map_move)
    if self.wTree.get_widget ("map_move_top"):
        self.wTree.get_widget ("map_move_top").connect ("clicked", self.on_map_move)
    if self.wTree.get_widget ("map_move_right"):
        self.wTree.get_widget ("map_move_right").connect ("clicked", self.on_map_move)
    if self.wTree.get_widget ("map_move_bottom"):
        self.wTree.get_widget ("map_move_bottom").connect ("clicked", self.on_map_move)
    if self.wTree.get_widget ("map_labels"):
        self.wTree.get_widget ("map_labels").connect ("toggled", self.on_map_label_toggled)
    if self.wTree.get_widget ("map_fancy"):
        self.wTree.get_widget ("map_fancy").connect ("toggled", self.on_map_fancy_toggled)

    # connect button
    self.wTree.get_widget ("connect").connect ("clicked", self.on_connect)

    #destroy the gtk window imported from glade
    self.gtkWindow = self.wTree.get_widget("window1")
    self.gtkWindow.destroy()

    # set combo boxes
    if self.wTree.get_widget ("video_mode"):
        self.wTree.get_widget ("video_mode").set_active (0)
    if self.wTree.get_widget ("video_front_back"):
        self.wTree.get_widget ("video_front_back").set_active (0)

    # set window size
    self.window.set_resizable (False)
    if istablet:
        self.window.fullscreen ()
    else:
        self.window.set_resizable (True)
        self.window.set_default_size (800, 480)

    # add timeouts
    gobject.timeout_add (1000, self.update_status_bar)
    gobject.timeout_add (1000, self.publish_ui_state)
    
    #display everything
    self.window.show_all()
   
    # map
    self.view_map = view_map_t (self.wTree.get_widget ("map"), 700, 400, self.wTree.get_widget ("map_scale"),
            50, 90, 20, self.bpp)
    self.wTree.get_widget ("map_frame").connect ("button-press-event", self.on_map_press)
    self.wTree.get_widget ("map_frame").connect ("button-release-event", self.on_map_release)
    self.wTree.get_widget ("map_frame").connect ("motion-notify-event", self.on_map_motion_notify)

    # system info
    self.view_system_info = view_system_info_t (self.wTree.get_widget ("system_info"), 800, 400, self.bpp)
    self.wTree.get_widget ("system_info_frame").connect ("button-press-event", self.on_system_info_press)
    self.wTree.get_widget ("system_info_frame").set_events (gtk.gdk.EXPOSURE_MASK | gtk.gdk.KEY_PRESS_MASK)

    # nav info
    self.view_nav_info = view_nav_info_t (self.wTree.get_widget ("nav_info"), 800, 400, self.bpp)
    self.wTree.get_widget ("nav_frame").connect ("button-press-event", self.on_nav_press)
    self.wTree.get_widget ("nav_frame").set_events (gtk.gdk.EXPOSURE_MASK | gtk.gdk.KEY_PRESS_MASK)

    # panel
    self.view_panel = view_panel_t (self.wTree.get_widget ("panel_info"), 800, 400, self.bpp)
    self.wTree.get_widget ("panel_frame").connect ("button-press-event", self.on_panel_info_press)
    self.view_panel.render ()

    # calibration
    self.view_calibration = view_calibration_t (self.wTree.get_widget ("calibration_info"), 800, 400, self.bpp)
    self.wTree.get_widget ("calibration_frame").connect ("button-press-event", self.on_calibration_info_press)
    self.view_calibration.render ()

    # main window keyboard callback
    #if self.wTree.get_widget ("window1"):
    self.wTree.get_widget ("vbox1").connect ("key-press-event", self.on_main_key_press)

    #########################################################################
  # message handlers

  def _handle_FUTURE_DIRECTION (self, data):
      p = generic_cmd_t.decode (data)
      if self.view_nav_info:
        self.view_nav_info.future_direction[p.code] = int (p.text)

  def _handle_MAP_LIST (self, data):
      self.map_list = dictionary_t.decode (data)

  def _handle_UI_MAP (self, data):
    # cpu intensive, so do it only if necessary
    self.view_map.data = ui_map_t.decode (data)
    self.view_map.update_hash ()
    if self.wTree.get_widget ("notebook").get_current_page() == 2:
      self.view_map.render_all ()
      print "rendering map..."

  def _handle_LOGGER_INFO (self, data):
      self.logger_info = logger_info_t.decode (data)

  def _handle_SYSTEM_INFO (self, data):
    # cpu intensive, so do it only if necessary
    si = system_info_t.decode (data)
    self.view_system_info.data = si
    if self.view_system_info:
        if self.logger_info:
            self.view_system_info.logger_info = self.logger_info
        if self.wTree.get_widget ("notebook").get_current_page () == 3:
            self.view_system_info.render ()

  def _handle_IMAGE_ANNOUNCE (self, data):
      cmd = generic_cmd_t.decode (data)
      if self.view_system_info:
          self.view_system_info.image_announce[cmd.code] = time.time()

  def _handle_FEATURES_ANNOUNCE (self, data):
      cmd = generic_cmd_t.decode (data)
      if self.view_system_info:
          self.view_system_info.features_announce[cmd.code] = time.time()

  def _handle_HEARTBEAT (self, data):
      self.heartbeat_time = time.time()

  def _handle_BLIT (self, data, id):
    if self.wTree.get_widget("hide_images").get_active():
        width=282
        height=180
        self.view_nav_info.left_img = create_image_from_data (None, width, height, 24)
        self.view_nav_info.right_img = create_image_from_data (None, width, height, 24)
        return
    image = image_t.decode (data)
    im = Image.open (StringIO (image.data))
    imrgb = im.convert ("RGB")
    if id == 0 and self.view_nav_info:
        self.view_nav_info.left_img = create_image_from_data (imrgb.tostring(), image.width, image.height, 24)
    if id == 1 and self.view_nav_info:
        self.view_nav_info.right_img = create_image_from_data (imrgb.tostring(), image.width, image.height, 24)
    if self.wTree.get_widget ("notebook").get_current_page () == 1:
        self.view_nav_info.render ()
    
  def _handle_BLIT0 (self, data):
    self._handle_BLIT (data, 0)

  def _handle_BLIT1 (self, data):
    self._handle_BLIT (data, 1)

  def _handle_BLIT2 (self, data):
    self._handle_BLIT (data, 2)

  def _handle_BLIT3 (self, data):
    self._handle_BLIT (data, 3)

  def _handle_PRINT (self, data):
    msg = phone_print_t.decode (data)
    self.messages.append (msg.text)
  
  def _handle_SCRSHOT_REQUEST (self, data):
    (scr,data,width,height,stride) = self.screenshot ()
    del scr

  def _handle_CLASS_STATE (self, data):
    # cpu intensive, so do it only if necessary
    st = class_param_t.decode (data)
    self.view_nav_info.data = st
    self.view_map.class_param = st
    self.view_nav_info.future_direction[0] = st.next_direction
    if self.wTree.get_widget ("notebook").get_current_page () == 1 or \
        self.wTree.get_widget ("notebook").get_current_page () == 2:
        self.view_nav_info.render ()

  def screenshot (self):
    width  = gtk.gdk.screen_width()
    height = gtk.gdk.screen_height()
    root = gtk.gdk.screen_get_default()
    active = root.get_active_window()
    # calculate the size of the wm decorations
    relativex, relativey, winw, winh, d = active.get_geometry()
    w = winw + (relativex*2)
    h = winh + (relativey+relativex)
    screenposx, screenposy = active.get_root_origin()
    
    screenshot = gtk.gdk.Pixbuf.get_from_drawable (
      gtk.gdk.Pixbuf (gtk.gdk.COLORSPACE_RGB, False, 8, w, h),
      gtk.gdk.get_default_root_window(),
      gtk.gdk.colormap_get_system(),
      screenposx, screenposy, 0, 0, w, h)
    count=0
    while os.path.exists ("%s/screenshot_%03d.jpg" % (SCREENSHOT_PATH, count)):
      count=count+1
    screenshot.save ("%s/screenshot_%03d.jpg" %
                     (SCREENSHOT_PATH, count), "jpeg", {"quality":"100"})
    self._screenshot_count = self._screenshot_count + 1
    return (screenshot, screenshot.get_pixels(), screenshot.get_width(),
            screenshot.get_height(), screenshot.get_rowstride())

  def send_screenshot (self):
    msg = image_t ()
    msg.garb, msg.data, msg.width, msg.height,
    msg.row_stride = self.screenshot()
    msg.size = msg.row_stride * msg.height
    self.send_message ("TABLET_SCRSHOT", msg.encode())
    del msg.data

#########################################################################

  def on_screenshot (self, widget):
    self.screenshot ()

  def request_images (self):
    self.send_tablet_event ("TABLET_IMAGES_REQUEST", 0)

  def on_system_reset (self, widget):
    self.send_tablet_event ("TABLET_EVENT", 0)
    
  def on_system_shutdown (self, widget):
    self.send_tablet_event ("TABLET_EVENT", 1)

  def on_camera_reset (self, widget):
      print "camera reset..."
      self.send_tablet_event ("TABLET_EVENT", 2)

  def on_imu_reset (self, widget):
      print "imu reset..."
      self.send_tablet_event ("TABLET_EVENT", 3)

  def on_start_record (self, widget):
      self.send_tablet_event ("TABLET_EVENT", 4);

  def on_stop_record (self, widget):
      self.send_tablet_event ("TABLET_EVENT", 5);
      # switch classifier to idle
      msg = generic_cmd_t ()
      msg.code = CLASS_STANDBY
      self.send_message ("CLASS_PARAM", msg.encode())

  def on_start_system (self, widget):
      self.send_tablet_event ("TABLET_EVENT", 6);

  def on_stop_system (self, widget):
      if self.confirm ("Are you sure you want to stop the sytem?") == gtk.RESPONSE_ACCEPT:
          self.send_tablet_event ("TABLET_EVENT", 7);

  def on_main_key_press (self, widget, event):
      if event.keyval == 65307:
        self.on_connect (None)

  def on_map_press (self, widget, event):
      if self.view_map:
        nd = self.view_map.find_node_by_coords (event.get_coords()[0], event.get_coords()[1])
        if nd:
            print "Selected node %d" % nd.id
            # exploration mode: label the node
            if self.view_map.class_param.mode == self.view_map.class_param.EXPLORATION_MODE:
                label = self.prompt_user ("Label node %d" % nd.id)
                if label:
                    self.label_node (nd.id, label)
      return True

  def on_map_motion_notify (self, widget, event):
      if event.state == gtk.gdk.BUTTON1_MASK:
          if self.view_map and not self.view_map.dragging:
            self.view_map.drag_start (event.get_coords()[0], event.get_coords()[1])

  def on_map_release (self, widget, event):
      if self.view_map and self.view_map.dragging:
        self.view_map.drag_stop (event.get_coords()[0], event.get_coords()[1])

  def on_nav_press (self, widget, event):
      if self.view_nav_info:
        s = self.view_nav_info.on_key_press (event.get_coords()[0], event.get_coords()[1])
        if s == 0:
            # unclear guidance
            msg = generic_cmd_t ()
            msg.code = CLASS_USER_UNCLEAR_GUIDANCE
            msg.text = " "
            self.send_message ("CLASS_CMD", msg.encode())
        if s == 1:
            # user is lost
            msg = generic_cmd_t ()
            msg.code = CLASS_USER_LOST
            msg.text = " "
            self.send_message ("CLASS_CMD", msg.encode())

  def on_system_info_press (self, widget, event):
      if self.view_system_info:
          s = self.view_system_info.on_key_press (event.get_coords()[0], event.get_coords()[1])
          if s == 0: # sound toggle button
             msg = generic_cmd_t ()
             msg.code = MONITOR_SOUND_PARAM_CHANGED
             msg.text = "%d" % (1 - self.view_system_info.data.sound_enabled)
             self.send_message ("SYSTEM_CMD", msg.encode())
          if s == 1: # shutdown
                if self.confirm ("Shutdown system?") == gtk.RESPONSE_ACCEPT:
                    msg = generic_cmd_t ()
                    msg.code = MONITOR_SHUTDOWN_SYSTEM
                    msg.text = ""
                    self.send_message ("SYSTEM_CMD", msg.encode())
          if s == 2: # reboot
                if self.confirm ("Restart system?") == gtk.RESPONSE_ACCEPT:
                    msg = generic_cmd_t ()
                    msg.code = MONITOR_RESTART_SYSTEM
                    msg.text = ""
                    self.send_message ("SYSTEM_CMD", msg.encode())
          if s == 3: # logger button
             if self.view_system_info.logger_info and self.view_system_info.logger_info.logging:
                if self.confirm ("Stop logger?") == gtk.RESPONSE_ACCEPT:
                    self.stop_logger ()
             else:
                if self.confirm ("Start logger?") == gtk.RESPONSE_ACCEPT:
                    self.start_logger ()


  def on_panel_info_press (self, widget, event):
      if self.view_panel:
        s = self.view_panel.on_key_press (event.get_coords()[0], event.get_coords()[1])
        if s == 0:
          if self.confirm ("Start new exploration?") == gtk.RESPONSE_ACCEPT:
              msg = generic_cmd_t ()
              msg.code = CLASS_START_EXPLORATION
              msg.text = ""
              self.send_message ("CLASS_CMD", msg.encode())
        if s == 1:
          if self.confirm ("End exploration?") == gtk.RESPONSE_ACCEPT:
              msg = generic_cmd_t ()
              msg.code = CLASS_END_EXPLORATION
              msg.text = ""
              self.send_message ("CLASS_CMD", msg.encode())
        if s == 2:
              if self.view_map.class_param:
                nodeid = self.node_prompt ()
                if nodeid is not -1:
                    msg = generic_cmd_t ()
                    msg.code = CLASS_FORCE_NODE
                    msg.text = "%d" % nodeid
                    self.send_message ("CLASS_CMD", msg.encode())

        if s == 3:
              if self.view_map.class_param:
                nodeid = self.node_prompt ()
                if nodeid is not -1:
                    msg = generic_cmd_t ()
                    msg.code = CLASS_GOTO_NODE
                    msg.text = "%d" % nodeid
                    self.send_message ("CLASS_CMD", msg.encode())
        if s == 4:
            if self.view_map.class_param and self.view_map.class_param.mode == self.view_map.class_param.EXPLORATION_MODE:
                label = self.prompt_user ("Label current place")
                if label and self.view_map.class_param.nodeid_now != -1:
                    self.label_node (self.view_map.class_param.nodeid_now, label)
        if s == 5:
            map_name = self.map_prompt ()
            if map_name:
                    msg = generic_cmd_t ()
                    msg.code = CLASS_LOAD_MAP
                    msg.text = "%s" % map_name
                    self.send_message ("CLASS_CMD", msg.encode())
                

  def on_calibration_info_press (self, widget, event):
      if self.view_calibration:
        s = self.view_calibration.on_key_press (event.get_coords()[0], event.get_coords()[1])
        if s == 0: # test calibration
            msg = generic_cmd_t ()
            msg.code = CLASS_CHECK_CALIBRATION
            msg.text = ""
            self.send_message ("CLASS_CMD", msg.encode())
        if s == 1: # cancel
            msg = generic_cmd_t ()
            msg.code = CLASS_STOP_CHECK_CALIBRATION
            msg.text = ""
            self.send_message ("CLASS_CMD", msg.encode())
            self.view_calibration.reset ()
        if s == 2: # main button
            msg = generic_cmd_t ()
            msg.code = CLASS_CALIBRATION_STEP
            msg.text = "%d" % self.view_calibration.calibration_step
            self.send_message ("CLASS_CMD", msg.encode())
            self.view_calibration.next_step ()
        if s == 3: # test UI
            self.test_ui () 
        print s
        # render
        self.view_calibration.render ()
            
  ##############################################################################
  # utility

  def label_node (self, id, label):
    # send message
    msg = generic_cmd_t ()
    msg.code = CLASS_SET_NODE_LABEL
    msg.text = "%d %s" % (id, label)
    self.send_message ("CLASS_CMD", msg.encode())

  def update_status_bar (self):
    if self.messages and len(self.messages) > 0:
        msg = self.messages.pop (0)
        if msg is not None:
            self.update_entry ("entry_status", msg)
    tms = "%.1f secs" % (time.time() - self.heartbeat_time)
#    tms = strftime ("%H:%M:%S", localtime())
    self.update_entry ("entry_time", tms)
    return True
 
  def publish_ui_state (self):
    msg = generic_cmd_t ()
    msg.code = UI_STATE
    msg.text = "%d" % self.wTree.get_widget("hide_images").get_active()
    self.send_message ("UI_STATE", msg.encode())
    return True

  def reparent_loc(self, widget, newParent):
    widget.reparent(newParent)

  def destroy(self):
    if self.client:
      self.client.close ()
    self.client = None
    self._connected = False

  def send_message (self, channel, data):
    if self.client:
      tosend = "".join ([ channel, "\0",
      struct.pack (">I", len(data)), data])
      self.client.sendall (tosend)

  def on_start_exploration (self, widget):
      if self.confirm ("Confirm new exploration?") == gtk.RESPONSE_ACCEPT:
          msg = generic_cmd_t ()
          msg.code = CLASS_START_EXPLORATION
          self.send_message ("CLASS_PARAM", msg.encode())

  def on_homing (self, widget):
      if self.confirm ("Confirm homing?") == gtk.RESPONSE_ACCEPT:
          msg = generic_cmd_t ()
          msg.code = CLASS_HOMING
          self.send_message ("CLASS_PARAM", msg.encode())

  def on_replay (self, widget):
      if self.confirm ("Confirm replay?") == gtk.RESPONSE_ACCEPT:
          msg = generic_cmd_t ()
          msg.code = CLASS_REPLAY
          self.send_message ("CLASS_PARAM", msg.encode())

  def on_add_node (self, widget):
      msg = generic_cmd_t ()
      msg.code = CLASS_ADD_GATE
      self.send_message ("CLASS_PARAM", msg.encode())

  def map_prompt (self):
    dialog = gtk.Dialog("Select map", None, gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
            (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT, gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
    radiobuttons = {}
    if self.map_list:
        nkeys = len (self.map_list.keys)
        if nkeys > 0:
            scrolledwindow = gtk.ScrolledWindow()
            table = gtk.Table (nkeys, 2)
            radiobutton = None
            count=0
            for k in self.map_list.keys:
                radiobutton = gtk.RadioButton (radiobutton)
                radiobuttons[k] = radiobutton
                table.attach (radiobutton, 1, 2, count, count+1)
                table.attach (gtk.Label ("%s" % k), 2, 3, count, count+1)
                count += 1
            scrolledwindow.add_with_viewport (table)
            dialog.vbox.pack_start (scrolledwindow, True, True, 0)
    dialog.set_size_request (500, 300)
    dialog.show_all()

    response = dialog.run ()
    map_name = None
    if response == gtk.RESPONSE_ACCEPT:
        for kr, vr in radiobuttons.iteritems():
            if vr.get_active():
                map_name = kr
                self.messages.append ("Requested map %s" % kr)
    dialog.destroy()
    return map_name

  def node_prompt (self):
    dialog = gtk.Dialog("Select destination", None, gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
            (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT, gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
    max_nodeid = 0
    min_nodeid = 0
    radiobuttons = {}
    if self.view_map and self.view_map.data:
        nodes = self.view_map.list_nodes_with_label()
        (min_nodeid, max_nodeid) = self.view_map.min_max_nodeid ()
        nkeys = len (nodes.keys())
        if nkeys > 0:
            scrolledwindow = gtk.ScrolledWindow()
            table = gtk.Table (nkeys, 3)
            radiobutton = None
            count=0
            for k, v in nodes.iteritems():
                radiobutton = gtk.RadioButton (radiobutton)
                radiobuttons[k] = radiobutton
                table.attach (radiobutton, 1, 2, count, count+1)
                table.attach (gtk.Label ("%d" % k), 2, 3, count, count+1)
                table.attach (gtk.Label ("%s" % v), 3, 4, count, count+1)
                count += 1
            scrolledwindow.add_with_viewport (table)
            dialog.vbox.pack_start (scrolledwindow, True, True, 0)
    dialog.vbox.pack_start (gtk.Label ("Node ID [%d-%d]" % (min_nodeid, max_nodeid)), False, True, 0)
    entry = gtk.Entry (10)
    dialog.vbox.pack_start (entry, False, True, 0)
    dialog.set_size_request (500, 300)
    dialog.show_all()

    response = dialog.run ()
    nodeid = -1
    if response == gtk.RESPONSE_ACCEPT:
        if len (entry.get_text()) > 0:
            try:
                nodeid = int (entry.get_text())
                self.messages.append ("Going to node %d" % nodeid)
            except (TypeError, ValueError):
                self.messages.append ("Error: Invalid ID")
                pass
        if nodeid == -1:
            for kr, vr in radiobuttons.iteritems():
                if vr.get_active():
                    nodeid = kr
                    self.messages.append ("Going to node %d" % kr)
    dialog.destroy()
    return nodeid

  def on_calib_start (self, widget):
    if not self._connected:
	self.dialog ("Connect first.", gtk.MESSAGE_ERROR)
        return
    msg = generic_cmd_t ()
    msg.code = CLASS_START_CALIBRATION
    self.send_message ("CLASS_PARAM", msg.encode())

  def on_calib_process (self, widget):
    if not self._connected:
	self.dialog ("Connect first.", gtk.MESSAGE_ERROR)
        return
    msg = generic_cmd_t ()
    msg.code = CLASS_RUN_CALIBRATION
    self.send_message ("CLASS_PARAM", msg.encode())

  def on_calib_stop_check (self, widget):
    if not self._connected:
	self.dialog ("Connect first.", gtk.MESSAGE_ERROR)
        return
    msg = generic_cmd_t ()
    msg.code = CLASS_STOP_CHECK_CALIBRATION
    self.send_message ("CLASS_PARAM", msg.encode())

  def on_calib_check (self, widget):
    if not self._connected:
	self.dialog ("Connect first.", gtk.MESSAGE_ERROR)
        return
    msg = generic_cmd_t ()
    msg.code = CLASS_CHECK_CALIBRATION
    self.send_message ("CLASS_PARAM", msg.encode())

  def on_connect (self, widget):
    if self.client:
        self.client.close ()
        self.client = None

    if self.status_icon:
        self.status_icon.set_from_stock (gtk.STOCK_DISCONNECT, gtk.ICON_SIZE_LARGE_TOOLBAR)

    # prompt user for hostname
    dialog = gtk.Dialog("Select host", None, gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
            (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT, gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
    dialog.show ()
    entry = gtk.combo_box_new_text ()
    for hn, kn in HOST_ADDR_LIST.iteritems ():
        entry.append_text (hn)
    dialog.vbox.pack_start (entry, True, True, 0)
    entry.show()
    entry.set_active (0)
    response = dialog.run ()
    if response == gtk.RESPONSE_REJECT:
        dialog.destroy ()
        return
    hostname = entry.get_active_text ()
    dialog.destroy ()

    self.messages.append ("Trying to connect to " + hostname)

    # create socket
    self.client = socket(AF_BLUETOOTH,SOCK_STREAM, BTPROTO_RFCOMM)
    addr=HOST_ADDR_LIST[hostname]
    port = 17
    address=(addr,port)
    
    try:
        print "trying to connect to " + addr
        self.client.connect (address)
        gobject.io_add_watch(self.client, gobject.IO_IN, self.handle_data)
        self._connected = True
        self.messages.append ("Connected to " + hostname)
        print "connected!"
    except error:
        self.client.close ()
        self.client = None
        self._connected = False
        self.dialog ("Connection failed!", gtk.MESSAGE_ERROR)
    
    if self.client and self.status_icon:
        self.status_icon.set_from_stock (gtk.STOCK_CONNECT, gtk.ICON_SIZE_LARGE_TOOLBAR)

  def on_new_gate (self, widget):
    msg = generic_cmd_t ()
    msg.code = CLASS_NEW_GATE
    self.send_message ("CLASS_PARAM", msg.encode())
    print "Creating new gate..."

  def on_checkpoint_create (self, widget):
    msg = generic_cmd_t ()
    msg.code = CLASS_CHECKPOINT_CREATE
    self.send_message ("CLASS_PARAM", msg.encode())
    print "Sending create message..."

  def test_ui (self):
    draw_cross (self.wTree.get_widget ("nav_image_left"), 320, 240, self.bpp)
    draw_cross (self.wTree.get_widget ("nav_image_right"), 320, 240, self.bpp)
    draw_cross (self.wTree.get_widget ("image_left"), 320, 240, self.bpp)
    draw_cross (self.wTree.get_widget ("image_right"), 320, 240, self.bpp)
    draw_compass (self.wTree.get_widget ("nav_compass"), 32, 124, 160, 160, self.bpp);
    draw_progress (self.wTree.get_widget ("nav_progress"), 45, 160, 80, self.bpp);
    self.view_map.init_random ()
    self.view_map.render_all ()
    self.view_system_info.init_random ()
    self.view_system_info.render ()
    self.view_nav_info.init_random ()
    self.view_nav_info.render ()

  def on_map_zoom_in (self, widget):
      if self.view_map:
        self.view_map.zoom_in ()

  def on_map_zoom_out (self, widget):
      if self.view_map:
        self.view_map.zoom_out ()

  def on_map_center_start (self, widget):
      if self.view_map:
        self.view_map.center_start ()

  def on_map_center_end (self, widget):
      if self.view_map:
        self.view_map.center_end ()

  def on_map_move (self, widget):
        if self.view_map and widget.get_name () == "map_move_right":
            self.view_map.move (0)
        if self.view_map and widget.get_name () == "map_move_topright":
            self.view_map.move (1)
        if self.view_map and widget.get_name () == "map_move_top":
            self.view_map.move (2)
        if self.view_map and widget.get_name () == "map_move_topleft":
            self.view_map.move (3)
        if self.view_map and widget.get_name () == "map_move_left":
            self.view_map.move (4)
        if self.view_map and widget.get_name () == "map_move_bottomleft":
            self.view_map.move (5)
        if self.view_map and widget.get_name () == "map_move_bottom":
            self.view_map.move (6)
        if self.view_map and widget.get_name () == "map_move_bottomright":
            self.view_map.move (7)

  def on_map_label_toggled (self, widget):
      if self.view_map:
        self.view_map.toggle_label (widget.get_active ())

  def on_map_fancy_toggled (self, widget):
      if self.view_map:
        self.view_map.toggle_fancy (widget.get_active ())

  def on_video_refresh (self, widget):
    msg = generic_cmd_t ()
    msg.code = CLASS_BLIT_REQUEST
    self.send_message ("CLASS_PARAM", msg.encode())

  def on_enable_features_toggled (self, widget):
      pass

  def handle_data (self, source, condition):
    data = source.recv (672)
    self._handle_chunk (data)
    if len(data)  > 0:
	return True
    else:
	return False

  def confirm (self, data=None):
      dialog = gtk.Dialog("Message", None, gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
              (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT, gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
      dialog.show ()
      entry = gtk.Label (data)
      dialog.vbox.pack_start (entry, True, True, 0)
      entry.show()
      response = dialog.run ()
      dialog.destroy ()
      return response

  def prompt_user (self, title):
      # query user for a label
      dialog = gtk.Dialog(title, None, gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
              (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT, gtk.STOCK_OK, gtk.RESPONSE_ACCEPT))
      entry = gtk.Entry(50)
      dialog.vbox.pack_start (entry, True, True, 0)
      entry.show ()
      response = dialog.run ()
      dialog.destroy ()
      if response == gtk.RESPONSE_ACCEPT:
        return entry.get_text ()
      return None

  def dialog (self, data=None, type=gtk.MESSAGE_INFO):
    if not data:
	return
    dialog = gtk.MessageDialog (self.window, gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,
                                type, gtk.BUTTONS_OK, data)
    dialog.connect ('response', lambda dialog, response: dialog.destroy())
    dialog.show()

  def start_logger (self):
      cmd = generic_cmd_t ()
      cmd.code = 0
      cmd.text = ""
      self.send_message ("START_LOGGING", cmd.encode())

  def stop_logger (self):
      cmd = generic_cmd_t ()
      cmd.code = 0
      cmd.text = ""
      self.send_message ("STOP_LOGGING", cmd.encode())
  
  def update_entry (self, label, data):
    entry = self.wTree.get_widget(label)
    if entry:
      if data:
        entry.set_text (data)
      else:
        entry.set_text ("")
        
  def _handle_chunk (self, msg):
    msglen = len (msg)
    if msglen == 0: return

    if self._recv_mode == "channel":
        s = msg.split ("\0", 1)
        self._data_channel = self._data_channel + s[0]
        if (len (s) > 1):
            self._recv_mode = "datalen"
            self._datalen_buf = ""
            self._handle_chunk (s[1])
    elif self._recv_mode == "datalen":
        toadd = min (msglen, 4 - len (self._datalen_buf))
        self._datalen_buf = self._datalen_buf + msg[:toadd]
        if len (self._datalen_buf) == 4:
            self._datalen = struct.unpack (">I", self._datalen_buf)[0]
            self._recv_mode = "data"
            self._data_buf = StringIO ()
            self._data_received = 0
            self._datalen_buf = ""
            if msglen > toadd:
                self._handle_chunk (msg[toadd:])
    elif self._recv_mode == "data":
        toadd = min (msglen, self._datalen - self._data_received)
        if toadd != msglen:
            self._data_buf.write (msg[:toadd])
        else:
            self._data_buf.write (msg)

        self._data_received += toadd
        if self._data_received == self._datalen:
            # complete message received.  handle it?
            try:
                handler = getattr (self, "_handle_%s" % self._data_channel)
            except AttributeError:
                print "unrecognized channel %s" % self._data_channel
                handler = None
            if handler:
                handler (self._data_buf.getvalue ())
            self._data_buf.close ()
            self._data_buf = None
            self._datalen = 0
            self._data_channel = ""
            self._data_received = 0
            self._recv_mode = "channel"
            if msglen > toadd:
                self._handle_chunk (msg[toadd:])

  def run(self):
    gtk.main ()
    
if __name__ == "__main__":
  app = VSloop()
  app.run() 
  app.destroy ()
