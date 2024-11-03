import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Gdk, GObject, Pango, PangoCairo

import cairo
import zzub

import config
from neil.utils import ( PluginType,
    get_plugin_type, is_effect, is_a_generator, is_controller, is_root, is_instrument,
    blend_float, box_contains, distance_from_line, plugin_color_names,
    prepstr, linear2db, ui
)

from neil import components, views
import neil.common as common
from rack import ParameterView
from neil.presetbrowser import PresetView
from patterns import key_to_note

from .volume_slider import VolumeSlider
from .utils import draw_line, draw_wavy_line, draw_line_arrow, router_sizes

AREA_ANY = 0
AREA_PANNING = 1
AREA_LED = 2


class RouteView(Gtk.DrawingArea):
    """
    Allows to monitor and control plugins and their connections.
    """
    __neil__ = dict(
        id = 'neil.core.router.view',
        singleton = True,
        categories = []
    )

    current_plugin = None
    connecting = False
    connecting_alt = False
    dragging = False
    dragoffset = 0, 0
    contextmenupos = 0, 0

    COLOR_DEFAULT = 0
    COLOR_MUTED = 1
    COLOR_LED_OFF = 2
    COLOR_LED_ON = 3
    COLOR_LED_BORDER = 4
    COLOR_LED_WARNING = 7
    COLOR_CPU_OFF = 2
    COLOR_CPU_ON = 3
    COLOR_CPU_BORDER = 4
    COLOR_CPU_WARNING = 4
    COLOR_BORDER_IN = 8
    COLOR_BORDER_OUT = 8
    COLOR_BORDER_SELECT = 8
    COLOR_TEXT = 9

    def __init__(self, parent):
        """
        Initializer.

        @param rootwindow: Main window.
        @type rootwindow: NeilFrame
        """
        Gtk.DrawingArea.__init__(self)

        self.panel                       = parent
        self.surface                     = None
        self.autoconnect_target          = None
        self.chordnotes                  = []
        self.volume_slider               = VolumeSlider(self)
        self.selections                  = {}                        #selections stored using the remember selection option
        self.area                        = Gdk.Rectangle(0, 0, 1, 1) # will be the pixel dimensions allocated to the drawingarea
        self.connecting                  = False
        self.drag_update_timer           = False

        eventbus                         = components.get('neil.core.eventbus')
        eventbus.zzub_connect           += self.on_zzub_redraw_event
        eventbus.zzub_disconnect        += self.on_zzub_redraw_event
        eventbus.zzub_plugin_changed    += self.on_zzub_plugin_changed
        eventbus.document_loaded        += self.redraw
        eventbus.active_plugins_changed += self.on_active_plugins_changed

        self.last_drop_ts = 0
        self.connect('drag-motion', self.on_drag_motion)
        self.connect('drag-data-received', self.on_drag_data_received)
        self.connect('drag-leave', self.on_drag_leave)
        self.drag_dest_set(Gtk.DestDefaults.ALL, common.PLUGIN_DRAG_TARGETS, Gdk.DragAction.COPY)

        self.add_events(Gdk.EventMask.ALL_EVENTS_MASK)
        self.update_colors()

        self.set_can_focus(True)
        self.connect('button-press-event', self.on_left_down)
        self.connect('button-release-event', self.on_left_up)
        self.connect('motion-notify-event', self.on_motion)
        self.connect("draw", self.on_draw)
        self.connect('key-press-event', self.on_key_jazz, None)
        self.connect('key-release-event', self.on_key_jazz_release, None)
        self.connect('configure-event', self.on_configure_event)
        self.connect('realize', self.on_realized)

        if config.get_config().get_led_draw() == True:
            self.adjust_draw_led_timer()


    def on_realized(self, widget):
        self.update_area()

    def adjust_draw_led_timer(self, timeout = 200):
        if getattr(self, 'ui_timeout', False):
            GObject.source_remove(self.ui_timeout)

        self.ui_timeout = GObject.timeout_add(timeout, self.on_draw_led_timer)

    def on_configure_event(self, widget, requisition):
        self.update_area()
        

    def update_area(self):
        self.area = self.get_allocation()
        self.redraw(True)
        

    def on_active_plugins_changed(self, *args):
       # player = components.get('neil.core.player')
        common.get_plugin_infos().reset_plugingfx()


    def get_plugin_info(self, plugin):
        return common.get_plugin_infos().get(plugin)


    def on_drag_leave(self, widget, context, time):
        #print "on_drag_leave",widget,context,time
        self.drag_unhighlight()


    def on_drag_motion(self, widget, context, x, y, time):
        # TODO!: highlight arrow drop
        # conn = self.get_connection_at((x,y))
        # if conn:
        #     print conn
        #     bmpctx = self.routebitmap.cairo_create()
        #     bmpctx.translate(0.5,0.5)
        #     bmpctx.set_line_width(1)
        #     bmpctx.set_source_rgb(1,0,0)
        #     # draw_line_arrow(bmpctx, arrowcolors[mp.get_input_connection_type(index)], int(crx), int(cry), int(rx), int(ry))
        #     bmpctx.rectangle(x,y,20,20)
        #     bmpctx.stroke()

        source = context.get_source_window()
        if not source:
            return
        self.drag_highlight()
        # context.drag_status(context.suggested_action, time)
        return True

    def on_drag_data_received(self, widget, context, x, y, data, info, time):
        if data.get_format() != 8:
            Gtk.drag_finish(context, False, False, time)
            Gdk.drop_finish(context, True, time)
            return True
        
        context.finish(True, True, time)

        player = components.get_player()
        player.plugin_origin = self.pixel_to_float((x, y))
        uri = str(data.get_data(), "utf-8")
        conn = None
        plugin = None
        pluginloader = player.get_pluginloader_by_name(uri)

        # autoconnect if dragged onto an existing audio connection
        if is_effect(pluginloader):
            conn = self.get_audio_connection_at((x, y))

        # autoconnect if dragged onto an existing plugin
        if not conn:
            res = self.get_plugin_at((x, y))
            if res:
                mp, (px, py), area = res
                if is_effect(mp) or is_instrument(mp):
                    plugin = mp

        player.create_plugin(pluginloader, connection=conn, plugin=plugin)
        Gtk.drag_finish(context, True, False, time)
        Gdk.drop_finish(context, True, time)
            
        return True


    def update_colors(self):
        """
        Updates the routers color scheme.
        """
        cfg = config.get_config()
        names = [
                'MV ${PLUGIN}',
                'MV ${PLUGIN} Mute',
                'MV ${PLUGIN} LED Off',
                'MV ${PLUGIN} LED On',
                'MV Indicator Background',
                'MV Indicator Foreground',
                'MV Indicator Border',
                'MV Indicator Warning',
                'MV Border',
                'MV Text',
        ]

        
        self.plugintype2brushes = {}
        for plugintype, name in plugin_color_names.items():
            brushes = []
            for name in [x.replace('${PLUGIN}', name) for x in names]:
                brushes.append(cfg.get_float_color(name))
            self.plugintype2brushes[plugintype] = brushes

        self.default_brushes = self.plugintype2brushes[PluginType.Instrument]

        common.get_plugin_infos().reset_plugingfx()

        self.arrowcolors = {
            zzub.zzub_connection_type_audio: [
                cfg.get_float_color("MV Arrow"),
                cfg.get_float_color("MV Arrow Border In"),
                cfg.get_float_color("MV Arrow Border Out"),
            ],
            zzub.zzub_connection_type_cv: [
                cfg.get_float_color("MV Controller Arrow"),
                cfg.get_float_color("MV Controller Arrow Border In"),
                cfg.get_float_color("MV Controller Arrow Border Out"),
            ],
        }


    def on_zzub_plugin_changed(self, plugin):
        common.get_plugin_infos().get(plugin).reset_plugingfx()
        self.redraw(True)

    def on_zzub_redraw_event(self, *args):
        print("zubb redraw event")
        self.redraw(True)

    def on_focus(self, event):
        self.redraw(True)

    def store_selection(self, index, plugins):
        self.selections[index] = [plugin.get_id() for plugin in plugins]

    def restore_selection(self, index):
        if self.has_selection(index):
            player = components.get_player()
            plugins = player.get_plugin_list()
            player.active_plugins = [plugin for plugin in plugins if plugin.get_id() in self.selections[index]]

    def has_selection(self, index):
        return index in self.selections

    def selection_count(self):
        return len(self.selections)

    def on_context_menu(self, widget, event):
        """
        Event handler for requests to show the context menu.

        @param event: event.
        @type event: wx.Event
        """
        mx, my = int(event.x), int(event.y)
        player = components.get('neil.core.player')
        player.plugin_origin = self.pixel_to_float((mx, my))
        res = self.get_plugin_at((mx, my))

        if res:
            mp, (x, y), area = res
            if mp in player.active_plugins and len(player.active_plugins) > 1:
                menu = views.get_contextmenu('multipleplugins', player.active_plugins)
            else:
                menu = views.get_contextmenu('singleplugin', mp)
        else:
            conns = self.get_connections_at((mx, my))
            if conns:
                # metaplugin, index = res
                menu = views.get_contextmenu('connection', conns)
            else:
                (x, y) = self.pixel_to_float((mx, my))
                menu = views.get_contextmenu('router', x, y)
        
        menu.easy_popup(self, event)


    def float_to_pixel(self, xy):
        """
        Converts a router coordinate to an on-screen pixel coordinate.

        @param xy: tuple (x,y) coordinate in float
        @return: tuple (x, y) coordinate in pixels.
        """
        return (xy[0] + 1) * self.area.width / 2, (xy[1] + 1) * self.area.height / 2


    def pixel_to_float(self, xy):
        """
        Converts an on-screen pixel coordinate to a router coordinate.

        @param x: X coordinate.
        @type x: int
        @param y: Y coordinate.
        @type y: int
        @return: A tuple returning the router coordinate.
        @rtype: (float, float)
        """
        return (2 * xy[0] / self.area.width) - 1, (2 * xy[1] / self.area.height) - 1

    
    def get_audio_connection_at(self, xy):
        """
        @param xy coordinate in pixels
        @return tuple (target_plugin, connection_index)
        """
        conns = self.get_connections_at(xy, (zzub.zzub_connection_type_audio,))

        if len(conns) == 0:
            return None
        
        if len(conns) == 1:
            return conns[0][:2]

        #TODO: implement a better way to select the connection
        return conns[0][:2]

    
    def get_connections_at(self, xy, types = (zzub.zzub_connection_type_audio, zzub.zzub_connection_type_cv)):
        """
        Finds all connections at a specific position. 
        By default it only matches audio + cv connections

        @param xy: (x, y) coordinate in pixels.
        @param types: a list of zzub connection types to match
        @return: a list of tuples [(target_plugin, connection_index, connection_type), (target_plugin, connection_index, connection_type), ...]
        """
        player = components.get('neil.core.player')

        matches = []
        for tplugin in player.get_plugin_list():
            tpos = self.float_to_pixel(tplugin.get_position()) # target plugin

            for index in range(tplugin.get_input_connection_count()):
                splugin = tplugin.get_input_connection_plugin(index) # source plugin
                spos = self.float_to_pixel(splugin.get_position()) 
                conn_type = tplugin.get_input_connection_type(index)

                if conn_type not in types:
                    continue

                if distance_from_line(spos, tpos, xy) < 13.5:
                    matches.append((tplugin, index, conn_type))

        return matches


    def get_plugin_at(self, xy) -> tuple[zzub.Plugin, tuple[int, int], int]:
        """
        Finds a plugin at a specific position.

        @param x: X coordinate in pixels.
        @type x: int
        @param y: Y coordinate in pixels.
        @type y: int
        @return: A connection item, exact pixel position and area (AREA_ANY, AREA_PANNING, AREA_LED) or None.
        @rtype: (zzub.Plugin,(int,int),int) or None
        """
        (x, y) = xy
        mx, my = x, y
        PW, PH = router_sizes.get('pluginwidth') / 2, router_sizes.get('pluginheight') / 2
        area = AREA_ANY
        player = components.get('neil.core.player')
        for mp in reversed(list(player.get_plugin_list())):
            pi = common.get_plugin_infos().get(mp)
            if not pi.songplugin:
                continue
            x,y = self.float_to_pixel(mp.get_position())
            plugin_box = (x - PW, y - PH, x + PW, y + PH)

            if box_contains(mx, my, plugin_box):
                led_tl_pos = (plugin_box[0] + router_sizes.get('ledofsx'), plugin_box[1] + router_sizes.get('ledofsy'))
                led_br_pos = (led_tl_pos[0] + router_sizes.get('ledwidth'), led_tl_pos[1] + router_sizes.get('ledheight'))
                if box_contains(mx, my, [*led_tl_pos, *led_br_pos]):
                    area = AREA_LED
                return mp, (x, y), area

    def on_left_dclick(self, widget, event):
        """
        Event handler for left doubleclicks. If the doubleclick
        hits a plugin, the parameter window is being shown.
        """
        player = components.get('neil.core.player')
        mx, my = int(event.x), int(event.y)
        res = self.get_plugin_at((mx, my))
        if not res:
            searchwindow = components.get('neil.core.searchplugins')
            searchwindow.show_all()
            searchwindow.present()
            return
        mp, (x, y), area = res
        if area == AREA_ANY:
            data = zzub.zzub_event_data_t()

            event_result = mp.invoke_event(data, 1)
            # plugins with custom guis are opened by the double click event
            if not mp.get_flags() & zzub.zzub_plugin_flag_has_custom_gui:
                components.get('neil.core.parameterdialog.manager').show(mp, self)

    def on_left_down(self, widget, event):
        """
        Event handler for left mouse button presses. Initiates
        plugin dragging or connection volume adjustments.

        @param event: Mouse event.
        @type event: wx.MouseEvent
        """
        self.grab_focus()
        player = components.get_player()
        if (event.button == 3):
            return self.on_context_menu(widget, event)

        if not event.button in (1, 2):
            return

        if (event.button == 1) and (event.type == Gdk.EventType._2BUTTON_PRESS):
            self.get_window().set_cursor(None)
            return self.on_left_dclick(widget, event)
            
        mx, my = int(event.x), int(event.y)
        res = self.get_plugin_at((mx, my))
        if res:
            mp, (x, y), area = res
            if area == AREA_LED:
                player.toggle_mute(mp)
                self.redraw()
            else:
                if not mp in player.active_plugins:
                    if (event.get_state() & Gdk.ModifierType.SHIFT_MASK):
                        player.active_plugins = [mp] + player.active_plugins
                    else:
                        player.active_plugins = [mp]
                if not mp in player.active_patterns and is_a_generator(mp):
                    if (event.get_state() & Gdk.ModifierType.SHIFT_MASK):
                        player.active_patterns = [(mp, 0)] + player.active_patterns
                    else:
                        player.active_patterns = [(mp, 0)]
                player.set_midi_plugin(mp)
                if (event.get_state() & Gdk.ModifierType.CONTROL_MASK) or (event.button == 2):
                    if is_controller(mp):
                        pass
                    else:
                        self.connectpos = int(mx), int(my)
                        self.connecting_alt = event.get_state() & Gdk.ModifierType.MOD1_MASK
                        self.get_window().set_cursor(Gdk.Cursor.new(Gdk.CursorType.CROSSHAIR))
                        self.connecting = True
                if not self.connecting:
                    # self.dragstart = (mx, my)
                    self.dragoffset = (x - mx, y - my)
                    for plugin in player.active_plugins:
                        pinfo = self.get_plugin_info(plugin)
                        pinfo.dragpos = plugin.get_position()
                        px, py = self.float_to_pixel(pinfo.dragpos)
                        pinfo.dragoffset = px - mx, py - my

                    self.adjust_draw_led_timer(100)
                    self.dragging = True
                    self.get_window().set_cursor(Gdk.Cursor.new(Gdk.CursorType.FLEUR))
                    self.grab_add()
        else:
            conn_at = self.get_audio_connection_at((mx, my))

            if not conn_at:
                return
            
            mp, index = conn_at
            (ret, ox, oy) = self.get_window().get_origin()
            if mp.get_input_connection_type(index) == zzub.zzub_connection_type_audio:
                self.volume_slider.display((ox + mx, oy + my), mp, index, my)

    def drag_update(self, x, y, state):
        player = components.get('neil.core.player')
        ox, oy = self.dragoffset
        mx, my = int(x), int(y)
        x, y = (mx - ox, my - oy)
        if (state & Gdk.ModifierType.CONTROL_MASK):
            # quantize position
            x = int(float(x) / router_sizes('quantizex') + 0.5) * router_sizes('quantizex')
            y = int(float(y) / router_sizes('quantizey') + 0.5) * router_sizes('quantizey')
        for plugin in player.active_plugins:
            pinfo = self.get_plugin_info(plugin)
            dx, dy = pinfo.dragoffset
            pinfo.dragpos = self.pixel_to_float((x + dx, y + dy))
        
        self.redraw(True)


    def on_motion(self, widget, event):
        if self.dragging:
            self.drag_update(event.x, event.y, event.state)
        elif self.connecting:
            self.connectpos = int(event.x), int(event.y)
            # if False: connect as stereo audio. if True: connect as control port
            self.connecting_alt = event.state & Gdk.ModifierType.MOD1_MASK
            self.redraw()
        else:
            res = self.get_plugin_at((event.x, event.y))
            if res:
                mp, (mx, my), area = res
                self.get_window().set_cursor(Gdk.Cursor.new(Gdk.CursorType.HAND1) if area == AREA_LED else None)

        return False


    # called by on_left_up after alt plugin connection 
    def choose_cv_connectors_dialog(self, from_plugin, to_plugin):
        from contextmenu import ConnectDialog
        dialog = ConnectDialog(self, from_plugin, to_plugin)
        response = dialog.run()

        if response == Gtk.ResponseType.OK:
            source, target = dialog.get_connectors()
        else:
            source = target = False

        data = dialog.get_cv_data()
        print("chosen connector", data)

        dialog.destroy()

        return (source, target, data)


    def on_left_up(self, widget, event):
        """
        Event handler for left mouse button releases.

        @param event: Mouse event.
        @type event: wx.MouseEvent
        """
        mx, my = int(event.x), int(event.y)
        player = components.get_player()

        if self.dragging:
            self.dragging = False
            self.get_window().set_cursor(None)
            self.grab_remove()
            self.grab_focus()
            ox, oy = self.dragoffset
            size = self.get_allocation()
            x, y = max(0, min(mx - ox, size.width)), max(0, min(my - oy, size.height))
            if (event.get_state() & Gdk.ModifierType.CONTROL_MASK):
                # quantize position
                x = int(float(x) / router_sizes.get('quantizex') + 0.5) * router_sizes.get('quantizex')
                y = int(float(y) / router_sizes.get('quantizey') + 0.5) * router_sizes.get('quantizey')
            for plugin in player.active_plugins:
                pinfo = self.get_plugin_info(plugin)
                dx, dy = pinfo.dragoffset
                plugin.set_position(*self.pixel_to_float((dx + x, dy + y)))
            player.history_commit("move plugin")

            self.redraw(True)
            self.adjust_draw_led_timer(200)

        if self.connecting:
            res = self.get_plugin_at((mx, my))

            if res:
                mp, (x, y), area = res
                active_plugins = player.get_active_plugins()
                
                if active_plugins:
                    if event.get_state() & Gdk.ModifierType.MOD1_MASK:
                        (source_connector, target_connector, data) = self.choose_cv_connectors_dialog(active_plugins[0], mp)

                        if source_connector and target_connector and data:
                            print("cv connector data", data, data.amp, data.modulate_mode, data.offset_before, data.offset_after)
                            mp.add_cv_connector(active_plugins[0], source_connector, target_connector, data)
                            player.history_commit("new cv connection")
                            
                    elif not is_controller(active_plugins[0]):
                        mp.add_input(active_plugins[0], zzub.zzub_connection_type_audio)
                        player.history_commit("new event connection")

            self.connecting = False
            self.connecting_alt = False

            self.redraw(True)

        res = self.get_plugin_at((mx, my))

        if res:
            mp, (x, y), area = res
            if area == AREA_LED:
                self.get_window().set_cursor(Gdk.Cursor.new(Gdk.CursorType.HAND1))
        else:
            self.get_window().set_cursor(None)

    def update_all(self):
        self.update_colors()
        self.redraw()

    def on_draw_led_timer(self):
        """
        Timer event that used to only update the plugin leds 5 times a second. 
        Temporarily fix for on-motion handlers being laggy for unknown reasons
        """
        if self.dragging:
            self.prepare_drag_update()
            return True
        
        if self.connecting:
            self.redraw()

        # TODO: find some other way to find out whether we are really visible
        #if self.rootwindow.get_current_panel() != self.panel:
        #       return True
        # TODO: find a better way
        if self.is_visible():
            player = components.get('neil.core.player')
            
            PW, PH = router_sizes.get('pluginwidth') / 2, router_sizes.get('pluginheight') / 2
            for mp, (rx, ry) in ((mp, self.float_to_pixel(mp.get_position())) for mp in player.get_plugin_list()):
                rx, ry = rx - PW, ry - PH
                rect = Gdk.Rectangle(int(rx), int(ry), int(router_sizes.get('pluginwidth')), int(router_sizes.get('pluginheight')))

#                self.get_window().invalidate_rect(rect, False)
        return True
    
    # dragging plugins was very jumpy, this is a backup till i figure out the proper fix
    def prepare_drag_update(self):
        display = Gdk.Display.get_default()
        seat = display.get_default_seat()
        device = seat.get_pointer()

        pos = self.get_window().get_device_position(device)

        self.drag_update(pos.x, pos.y, pos.mask)

        # self.translate_coordinates(parent, position[0], position[1], self.dragoffset)

    def redraw(self, full_refresh=False):
        if full_refresh:
            self.surface = None

        if self.get_window():
            alloc_rect = self.get_allocation()
            rect = Gdk.Rectangle(0, 0, alloc_rect.width, alloc_rect.height)
            self.get_window().invalidate_rect(rect, False)
            self.queue_draw()

    def draw_leds(self, ctx, pango_layout):
        """
        Draws only the leds into the offscreen buffer.
        """
        player = components.get('neil.core.player')
        if player.is_loading():
            return

        cfg = config.get_config()
        plugin_width = int(router_sizes.get('pluginwidth'))
        plugin_height = int(router_sizes.get('pluginheight'))
        half_width, half_height = plugin_width / 2, plugin_height / 2

        driver = components.get('neil.core.driver.audio')
        cpu_scale = driver.get_cpu_load()
        max_cpu_scale = 1.0 / player.get_plugin_count()
        for mp, (rx, ry) in ((mp, self.float_to_pixel(mp.get_position())) for mp in player.get_plugin_list()):
            pi = common.get_plugin_infos().get(mp)

            if not pi or not pi.songplugin:
                continue

            if self.dragging and mp in player.active_plugins:
                # pinfo = self.get_plugin_info(mp)
                rx, ry = self.float_to_pixel(pi.dragpos)
                
                # print("drag pos in draw %.2f %.2f" % self.float_to_pixel(pinfo.dragpos), "%.2f %.2f" % pinfo.dragpos)
                # print("drag pos in draw %.2f %.2f" % pi.dragpos, "%.2f %.2f" % self.float_to_pixel(pi.dragpos))

            rx, ry = rx - half_width, ry - half_height

            # default_brushes = self.plugintype2brushes[PluginType.Generator];
            brushes = self.plugintype2brushes.get(get_plugin_type(mp), self.default_brushes)

            def flag2col(flag):
                return brushes[flag]

            if pi.plugingfx:
                pluginctx = cairo.Context(pi.plugingfx)
            else:
                pi.plugingfx = cairo.ImageSurface(
                    cairo.Format.ARGB32,
                    plugin_width,
                    plugin_height
                )

                pluginctx = cairo.Context(pi.plugingfx)

                # adjust colour for muted plugins
                color = brushes[self.COLOR_MUTED if pi.muted else self.COLOR_DEFAULT]
                pluginctx.set_source_rgb(*color)
#                if pi.muted:
#                    gc.set_foreground(cm.alloc_color(brushes[self.COLOR_MUTED]))
#                else:
#                    gc.set_foreground(cm.alloc_color(brushes[self.COLOR_DEFAULT]))
                pluginctx.rectangle( 0, 0, plugin_width, plugin_height )
                pluginctx.fill()

                # outer border
                pluginctx.set_source_rgb(*flag2col(self.COLOR_BORDER_OUT))
                pluginctx.rectangle( 0, 0, plugin_width, plugin_height )
                pluginctx.stroke()

                #  inner border
                border = blend_float(color, (1, 1, 1), 0.65)
                pluginctx.set_source_rgb(*border)
                pluginctx.rectangle( 1, 1, plugin_width - 2, plugin_height - 2 )
                pluginctx.stroke()

                if (player.solo_plugin and player.solo_plugin != mp
                    and is_instrument(mp)):
                    title = prepstr('[' + mp.get_name() + ']')
                elif pi.muted or pi.bypassed:
                    title = prepstr('(' + mp.get_name() + ')')
                else:
                    title = prepstr(mp.get_name())

                pango_layout.set_markup("<small>%s</small>" % title)
                lw, lh = pango_layout.get_pixel_size()
                half_lw, half_lh = lw / 2, lh / 2
                if mp in player.active_plugins:
                    pluginctx.set_source_rgb(*flag2col(self.COLOR_BORDER_SELECT))
                    pluginctx.rectangle(
                        half_width - lw / 2 - 3,
                        half_height - half_lh,
                        lw + 6,
                        lh)
                    pluginctx.stroke()

                pencol = flag2col(self.COLOR_MUTED if pi.muted else self.COLOR_DEFAULT)
                blendedpen = blend_float(pencol, (1,1,1), 0.7)
                pluginctx.set_source_rgb(*blendedpen)
                pluginctx.move_to(
                    half_width - half_lw + 1, 
                    half_height - half_lh + 1
                )
                PangoCairo.show_layout(pluginctx, pango_layout)

                pluginctx.set_source_rgb(*flag2col(self.COLOR_TEXT))
                pluginctx.move_to(
                    half_width - half_lw, 
                    half_height - half_lh
                )
                PangoCairo.show_layout(pluginctx, pango_layout)

            if config.get_config().get_led_draw() == True:
                # led border
                col = flag2col(self.COLOR_MUTED if pi.muted else self.COLOR_DEFAULT)
                border = blend_float(col, [0,0,0], 0.5)
                pluginctx.set_source_rgb(*border)
                pluginctx.rectangle(
                    int(router_sizes.get('ledofsx')), 
                    int(router_sizes.get('ledofsy')), 
                    int(router_sizes.get('ledwidth')) - 1, 
                    int(router_sizes.get('ledheight')) - 1
                )
                pluginctx.stroke()

                maxl, maxr = mp.get_last_peak()
                amp = min(max(maxl, maxr), 1.0)
                if amp != amp:   # occasionally getting a nan during startup
                    amp = 0
                if amp != pi.amp:
                    if amp >= 1:
                        pluginctx.set_source_rgb(*brushes[self.COLOR_LED_WARNING])
                        pluginctx.rectangle(
                            int(router_sizes.get('ledofsx')) + 1, 
                            int(router_sizes.get('ledofsy')) + 1, 
                            int(router_sizes.get('ledwidth')) - 2, 
                            int(router_sizes.get('ledheight')) - 2
                        )
                        pluginctx.fill()
                    else:
                        pluginctx.set_source_rgb(*brushes[self.COLOR_LED_OFF])
                        pluginctx.rectangle(
                            int(router_sizes.get('ledofsx')), 
                            int(router_sizes.get('ledofsy')), 
                            int(router_sizes.get('ledwidth')), 
                            int(router_sizes.get('ledheight'))
                        )
                        pluginctx.fill()

                        amp = 1.0 - (linear2db(amp, -76.0) / -76.0)
                        height = int((router_sizes.get('ledheight') - 4) * amp + 0.5)
                        if (height > 0):
                            # led fill
                            pluginctx.set_source_rgb(*brushes[self.COLOR_LED_ON])
                            pluginctx.rectangle(
                                int(router_sizes.get('ledofsx')) + 1, 
                                int(router_sizes.get('ledofsy') + router_sizes.get('ledheight')) - height - 1, 
                                int(router_sizes.get('ledwidth')) - 2, 
                                height
                            )
                            pluginctx.fill()
                            # # peak falloff
                            # from collections import deque
                            # if not mp.get_name() in self.peaks:
                            #     self.peaks[mp.get_name()] = deque(maxlen=25)
                            # dq = self.peaks[mp.get_name()]
                            # dq.append(height)

                            # peak_color = cm.alloc_color(blend(cm.alloc_color(brushes[self.COLOR_LED_ON]), Gdk.Color("#fff"), 0.15))
                            # h = LEDOFSY + LEDHEIGHT - 1 - max(height, sum(dq)/len(dq))
                            # gc.set_foreground(peak_color)
                            # pi.plugingfx.draw_line(gc, LEDOFSX + 1, h, LEDOFSX + LEDWIDTH - 2, h)

                    pi.amp = amp

                # relperc = (min(1.0, mp.get_last_cpu_load() / max_cpu_scale) * cpu_scale)
                # if relperc != pi.cpu:
                #     pi.cpu = relperc

                #     # cpu fill
                #     pluginctx.set_source_rgb(*flag2col(self.COLOR_CPU_OFF))
                #     pluginctx.rectangle(CPUOFSX, CPUOFSY, CPUWIDTH, CPUHEIGHT)
                #     pluginctx.fill()

                #     # cpu border
                #     color = brushes[self.COLOR_MUTED if pi.muted else self.COLOR_DEFAULT]
                #     border = blend_float(color, [0,0,0], 0.5)
                #     pluginctx.set_source_rgb(*border)
                #     pluginctx.rectangle(CPUOFSX, CPUOFSY, CPUWIDTH - 1, CPUHEIGHT - 1)
                #     pluginctx.stroke()

                #     height = int((CPUHEIGHT - 4) * relperc + 0.5)
                #     if (height > 0):
                #         if relperc >= 0.9:
                #             pluginctx.set_source_rgb(*flag2col(self.COLOR_CPU_WARNING))
                #         else:
                #             pluginctx.set_source_rgb(*flag2col(self.COLOR_CPU_ON))
                #         pluginctx.rectangle(CPUOFSX + 1, (CPUOFSY + CPUHEIGHT - height - 1), CPUWIDTH - 2, height)
                #         pluginctx.fill()

            # shadow
            pluginctx.set_source_rgba(0.0, 0.0, 0.0, 0.2)
            pluginctx.rectangle(
                rx + 3, 
                ry + 3, 
                plugin_width, 
                plugin_height
            )
            pluginctx.fill()

                # flip plugin pixmap to screen
            ctx.set_source_surface(pi.plugingfx, int(rx), int(ry))
            ctx.paint()


    def on_draw(self, widget, ctx):
        self.draw(ctx)
        return True
    
    def normalize_amp(self, amp):
        amp /= 16384.0
        amp = amp ** 0.5
        return amp
                    
    def draw_connections(self, ctx, cfg):
        ctx.translate(0.5, 0.5)
        ctx.set_line_width(1)
        player = components.get_player()
        for mp in player.get_plugin_list():
            rx,ry = self.float_to_pixel(mp.get_position())

            if self.dragging and mp in player.active_plugins:
                pinfo = self.get_plugin_info(mp)
                rx, ry = self.float_to_pixel(pinfo.dragpos)

            for index in range(mp.get_input_connection_count()):
                target_mp = mp.get_input_connection_plugin(index)
                conn_type = mp.get_input_connection_type(index)

                pi = common.get_plugin_infos().get(target_mp)
                if not pi.songplugin:
                    continue

                t_pos = target_mp.get_position()
                if self.dragging and target_mp in player.active_plugins:
                    pinfo = self.get_plugin_info(target_mp)
                    t_pos = pinfo.dragpos

                crx, cry = self.float_to_pixel(t_pos)

                if (conn_type == zzub.zzub_connection_type_cv):
                    draw_wavy_line(ctx, self.arrowcolors[zzub.zzub_connection_type_cv][0], int(crx), int(cry), int(rx), int(ry))
                else:
                    amp = self.normalize_amp(mp.get_parameter_value(0, index, 0))
                    blended = blend_float(cfg.get_float_color("MV Arrow"), (0.0, 0.0, 0.0), amp)
                    self.arrowcolors[zzub.zzub_connection_type_audio][0] = blended
                    draw_line_arrow(ctx, self.arrowcolors[mp.get_input_connection_type(index)], int(crx), int(cry), int(rx), int(ry), cfg)

        ctx.translate(-0.5, -0.5)

    
    def draw(self, ctx):
        """
        Draws plugins, connections and arrows to an offscreen buffer.
        """
        player = components.get('neil.core.player')
        if player.is_loading():
            return

        cfg = config.get_config()

        # rect = self.get_allocation()
        # w, h = rect.width, rect.height

        # arrowcolors = {
        #         zzub.zzub_connection_type_audio: [
        #                 cfg.get_float_color("MV Arrow"),
        #                 cfg.get_float_color("MV Arrow Border In"),
        #                 cfg.get_float_color("MV Arrow Border Out"),
        #         ],
        #         zzub.zzub_connection_type_event: [
        #                 cfg.get_float_color("MV Controller Arrow"),
        #                 cfg.get_float_color("MV Controller Arrow Border In"),
        #                 cfg.get_float_color("MV Controller Arrow Border Out"),
        #         ],
        # }

        # cx, cy = w * 0.5, h * 0.5


        pango_layout = Pango.Layout(self.get_pango_context())
        #~ layout.set_font_description(self.fontdesc)
        pango_layout.set_width(-1)

        if not self.surface:
            w = self.get_allocated_width()
            h = self.get_allocated_height()
            self.surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, self.area.width, self.area.height)
            surfctx = cairo.Context(self.surface)

            bg_color = cfg.get_float_color('MV Background')
            surfctx.set_source_rgb(*bg_color)
            surfctx.rectangle(0, 0, w, h)
            surfctx.fill()

            self.draw_connections(surfctx, cfg)

            self.draw_leds(surfctx, pango_layout)

        ctx.set_source_surface(self.surface, 0.0, 0.0)
        ctx.paint()
        
        if self.connecting and len(player.active_plugins) > 0:
            ctx.set_line_width(1)
            crx, cry = self.float_to_pixel(player.active_plugins[0].get_position())
            rx, ry = self.connectpos
            linepen = cfg.get_float_color("MV Line")

            if self.connecting_alt:
                draw_wavy_line(ctx, linepen, int(crx), int(cry), int(rx), int(ry))
            else:
                draw_line(ctx, linepen, int(crx), int(cry), int(rx), int(ry))


    # This method is not *just* for key-jazz, it handles all key-events in router. Rename?
    def on_key_jazz(self, widget, event, plugin):
        mask = event.get_state()
        kv = event.keyval
        k = Gdk.keyval_name(kv)
        if mask & Gdk.ModifierType.CONTROL_MASK:
            if k == 'Return':
                components.get('neil.core.pluginbrowser', self)
                return
        player = components.get('neil.core.player')
        if not plugin:
            if player.active_plugins:
                plugin = player.active_plugins[0]
            else:
                return
        note = None
        octave = player.octave
        if  k == 'KP_Multiply':
            octave = min(max(octave + 1, 0), 9)
        elif k == 'KP_Divide':
            octave = min(max(octave - 1, 0), 9)
        elif k == 'Delete':
            for plugin in player.active_plugins:
                if not is_root(plugin):
                    player.delete_plugin(plugin)
        elif kv < 256:
            note = key_to_note(kv)
        player.octave = octave
        if note:
            if note not in self.chordnotes:
                self.chordnotes.append(note)
                n = ((note[0] + octave) << 4 | note[1] + 1)
                plugin.play_midi_note(n, 0, 127)

    def on_key_jazz_release(self, widget, event, plugin):
        player = components.get('neil.core.player')
        kv = event.keyval
        mask = event.get_state()

        if not plugin:
            if player.active_plugins:
                plugin = player.active_plugins[0]
            else:
                return

        if kv < 256:
            player = components.get('neil.core.player')
            octave = player.octave
            note = key_to_note(kv)
            if note in self.chordnotes:
                self.chordnotes.remove(note)
                n = ((note[0] + octave) << 4 | note[1] + 1)
                plugin.play_midi_note(zzub.zzub_note_value_off, n, 0)
