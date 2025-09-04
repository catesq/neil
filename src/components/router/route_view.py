import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Gdk, GObject, Pango, PangoCairo

from typing import cast
import cairo
import zzub

import config
from neil.utils import ( 
    is_effect, is_a_generator, has_audio_output, has_output, has_cv_output, is_root, is_instrument, 
    prepstr, linear2db, Vec2, Area, ui
)

from neil import components, views
import neil.common as common
from patterns import key_to_note

from .parameter_dialog import ParameterDialogManager
from .volume_slider import VolumeSlider
from .utils import draw_line, draw_wavy_line, draw_line_arrow, router_sizes
from .ui import AreaType, RouterLayer, ClickedArea
from contextmenu import ConnectDialog

AREA_ANY = 0
AREA_PANNING = int(AreaType.PLUGIN_PAN)
AREA_LED = int(AreaType.PLUGIN_LED)

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
    # dragoffset = 0, 0
    contextmenupos = 0, 0



    def __init__(self, parent):
        """
        Initializer.

        @param rootwindow: Main window.
        @type rootwindow: NeilFrame
        """
        Gtk.DrawingArea.__init__(self)
        
        self.panel              = parent
        self.surface            = None
        self.autoconnect_target = None
        self.chordnotes         = []
        self.volume_slider      = VolumeSlider(self)
        self.selections         = {}                    #selections stored using the remember selection option
        self.area               = Area()                # will be the pixel dimensions allocated to the drawingarea
        self.colors             = ui.Colors(config.get_config()) 

        self.dragoffset         = Vec2(0, 0)           # pixel offset of the mouse click that initiated a drag to the mid point

        self.connecting         = False                 # is a connection being drawn
        self.connecting_alt     = False                 # is the connection audio(not alt) or cv (is alt)

        self.need_drag_update   = False                 # on some systems drag/drop of plugins is very jumpy

        self.last_drop_ts       = 0  # there was a multiple drop bug in drag/drop, ignore several milliseconds


        # need screen to be realised for reliable sizes
        self.router_layer       = RouterLayer(router_sizes, self.colors)
        self.locator            = self.router_layer.get_item_locator()


        # self.connect('drag-motion', self.on_drag_motion)
        self.connect('drag-data-received', self.on_drag_data_received)
        self.connect('drag-leave', self.on_drag_leave)
        self.drag_dest_set(Gtk.DestDefaults.ALL, common.PLUGIN_DRAG_TARGETS, Gdk.DragAction.COPY)

        self.add_events(Gdk.EventMask.ALL_EVENTS_MASK)

        self.set_can_focus(True)
        self.connect("draw", self.on_draw)
        self.connect('key-press-event', self.on_key_jazz, None)
        self.connect('key-release-event', self.on_key_jazz_release, None)
        self.connect('configure-event', self.on_configure_event)
        self.connect('realize', self.on_realized)



        self.connect('button-press-event', self.on_click)
        self.connect('button-release-event', self.on_release)
        self.connect('motion-notify-event', self.on_mousemove)
        # self.setup_event_handler(self.router_layer.clickareas)

        if config.get_config().get_led_draw() == True: # pyright: ignore[reportAttributeAccessIssue]
            self.set_overlay_timer()
        
        self.recreate_ui_objects()
        self.update_color_scheme()


    def on_document_loaded(self):
        self.recreate_ui_objects()
        self.redraw()
        

    def recreate_ui_objects(self):
        self.router_layer.populate(components.get_player())

                
    # either build a PluginContainer or a class defined by 'router_item' config


    def on_realized(self, widget):
        self.router_layer.set_parent(widget)
        self.register_eventbus()
        
    def on_configure_event(self, widget, requisition):
        self.update_area()

    def on_zzub_plugin_create(self, plugin):
        # self.router_layer.add_plugin_item(plugin)
        self.redraw(True)
        print("plugin created", plugin.get_id())

    def register_eventbus(self):
        eventbus = components.get_eventbus()
        eventbus.attach('connect', self.on_zzub_redraw_event)
        eventbus.attach('disconnect', self.on_zzub_redraw_event)
        eventbus.attach('plugin_changed', self.on_zzub_plugin_changed)


        eventbus.attach('document_loaded', self.on_document_loaded)
        eventbus.attach('active_plugins_changed', self.on_active_plugins_changed)


    def update_area(self):
        area = self.get_allocation()
        print("router area", area.width, area.height)
        if self.area != area:
            self.area.size.set(area.width, area.height)
            self.router_layer.set_size(Vec2(area.width, area.height))
            self.redraw(True)
        

    # LED's usually redraw 5 fps.
    # Dragging plugins/connections is laggy (recent change, not sure if me or Gtk)
    # Temporarily speedup this timer to 10/20fps when mouse move handler active
    def set_overlay_timer(self, fps = 5):
        if getattr(self, 'ui_timeout', False):
            GObject.source_remove(self.ui_timeout)

        self.ui_timeout = GObject.timeout_add(1000/fps, self.on_draw_overlay)


    def on_active_plugins_changed(self, *args):
       # player = components.get('neil.core.player')
        common.get_plugin_infos().reset_plugingfx()


    def get_plugin_info(self, plugin) -> common.PluginInfo:
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
        player.plugin_origin = self.router_layer.to_normal_pos_pair(x, y)
        uri = str(data.get_data(), "utf-8")
        conn = None
        plugin = None
        pluginloader = player.get_pluginloader_by_name(uri)

        if not pluginloader:
            return False
        
        # autoconnect if dragged onto an existing audio connection
        conn = self.get_audio_connection_at(x, y) if is_effect(pluginloader) else None

        # autoconnect if dragged onto an existing plugin
        if conn is None:
            clicked = next(self.router_layer.get_plugins_at(x, y), None)

            if clicked:
                # mp, (px, py), area = res
                if is_effect(clicked.object) or is_instrument(clicked.object):
                    plugin = clicked.object
            # res = self.get_plugin_at((x, y))
            # if res:
            #     mp, (px, py), area = res
            #     if is_effect(mp) or is_instrument(mp):
            #         plugin = mp

        metaplugin = player.create_plugin(pluginloader, connection=conn, plugin=plugin)
        print("dragged plugin", metaplugin.get_id())
        self.router_layer.add_plugin_item(metaplugin)
        
        Gtk.drag_finish(context, True, False, time)
        Gdk.drop_finish(context, True, time)

        return True


    def update_color_scheme(self):
        """
        Updates the color scheme.
        """
        self.colors.refresh()

        common.get_plugin_infos().reset_plugingfx()


    def on_zzub_plugin_changed(self, plugin):
        print("plugin changed", plugin.get_id())
        common.get_plugin_infos().reset_plugin(plugin)
        self.redraw(True)


    def on_zzub_redraw_event(self, *args):
        self.redraw(True)


    def on_focus(self):
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


    def on_click(self, widget, event):
        self.grab_focus()

        found = self.locator.get_object_at(event.x, event.y)
        if event.button == Gdk.BUTTON_SECONDARY:
            return self.right_click(event, found)
        
        if event.type == Gdk.EventType._2BUTTON_PRESS: # and event.button == Gdk.BUTTON_PRIMARY:
            return self.double_click(event, found)
        
        if found:
            if found.type & AreaType.CONNECTION:
                return self.click_connection(event, found)
            elif found.type & AreaType.PLUGIN:
                return self.click_plugin(event, found)
        

    # main handler for right button clicks 
    def right_click(self, event, found):
        if not found:
            return self.context_menu_any(event)
        elif found.type & AreaType.PLUGIN:
            return self.context_menu_plugins(event, found.object)
        elif found and found.type & AreaType.CONNECTION:
            return self.context_menu_connection(event)

            


    def double_click(self, event, found):
        if found and found.type == AreaType.PLUGIN:
            return self.double_click_plugin(event, found)
        else:
            return self.double_click_not_plugin(event)
  

    def click_connection(self, event, found):
        if found.type == AreaType.CONNECTION_ARROW:
            return self.click_volume(event, found.object)


    def click_plugin(self, event, found):
        self.click_set_active_plugin(event, found.object)

        start_connect = (event.button == Gdk.BUTTON_MIDDLE or (event.get_state() & Gdk.ModifierType.CONTROL_MASK))
        print("start connect:", str(start_connect), ", has_output:", str(has_output(found.object)))
        if start_connect and has_output(found.object):
            return self.click_start_connecting(event, found.object)
        else:
            self.click_start_dragging(event, found)


 


    def context_menu_plugins(self, event, plugin):
        print("context menu plugins")
        player = components.get_player()

        if plugin in player.active_plugins and len(player.active_plugins) > 1:
            menu = views.get_contextmenu('multipleplugins', player.active_plugins)
        else:
            menu = views.get_contextmenu('singleplugin', plugin)

        menu.easy_popup(self, event)
        return True


    def context_menu_connection(self, event):
        player = components.get_player()
        connection_items = self.locator.get_all_type_at(event.x, event.y, AreaType.CONNECTION)

        # the connection submenu expects: tuple(target_plugin, connection_index, connection_type)
        connections = [(
            player.get_plugin(item.id.target_id),
            item.id.conn_index,
            item.object.get_type()
        ) for item in connection_items]

        print("context menu connection", connections)
        views.get_contextmenu('connection', connections).easy_popup(self, event)
        return True


    def context_menu_any(self, event):
        print("context menu any")
        (x, y) = self.router_layer.to_normal_pos_pair(event.x, event.y)
        print("normalised screen pos", x, y)
        views.get_contextmenu('router', x, y, self, components.get_player()).easy_popup(self, event)


    def double_click_plugin(self, widget, plugin):
        data = zzub.zzub_event_data_t()
        print("double click plugin")
        event_result = plugin.invoke_event(data, 1)
        # plugins with custom guis are opened by the double click event
        if not plugin.get_flags() & zzub.zzub_plugin_flag_has_custom_gui:
            cast(ParameterDialogManager, views.get_view('parameterdialog.manager')).show_plugin(plugin, self) 


    def double_click_not_plugin(self, event):
        print("double click not plugin")
        searchwindow = views.get_dialog('searchplugins')
        searchwindow.show_all()
        searchwindow.present()


    def click_mute(self, event, plugin: zzub.Plugin):
        print("MUTE")
        components.get_player().toggle_mute(plugin)
        self.redraw()


    def click_pan(self, event, plugin: zzub.Plugin):
        print("PANNING")


    def click_volume(self, event, plugin: zzub.Plugin):
        print("VOLUME CLICK")


    def click_set_active_plugin(self, event, plugin: zzub.Plugin):
        player = components.get_player()
        if not plugin in player.active_plugins:
            if (event.get_state() & Gdk.ModifierType.SHIFT_MASK):
                player.active_plugins = [plugin] + player.active_plugins
            else:
                player.active_plugins = [plugin]
        if not plugin in player.active_patterns and is_a_generator(plugin):
            if (event.get_state() & Gdk.ModifierType.SHIFT_MASK):
                player.active_patterns = [(plugin, 0)] + player.active_patterns
            else:
                player.active_patterns = [(plugin, 0)]
        player.set_midi_plugin(plugin)


    def click_start_connecting(self, event, plugin):
        player = components.get_player()
        self.connectpos = int(event.x), int(event.y)
        self.connecting_alt = event.get_state() & Gdk.ModifierType.MOD1_MASK
        self.get_window().set_cursor(Gdk.Cursor.new(Gdk.CursorType.CROSSHAIR)) # type: ignore reportOptionalMemberAccess
        self.connecting = True
        self.router_layer.add_drag_connection(event.x, event.y, player.get_first_active_plugin())
        self.set_overlay_timer(10)
        self.grab_add()
        return True


    def click_start_dragging(self, event, found):
        self.dragging = True

        player = components.get_player()
        self.dragoffset = found.rect.get_mid() - event

        for plugin in player.active_plugins:
            pinfo = self.get_plugin_info(found.object)
            pinfo.dragpos = Vec2(*plugin.get_position())
            screen_pos = self.router_layer.to_screen_pos(pinfo.dragpos)
            pinfo.dragoffset = screen_pos - event

        self.drag_pointer = self.get_drag_pointer()
        self.get_window().set_cursor(Gdk.Cursor.new(Gdk.CursorType.FLEUR)) # type: ignore reportOptionalMemberAccess
        self.grab_add()
        self.set_overlay_timer(10)

    def get_drag_pointer(self):
        display = Gdk.Display.get_default()
        seat = display.get_default_seat()   #type: ignore reportOptionalMemberAccess
        return seat.get_pointer()         #type: ignore reportOptionalMemberAccess


    def drag_update(self, x, y, state):
        drop_pos = Vec2(x, y) - self.dragoffset

        if (state & Gdk.ModifierType.CONTROL_MASK):
            scaled = router_sizes['plugin'] * self.router_layer.get_zoom()  # type: ignore
            # quantize position
            drop_pos = (drop_pos / scaled + 0.5).ints() * scaled
            x = int(x / scaled.x + 0.5) * scaled.x
            y = int(y / scaled.y + 0.5) * scaled.y

        for plugin in components.get_player().active_plugins:
            pinfo = self.get_plugin_info(plugin)
            pinfo.dragpos = self.router_layer.to_normal_pos(drop_pos + pinfo.dragoffset)
            self.router_layer.move_plugin(plugin, pinfo.dragpos)

        self.redraw(True)


    def on_mousemove(self, widget, event):
        if self.dragging:
            return self.on_motion_drag(event)
        elif self.connecting:
            return self.on_motion_connecting(event)
        
        found = self.locator.get_object_group_at(event.x, event.y, AreaType.PLUGIN)
        if found:
            self.on_motion_over_plugin(event, found)
        else:
            self.on_motion_elsewhere(event)
    

    def on_motion_drag(self, event):
        self.drag_update(event.x, event.y, event.state)
        return True


    def on_motion_connecting(self, event):
        # print("on_motion_connecting")
        self.connectpos = int(event.x), int(event.y)
        self.connecting_alt = event.state & Gdk.ModifierType.MOD1_MASK
        self.router_layer.update_drag_connection(event.x, event.y)
        return True

    def set_cursor(self, new_cursor):
        window = self.get_window()

        if window:
            window.set_cursor(new_cursor)

    def on_motion_over_plugin(self, event, result: ClickedArea):
        # print("on_motion_over_plugin")
        if result.type == AreaType.PLUGIN_LED:
            self.set_cursor(Gdk.Cursor.new(Gdk.CursorType.HAND1))
        else:
            self.set_cursor(None)


    def on_motion_elsewhere(self, event):
        self.redraw()


    def on_release(self, widget, event):
        self.set_overlay_timer()

        if self.connecting:
            return self.on_release_connecting(event)
        elif self.dragging:
            return self.on_release_dragging(event)
        elif self.locator.get_object_at(event.x, event.y):
             self.set_cursor(Gdk.Cursor.new(Gdk.CursorType.HAND1))
        else:
            self.set_cursor(None)

        self.redraw(True)
  

    def on_release_dragging(self, event):
        self.dragging = False
        self.set_cursor(None)
        self.redraw()
        self.grab_remove()

        for plugin in components.get_player().active_plugins:
            plugin.set_position(*self.get_plugin_info(plugin).dragpos)
            
        components.get_player().history_commit("move plugin")
        self.set_overlay_timer()
        # self.adjust_draw_led_timer()


    def on_release_connecting(self, event):
        self.set_overlay_timer()
        self.router_layer.remove_drag_connection()
        self.redraw()
        self.grab_remove()

        self.connecting = False
        self.connecting_alt = False

        self.set_cursor(None)

        target_item = self.locator.get_object_group_at(event.x, event.y, AreaType.PLUGIN)
        
        source_plugin = components.get_player().get_first_active_plugin()
        if not target_item or not source_plugin:
            return
        
        target_plugin = target_item.object
        if event.get_state() & Gdk.ModifierType.MOD1_MASK:
            if has_cv_output(source_plugin):
                (source_connector, target_connector, data) = self.choose_cv_connectors_dialog(source_plugin, target_plugin)

                if source_connector and target_connector and data:
                    print("cv connector data", data, data.amp, data.modulate_mode, data.offset_before, data.offset_after) # type: ignore
                    target_plugin.add_cv_connector(source_plugin, source_connector, target_connector, data)
                    components.get_player().history_commit("new cv connection")
                    
        elif has_audio_output(source_plugin):
            target_plugin.add_input(source_plugin, zzub.zzub_connection_type_audio)
            components.get_player().history_commit("new audio connection")

        self.recreate_ui_objects()




    def on_release_over_led(self, event, plugin):
        self.set_cursor(Gdk.Cursor.new(Gdk.CursorType.HAND1))


    def on_release_elsewhere(self, event):
        self.set_cursor(None)


    def update_all(self):
        self.update_color_scheme()
        self.redraw()


    def get_audio_connection_at(self, x, y):
        """
        get the first audio connection at location

        @param x 
        @param y 
        @return zzub.Connection
        """
        for conn_item in self.router_layer.get_connections_at(x, y):
            if conn_item.type == zzub.zzub_connection_type_audio:
                return conn_item


    # called by on_left_up after alt plugin connection 
    def choose_cv_connectors_dialog(self, from_plugin: zzub.Plugin, to_plugin):
        """
        open dialog to choose which ports to connect 

        @param from_plugin
        @param to_plugin
        @return (source: zzub.CvNode, target: CvNode, data: CvData)
        """
        dialog = ConnectDialog(self, from_plugin, to_plugin)
        response = dialog.run()

        if response == Gtk.ResponseType.OK:
            source_node, target_node = dialog.get_connectors()
        else:
            source_node = target_node = False

        data = dialog.get_cv_data()

        dialog.destroy()

        return (source_node, target_node, data)


    # FIX doing a full refresh is unnecesary most of the time
    def on_draw_overlay(self):
        """
        Some on-motion handlers were laggy for unknown reasons so forcing an update here
        """
        if self.dragging and self.need_drag_update:
            self.force_drag_update()
            return True
        
        if self.connecting:
            self.redraw()
        # print("on draw led")
        # self.update_led_overlay()

        # TODO: find some other way to find out whether we are really visible
        #if self.rootwindow.get_current_panel() != self.panel:
        #       return True
        # TODO: find a better way
        return True

    

    # dragging plugins was very jumpy, 
    # have to get set a timer and get the pointer position to simulate a motion event
    def force_drag_update(self):
        if not self.drag_pointer:
            return

        pos = self.get_window().get_device_position(device) #type: ignore reportOptionalMemberAccess

        # self.drag_update(pos.x, pos.y, pos.mask)

        # self.translate_coordinates(parent, position[0], position[1], self.dragoffset)


    def redraw(self, full_refresh=False):
        if full_refresh:
            self.router_layer.set_refresh()

        window = self.get_window()
        
        if window:
            alloc = self.get_allocation()
            window.invalidate_rect(alloc, False)
            self.queue_draw()

      

    def on_draw(self, widget, ctx):
        self.router_layer.draw_router(ctx)
        return True
        # self.draw(ctx)
        # return True
    
    def normalize_amp(self, amp):
        amp /= 16384.0
        amp = amp ** 0.5
        return amp
    

    # This method is not *just* for key-jazz, it handles all key-events in router. Rename?
    def on_key_jazz(self, widget, event, plugin):
        mask = event.get_state()
        kv = event.keyval
        k = Gdk.keyval_name(kv)
        if mask & Gdk.ModifierType.CONTROL_MASK:
            if k == 'Return':
                components.get('neil.core.pluginbrowser', self)
                return
        player = components.get_player()
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
        player = components.get_player()
        kv = event.keyval
        mask = event.get_state()

        if not plugin:
            if player.active_plugins:
                plugin = player.active_plugins[0]
            else:
                return

        if kv < 256:
            octave = player.octave
            note = key_to_note(kv)
            if note in self.chordnotes:
                self.chordnotes.remove(note)
                n = ((note[0] + octave) << 4 | note[1] + 1)
                plugin.play_midi_note(zzub.zzub_note_value_off, n, 0)


