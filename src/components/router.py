#encoding: latin-1

# Neil
# Modular Sequencer
# Copyright (C) 2006,2007,2008 The Neil Development Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA.

"""
Provides dialogs and controls to render the plugin view/router and its
associated components.
"""

if __name__ == '__main__':
    import os
    os.system('../../bin/neil-combrowser neil.core.routerpanel')
    raise SystemExit

from typing import Generator
import gi
gi.require_version("Gtk", "3.0")
import neil.com as com
from gi.repository import Gtk, Gdk
from gi.repository import GObject
from gi.repository import Pango, PangoCairo
# from cairo import Context
import cairo
import ctypes


from neil.utils import PluginType
from neil.utils import get_plugin_type
from neil.utils import is_effect, is_generator, is_controller, is_root
from neil.utils import prepstr, db2linear, linear2db, error, new_listview, add_scrollbars
from neil.utils import blend, blend_float, box_contains
from neil.utils import get_window_pointer
import config
import zzub
import neil.common as common
from neil.common import MARGIN, DRAG_TARGETS
from rack import ParameterView
from neil.presetbrowser import PresetView
from patterns import key_to_note


PLUGINWIDTH = 100
PLUGINHEIGHT = 25
LEDWIDTH, LEDHEIGHT = 6, PLUGINHEIGHT - 8         # size of LED
LEDOFSX, LEDOFSY = 4, 4                           # offset of LED
CPUWIDTH, CPUHEIGHT = 6, PLUGINHEIGHT - 8         # size of LED
CPUOFSX, CPUOFSY = PLUGINWIDTH - CPUWIDTH - 4, 4  # offset of LED

ARROWRADIUS = 8

QUANTIZEX = PLUGINWIDTH + ARROWRADIUS * 2
QUANTIZEY = PLUGINHEIGHT + ARROWRADIUS * 2

VOLBARWIDTH = 32
VOLBARHEIGHT = 128
VOLKNOBHEIGHT = 16

AREA_ANY = 0
AREA_PANNING = 1
AREA_LED = 2

def draw_line(bmpctx, linepen, crx, cry, rx, ry):
    vx, vy = (rx - crx), (ry - cry)
    length = (vx * vx + vy * vy) ** 0.5
    if not length:
        return
    vx, vy = vx / length, vy / length
    bmpctx.move_to(crx, cry)
    bmpctx.line_to(rx, ry)
    bmpctx.set_source_rgb(*linepen)
    bmpctx.stroke()

def draw_line_arrow(bmpctx, clr, crx, cry, rx, ry, cfg):
    vx, vy = (rx - crx), (ry - cry)
    length = (vx * vx + vy * vy) ** 0.5
    if not length:
        return
    vx, vy = vx / length, vy / length

    cpx, cpy = crx + vx * (length * 0.5), cry + vy * (length * 0.5)

    def make_triangle(radius):
        ux, uy = vx, vy
        if cfg.get_curve_arrows():
            # bezier curve tangent
            def dp(t, a, b, c, d):
                return -3 * (1 - t) ** 2 * a + 3 * (1 - t) ** 2 * b - 6 * t * (1 - t) * b - 3 * (t ** 2) * c + 6 * t * (1 - t) * c + 3 * (t ** 2) * d
            tx = dp(.5, crx, crx + vx * (length * 0.6), rx - vx * (length * 0.6), rx)
            ty = dp(.5, cry, cry, ry, ry)
            tl = (tx ** 2 + ty ** 2) ** .5
            ux, uy = tx / tl, ty / tl

        t1 = (int(cpx - ux * radius + uy * radius),
                int(cpy - uy * radius - ux * radius))
        t2 = (int(cpx + ux * radius),
                int(cpy + uy * radius))
        t3 = (int(cpx - ux * radius - uy * radius),
                int(cpy - uy * radius + ux * radius))

        return t1, t2, t3

    def draw_triangle(t1, t2, t3):
        bmpctx.move_to(*t1)
        bmpctx.line_to(*t2)
        bmpctx.line_to(*t3)
        bmpctx.close_path()

    tri1 = make_triangle(ARROWRADIUS)

    bmpctx.save()
    bmpctx.translate(-0.5, -0.5)

    bgbrush = cfg.get_float_color("MV Background")
    linepen = cfg.get_float_color("MV Line")

    # curve
    if cfg.get_curve_arrows():
        # bezier curve tanget
        def dp(t, a, b, c, d):
            return -3 * (1 - t) ** 2 * a + 3 * (1 - t) ** 2 * b - 6 * t * (1 - t) * b - 3 * (t ** 2) * c + 6 * t * (1 - t) * c + 3 * (t ** 2) * d
        tx = dp(.5, crx, crx + vx * (length * 0.6), rx - vx * (length * 0.6), rx)
        ty = dp(.5, cry, cry, ry, ry)
        tl = (tx ** 2 + ty ** 2) ** .5
        tx, ty = tx / tl, ty / tl

        # stroke the triangle
        draw_triangle(*tri1)
        bmpctx.set_source_rgb(*[x * 0.5 for x in bgbrush])
        bmpctx.set_line_width(1)
        bmpctx.stroke()

        bmpctx.move_to(crx, cry)
        bmpctx.curve_to(crx + vx * (length * 0.6), cry,
                        rx - vx * (length * 0.6), ry,
                        rx, ry)

        bmpctx.set_line_width(4)
        bmpctx.stroke_preserve()
        bmpctx.set_line_width(2.5)
        bmpctx.set_source_rgb(*clr[0])
        # bmpctx.set_source_rgb(*linepen)
        bmpctx.stroke()

        draw_triangle(*tri1)
        bmpctx.fill()
    # straight line
    else:
        bmpctx.set_line_width(1)
        bmpctx.set_source_rgb(*linepen)
        bmpctx.move_to(crx, cry)
        bmpctx.line_to(rx, ry)
        bmpctx.stroke()

        bmpctx.set_source_rgb(*clr[0])
        draw_triangle(*tri1)
        bmpctx.fill_preserve()
        bmpctx.set_source_rgb(*linepen)
        bmpctx.stroke()

    bmpctx.restore()


class AttributesDialog(Gtk.Dialog):
    """
    Displays plugin atttributes and allows to edit them.
    """
    __neil__ = dict(
            id = 'neil.core.attributesdialog',
            singleton = False,
            categories = [
            ]
    )

    def __init__(self, plugin, parent):
        """
        Initializer.

        @param plugin: Plugin object.
        @type plugin: wx.Plugin
        """
        Gtk.Dialog.__init__(self,
                "Attributes",
                parent.get_toplevel(),
                Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT,
                None
        )
        vbox = Gtk.VBox(False, MARGIN)
        vbox.set_border_width(MARGIN)
        self.plugin = plugin
        self.pluginloader = plugin.get_pluginloader()
        self.resize(300, 200)
        self.attriblist, self.attribstore, columns = new_listview([
                ('Attribute', str),
                ('Value', str),
                ('Min', str),
                ('Max', str),
                ('Default', str),
        ])
        vbox.add(add_scrollbars(self.attriblist))
        hsizer = Gtk.HButtonBox()
        hsizer.set_spacing(MARGIN)
        hsizer.set_layout(Gtk.ButtonBoxStyle.START)
        self.edvalue = Gtk.Entry()
        self.edvalue.set_size_request(50, -1)
        self.btnset = Gtk.Button("_Set")
        self.btnok = self.add_button(Gtk.STOCK_OK, Gtk.ResponseType.OK)
        self.btncancel = self.add_button(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL)
        hsizer.pack_start(self.edvalue, False, True, 0)
        hsizer.pack_start(self.btnset, False, True, 0)
        vbox.pack_start(hsizer, False, True, 0)
        self.attribs = []
        for i in range(self.pluginloader.get_attribute_count()):
            attrib = self.pluginloader.get_attribute(i)
            self.attribs.append(self.plugin.get_attribute_value(i))
            self.attribstore.append([
                    prepstr(attrib.get_name()),
                    "%i" % self.plugin.get_attribute_value(i),
                    "%i" % attrib.get_value_min(),
                    "%i" % attrib.get_value_max(),
                    "%i" % attrib.get_value_default(),
            ])
        self.btnset.connect('clicked', self.on_set)
        self.connect('response', self.on_ok)
        self.attriblist.get_selection().connect('changed', self.on_attrib_item_focused)
        if self.attribs:
            self.attriblist.grab_focus()
            self.attriblist.get_selection().select_path((0,))
        self.get_content_area().add(vbox)
        self.show_all()

    def get_focused_item(self):
        """
        Returns the currently focused attribute index.

        @return: Index of the attribute currently selected.
        @rtype: int
        """
        store, rows = self.attriblist.get_selection().get_selected_rows()
        return rows[0][0]

    def on_attrib_item_focused(self, selection):
        """
        Called when an attribute item is being focused.

        @param event: Event.
        @type event: wx.Event
        """
        v = self.attribs[self.get_focused_item()]
        self.edvalue.set_text("%i" % v)

    def on_set(self, widget):
        """
        Called when the "set" button is being pressed.
        """
        i = self.get_focused_item()
        attrib = self.pluginloader.get_attribute(i)
        try:
            v = int(self.edvalue.get_text())
            assert v >= attrib.get_value_min()
            assert v <= attrib.get_value_max()
        except:
            error(self, "<b><big>The number you entered is invalid.</big></b>\n\nThe number must be in the proper range.")
            return
        self.attribs[i] = v
        iter = self.attribstore.get_iter((i,))
        self.attribstore.set_value(iter, 1, "%i" % v)

    def on_ok(self, widget, response):
        """
        Called when the "ok" or "cancel" button is being pressed.
        """
        if response == Gtk.ResponseType.OK:
            for i in range(len(self.attribs)):
                self.plugin.set_attribute_value(i, self.attribs[i])


class ParameterDialog(Gtk.Dialog):
    """
    Displays parameter sliders for a plugin in a new Dialog.
    """
    __neil__ = dict(
            id = 'neil.core.parameterdialog',
            singleton = False,
            categories = [
            ]
    )

    def __init__(self, manager, plugin, parent):
        Gtk.Dialog.__init__(self, parent=parent.get_toplevel())
        self.plugin = plugin
        self.manager = manager
        self.manager.plugin_dialogs[plugin] = self
        self.paramview = ParameterView(plugin)
        self.set_title(self.paramview.get_title())
        self.get_content_area().add(self.paramview)
        self.connect('destroy', self.on_destroy)
        self.connect('realize', self.on_realize)
        eventbus = com.get('neil.core.eventbus')
        eventbus.zzub_delete_plugin += self.on_zzub_delete_plugin

    def on_realize(self, widget):
        self.set_default_size(*self.paramview.get_best_size())

    def on_zzub_delete_plugin(self, plugin):
        if plugin == self.plugin:
            self.destroy()

    def on_destroy(self, event):
        """
        Handles destroy events.
        """
        del self.manager.plugin_dialogs[self.plugin]


class ParameterDialogManager:
    """
    Manages the different parameter dialogs.
    """
    __neil__ = dict(
            id = 'neil.core.parameterdialog.manager',
            singleton = True,
            categories = [
            ]
    )

    def __init__(self):
        self.plugin_dialogs = {}

    def show(self, plugin, parent):
        """
        Shows a parameter dialog for a plugin.

        @param plugin: Plugin instance.
        @type plugin: Plugin
        """
        dlg = self.plugin_dialogs.get(plugin, None)
        if not dlg:
            dlg = ParameterDialog(self, plugin, parent)
        dlg.show_all()


class PresetDialogManager:
    """
    Manages the different preset dialogs.
    """
    __neil__ = dict(
            id = 'neil.core.presetdialog.manager',
            singleton = True,
            categories = [
            ]
    )

    def __init__(self):
        self.preset_dialogs = {}

    def show(self, plugin, parent):
        """
        Shows a preset dialog for a plugin.

        @param plugin: Plugin instance.
        @type plugin: Plugin
        """
        dlg = self.preset_dialogs.get(plugin, None)
        if not dlg:
            dlg = PresetDialog(self, plugin, parent)
        dlg.show_all()


class PresetDialog(Gtk.Dialog):
    """
    Displays parameter sliders for a plugin in a new Dialog.
    """
    def __init__(self, manager, plugin, parent):
        #GObject.GObject.__init__(self, parent=parent.get_toplevel())
        Gtk.Dialog.__init__(self, parent=com.get('neil.core.window.root'))
        self.plugin = plugin
        self.manager = manager
        self.manager.preset_dialogs[plugin] = self
        self.view = parent
        self.plugin = plugin
        self.presetview = PresetView(self, plugin, self)
        self.set_title(self.presetview.get_title())
        self.get_content_area().add(self.presetview)
        self.connect('realize', self.on_realize)
        eventbus = com.get('neil.core.eventbus')
        eventbus.zzub_delete_plugin += self.on_zzub_delete_plugin

    def on_zzub_delete_plugin(self, plugin):
        if plugin == self.plugin:
            self.destroy()

    def on_destroy(self, event):
        """
        Handles destroy events.
        """
        del self.manager.preset_dialogs[self.plugin]

    def on_realize(self, widget):
        # This is the size specified in presetbrowser.py.
        # Seems to have no effect though -- PresetView is full-screen?
        self.set_default_size(200, 400)



class RoutePanel(Gtk.VBox):
    """
    Contains the view panel and manages parameter dialogs.
    """
    __neil__ = dict(
        id = 'neil.core.routerpanel',
        singleton = True,
        categories = [
                'neil.viewpanel',
                'view',
        ]
    )

    __view__ = dict(
        label = "Router",
        stockid = "neil_router",
        shortcut = 'F3',
        default = True,
        order = 3,
    )

    def __init__(self):
        """
        Initializer.
        """
        Gtk.VBox.__init__(self)
        self.view = com.get('neil.core.router.view', self)
        self.pack_start(self.view, True, True, 0)

    def handle_focus(self):
        self.view.grab_focus()

    def reset(self):
        """
        Resets the router view. Used when
        a new song is being loaded.
        """
        self.view.reset()

    def update_all(self):
        self.view.update_colors()
        self.view.redraw()


class VolumeSlider(Gtk.Window):
    """
    A temporary popup volume control for the router. Can
    only be summoned parametrically and will vanish when the
    left mouse button is being released.
     """
    def __init__(self, parent):
        """
        Initializer.
        """
        self.parent_window = parent
        self.plugin = None
        self.index = -1
        Gtk.Window.__init__(self, Gtk.WindowType.POPUP)
        self.drawingarea = Gtk.DrawingArea()
        self.add(self.drawingarea)
        self.drawingarea.add_events(Gdk.EventMask.ALL_EVENTS_MASK)
        self.drawingarea.set_property('can-focus', True)
        self.resize(VOLBARWIDTH, VOLBARHEIGHT)
        self.hide()
        self.drawingarea.connect('motion-notify-event', self.on_motion)
        self.drawingarea.connect('draw', self.on_draw)
        self.drawingarea.connect('button-release-event', self.on_left_up)

    def redraw(self):
        if self.is_visible() and self.drawingarea.get_window():
            self.queue_draw()

    def on_motion(self, widget, event):
        """
        Event handler for mouse movements.
        """
        x, y, state = (event.x, event.y, event.get_state())
        newpos = int(y)
        delta = newpos - self.y
        if delta == 0:
            return
        self.y = newpos
        self.amp = max(min(self.amp + (float(delta) / VOLBARHEIGHT), 1.0), 0.0)
        amp = min(max(int(db2linear(self.amp * -48.0, -48.0) * 16384.0), 0), 16384)
        self.plugin.set_parameter_value_direct(0, self.index, 0, amp, False)
        self.redraw()
        return True

    def on_draw(self, widget, ctx):
        """
        Event handler for paint requests.
        """
        rect = self.drawingarea.get_allocation()
        w, h = rect.width, rect.height

        cfg = config.get_config()
        whitebrush = cfg.get_float_color('MV Amp BG')
        blackbrush = cfg.get_float_color('MV Amp Handle')
        outlinepen = cfg.get_float_color('MV Amp Border')

        ctx.set_source_rgb(*whitebrush)
        ctx.rectangle(0, 0, w, h)
        ctx.fill()
        ctx.set_source_rgb(*outlinepen)
        ctx.rectangle(0, 0, w - 1, h - 1)
        ctx.fill()

        if self.plugin:
            ctx.set_source_rgb(*blackbrush)
            pos = int(self.amp * (VOLBARHEIGHT - VOLKNOBHEIGHT))
            ctx.rectangle(1, pos + 1, VOLBARWIDTH - 2, VOLKNOBHEIGHT - 2)
            ctx.fill()

        black = (0, 0, 0)
        ctx.set_source_rgb(*black)
        layout = Pango.Layout(self.get_pango_context())
        font = Pango.FontDescription("sans 6")
        layout.set_font_description(font)
        layout.set_markup("<small>%.1f dB</small>" % (self.amp * -48.0))
        ctx.move_to(2, 2)
        PangoCairo.show_layout(ctx, layout)

        return False

    def display(self, xxx_todo_changeme, mp, index, orig_event_y):
        """
        Called by the router view to show the control.

        @param mx: X coordinate of the control center in pixels.
        @type mx: int
        @param my: Y coordinate of the control center in pixels.
        @type my: int
        @param conn: Connection to control.
        @type conn: zzub.Connection
        """
        (mx, my) = xxx_todo_changeme

        self.plugin = mp
        self.index = index
        self.amp = abs((linear2db((self.plugin.get_parameter_value(0, index, 0) / 16384.0), -48.0) / -48.0))
        self.y = orig_event_y
        self.oy = int(my - VOLBARHEIGHT * self.amp)

        self.move(int(mx - VOLBARWIDTH * 0.5), int(my - VOLBARHEIGHT * self.amp))
        self.show_all()

        self.drawingarea.grab_add()


    def on_left_up(self, widget, event):
        """
        Event handler for left mouse button releases. Will
        hide control.

        @param event: Mouse event.
        @type event: wx.MouseEvent
        """
        self.parent_window.redraw()
        self.hide()
        self.drawingarea.grab_remove()



class RouteView(Gtk.DrawingArea):
    """
    Allows to monitor and control plugins and their connections.
    """
    __neil__ = dict(
            id = 'neil.core.router.view',
            singleton = True,
            categories = [
            ]
    )

    current_plugin = None
    connecting = False
    dragging = False
    dragoffset = 0, 0
    contextmenupos = 0, 0
    showing_cvports = False

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
        self.selections                  = {}                    #slections stored using the remember selction option

        eventbus                         = com.get('neil.core.eventbus')
        eventbus.zzub_connect           += self.on_zzub_redraw_event
        eventbus.zzub_disconnect        += self.on_zzub_redraw_event
        eventbus.zzub_plugin_changed    += self.on_zzub_plugin_changed
        eventbus.document_loaded        += self.redraw
        eventbus.active_plugins_changed += self.on_active_plugins_changed

        self.last_drop_ts = 0
        self.connect('drag_motion', self.on_drag_motion)
        self.connect('drag_drop', self.on_drag_drop)
        self.connect('drag_data_received', self.on_drag_data_received)
        self.connect('drag_leave', self.on_drag_leave)
        self.drag_dest_set(Gtk.DestDefaults.ALL, DRAG_TARGETS, Gdk.DragAction.COPY)

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

        if config.get_config().get_led_draw() == True:
            GObject.timeout_add(500, self.on_draw_led_timer)

    def on_active_plugins_changed(self, *args):
       # player = com.get('neil.core.player')
        common.get_plugin_infos().reset_plugingfx()

    def get_plugin_info(self, plugin):
        return common.get_plugin_infos().get(plugin)

    def on_drag_leave(self, widget, context, time):
        #print "on_drag_leave",widget,context,time
        self.drag_unhighlight()

    def on_drag_motion(self, widget, context, x, y, time):
        #print "on_drag_motion",widget,context,x,y,time

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

    def on_drag_drop(self, widget, context, x, y, time):
        if context.list_targets():
            widget.drag_get_data(context, Gdk.Atom.intern('application/x-neil-plugin-uri', False), time)
            return True
        return False


    def on_drag_data_received(self, widget, context, x, y, data, info, time):
        if time == self.last_drop_ts:
            print("TODO fix the duoble drop nonsense")
            return

        self.last_drop_ts = time
        if data.get_format() == 8:
            context.finish(True, True, time)

            player = com.get('neil.core.player')
            player.plugin_origin = self.pixel_to_float((x, y))
            uri = str(data.get_data(), "utf-8")
            conn = None
            plugin = None
            pluginloader = player.get_pluginloader_by_name(uri)
            if is_effect(pluginloader):
                conn = self.get_connection_at((x, y))
            if not conn:
                res = self.get_plugin_at((x, y))
                if res:
                    mp, (px, py), area = res
                    if is_effect(mp) or is_root(mp):
                        plugin = mp
            player.create_plugin(pluginloader, connection=conn, plugin=plugin)
            Gtk.drag_finish(context, True, False, time)
            Gdk.drop_finish(context, True, time)
        else:
            Gtk.drag_finish(context, False, False, time)
            Gdk.drop_finish(context, True, time)
        return True

    def on_configure_event(self, widget, requisition):
        self.redraw()

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
        flagids = [
                (PluginType.Root, 'Master'),
                (PluginType.Generator, 'Generator'),
                (PluginType.Effect, 'Effect'),
                (PluginType.Controller, 'Controller'),
        ]
        self.plugintype2brushes = {}
        for plugintype, name in flagids:
            brushes = []
            for name in [x.replace('${PLUGIN}', name) for x in names]:
                brushes.append(cfg.get_float_color(name))
            self.plugintype2brushes[plugintype] = brushes
        common.get_plugin_infos().reset_plugingfx()

    def on_zzub_plugin_changed(self, plugin):
        common.get_plugin_infos().get(plugin).reset_plugingfx()
        self.redraw()

    def on_zzub_redraw_event(self, *args):
        self.redraw()

    def on_focus(self, event):
        self.redraw()

    def store_selection(self, index, plugins):
        self.selections[index] = [plugin.get_id() for plugin in plugins]

    def restore_selection(self, index):
        if self.has_selection(index):
            player = com.get('neil.core.player')
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
        player = com.get('neil.core.player')
        player.plugin_origin = self.pixel_to_float((mx, my))
        res = self.get_plugin_at((mx, my))
        if res:
            mp, (x, y), area = res
            if mp in player.active_plugins and len(player.active_plugins) > 1:
                menu = com.get('neil.core.contextmenu.multipleplugins', player.active_plugins)
            else:
                menu = com.get('neil.core.contextmenu.singleplugin', mp)
        else:
            res = self.get_connection_at((mx, my))
            if res:
                metaplugin, index = res
                menu = com.get('neil.core.contextmenu.connection', metaplugin, index)
            else:
                (x, y) = self.pixel_to_float((mx, my))
                menu = com.get('neil.core.contextmenu.router', x, y)

        menu.popup(self, event)

    def float_to_pixel(self, xxx_todo_changeme1):
        """
        Converts a router coordinate to an on-screen pixel coordinate.

        @param x: X coordinate.
        @type x: float
        @param y: Y coordinate.
        @type y: float
        @return: A tuple returning the pixel coordinate.
        @rtype: (int,int)
        """
        (x, y) = xxx_todo_changeme1
        rect = self.get_allocation()
        w, h = rect.width, rect.height
        cx, cy = w * 0.5, h * 0.5
        return cx * (1 + x), cy * (1 + y)

    def pixel_to_float(self, xxx_todo_changeme2):
        """
        Converts an on-screen pixel coordinate to a router coordinate.

        @param x: X coordinate.
        @type x: int
        @param y: Y coordinate.
        @type y: int
        @return: A tuple returning the router coordinate.
        @rtype: (float, float)
        """
        (x, y) = xxx_todo_changeme2
        rect = self.get_allocation()
        w, h = rect.width, rect.height
        cx, cy = w * 0.5, h * 0.5
        return (x / cx) - 1, (y / cy) - 1

    def get_connection_at(self, xxx_todo_changeme3):
        """
        Finds the connection arrow at a specific position.

        @param mx: X coordinate in pixels.
        @type mx: int
        @param my: Y coordinate in pixels.
        @type my: int
        @return: A connection item or None.
        @rtype: zzub.Connection or None
        """
        (mx, my) = xxx_todo_changeme3
        player = com.get('neil.core.player')
        rect = self.get_allocation()
        w, h = rect.width, rect.height
        cx, cy = w * 0.5, h * 0.5

        def get_pixelpos(x, y):
            return cx * (1 + x), cy * (1 + y)
        for mp in player.get_plugin_list():
            rx, ry = get_pixelpos(*mp.get_position())
            for index in range(mp.get_input_connection_count()):
                crx, cry = get_pixelpos(*mp.get_input_connection_plugin(index).get_position())
                cpx, cpy = (crx + rx) * 0.5, (cry + ry) * 0.5
                dx, dy = cpx - mx, cpy - my
                length = (dx * dx + dy * dy) ** 0.5
                if length <= 14:  # why exactly 14?
                    return mp, index

    def get_plugin_at(self, xxx_todo_changeme4):
        """
        Finds a plugin at a specific position.

        @param x: X coordinate in pixels.
        @type x: int
        @param y: Y coordinate in pixels.
        @type y: int
        @return: A connection item, exact pixel position and area (AREA_ANY, AREA_PANNING, AREA_LED) or None.
        @rtype: (zzub.Plugin,(int,int),int) or None
        """
        (x, y) = xxx_todo_changeme4
        rect = self.get_allocation()
        w, h = rect.width, rect.height
        cx, cy = w * 0.5, h * 0.5
        mx, my = x, y
        PW, PH = PLUGINWIDTH / 2, PLUGINHEIGHT / 2
        area = AREA_ANY
        player = com.get('neil.core.player')
        for mp in reversed(list(player.get_plugin_list())):
            pi = common.get_plugin_infos().get(mp)
            if not pi.songplugin:
                continue
            x, y = mp.get_position()
            x, y = int(cx * (1 + x)), int(cy * (1 + y))
            plugin_box = (x - PW, y - PH, x + PW, y + PH)

            if box_contains(mx, my, plugin_box):
                led_tl_pos = (plugin_box[0] + LEDOFSX, plugin_box[1] + LEDOFSY)
                led_br_pos = (led_tl_pos[0] + LEDWIDTH, led_tl_pos[1] + LEDHEIGHT)
                if box_contains(mx, my, [*led_tl_pos, *led_br_pos]):
                    area = AREA_LED
                return mp, (x, y), area

    def on_left_dclick(self, widget, event):
        """
        Event handler for left doubleclicks. If the doubleclick
        hits a plugin, the parameter window is being shown.
        """
        #player = com.get('neil.core.player')
        print("dcilck")
        mx, my = int(event.x), int(event.y)
        res = self.get_plugin_at((mx, my))
        if not res:
            searchwindow = com.get('neil.core.searchplugins')
            searchwindow.show_all()
            searchwindow.present()
            return
        mp, (x, y), area = res
        if area == AREA_ANY:
            data = zzub.zzub_event_data_t()

            mp.invoke_event(data, 1)

            if mp.get_pluginloader().get_uri().startswith('@zzub.org/lv2adapter/'):
                ui_opened = mp.get_attribute_value(1)
                if ui_opened == 1: # this attribute only == 1 when a custom ui has just been opened
                    return          # if ui was already open - or no ui open - it == 0

            #when not lv2adapter or lv2 ui already open/not opened then display ParameterDialog
            com.get('neil.core.parameterdialog.manager').show(mp, self)

    def on_left_down(self, widget, event):
        """
        Event handler for left mouse button presses. Initiates
        plugin dragging or connection volume adjustments.

        @param event: Mouse event.
        @type event: wx.MouseEvent
        """
        self.grab_focus()
        player = com.get('neil.core.player')
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
                if not mp in player.active_patterns and is_generator(mp):
                    if (event.get_state() & Gdk.ModifierType.SHIFT_MASK):
                        player.active_patterns = [(mp, 0)] + player.active_patterns
                    else:
                        player.active_patterns = [(mp, 0)]
                player.set_midi_plugin(mp)
                if (event.get_state() & Gdk.ModifierType.CONTROL_MASK) or (event.button == 2):
                    if is_controller(mp):
                        pass
                    else:
                        self.connecting = True
                        self.connectpos = int(mx), int(my)
                        self.get_window().set_cursor(Gdk.Cursor.new(Gdk.CursorType.CROSSHAIR))
                if not self.connecting:
                    for plugin in player.active_plugins:
                        pinfo = self.get_plugin_info(plugin)
                        pinfo.dragpos = plugin.get_position()
                        x, y = self.float_to_pixel(pinfo.dragpos)
                        pinfo.dragoffset = x - mx, y - my
                    self.dragging = True
                    self.get_window().set_cursor(Gdk.Cursor.new(Gdk.CursorType.FLEUR))
                    self.grab_add()
        else:
            res = self.get_connection_at((mx, my))
            if res:
                mp, index = res
                (ret, ox, oy) = self.get_window().get_origin()
                connectiontype = mp.get_input_connection_type(index)
                if connectiontype == zzub.zzub_connection_type_audio:
                    self.volume_slider.display((ox + mx, oy + my), mp, index, my)
                elif connectiontype == zzub.zzub_connection_type_event:
                    # no idea what to do when clicking on an event connection yet
                    pass
            else:
                player.active_plugins = []

    def on_motion(self, widget, event):
        x, y, state = event.x, event.y, event.state
        if self.dragging:
            player = com.get('neil.core.player')
            ox, oy = self.dragoffset
            mx, my = int(x), int(y)
            size = self.get_allocation()
            x, y = max(0, min(mx - ox, size.width)), max(0, min(my - oy, size.height))
            if (event.get_state() & Gdk.ModifierType.CONTROL_MASK):
                # quantize position
                x = int(float(x) / QUANTIZEX + 0.5) * QUANTIZEX
                y = int(float(y) / QUANTIZEY + 0.5) * QUANTIZEY
            for plugin in player.active_plugins:
                pinfo = self.get_plugin_info(plugin)
                dx, dy = pinfo.dragoffset
                pinfo.dragpos = self.pixel_to_float((x + dx, y + dy))
            self.redraw()
        elif self.connecting:
            self.connectpos = int(x), int(y)
            self.redraw()
        else:
            res = self.get_plugin_at((x, y))
            if res:
                mp, (mx, my), area = res
                self.get_window().set_cursor(Gdk.Cursor.new(Gdk.CursorType.HAND1) if area == AREA_LED else None)
        return True

    def on_left_up(self, widget, event):
        """
        Event handler for left mouse button releases.

        @param event: Mouse event.
        @type event: wx.MouseEvent
        """
        mx, my = int(event.x), int(event.y)
        player = com.get('neil.core.player')
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
                x = int(float(x) / QUANTIZEX + 0.5) * QUANTIZEX
                y = int(float(y) / QUANTIZEY + 0.5) * QUANTIZEY
            for plugin in player.active_plugins:
                pinfo = self.get_plugin_info(plugin)
                dx, dy = pinfo.dragoffset
                plugin.set_position(*self.pixel_to_float((dx + x, dy + y)))
            player.history_commit("move plugin")
        if self.connecting:
            res = self.get_plugin_at((mx, my))
            if res:
                mp, (x, y), area = res
                if player.active_plugins:
                    if not is_controller(player.active_plugins[0]):
                        mp.add_input(player.active_plugins[0], zzub.zzub_connection_type_audio)
                        player.history_commit("new connection")
        self.connecting = False
        self.redraw()
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
        Timer event that only updates the plugin leds.
        """
        # TODO: find some other way to find out whether we are really visible
        #if self.rootwindow.get_current_panel() != self.panel:
        #       return True
        # TODO: find a better way
        if self.is_visible():
            player = com.get('neil.core.player')
            rect = self.get_allocation()
            w, h = rect.width, rect.height
            cx, cy = w * 0.5, h * 0.5

            def get_pixelpos(x, y):
                return cx * (1 + x), cy * (1 + y)
            PW, PH = PLUGINWIDTH / 2, PLUGINHEIGHT / 2
            for mp, (rx, ry) in ((mp, get_pixelpos(*mp.get_position())) for mp in player.get_plugin_list()):
                rx, ry = rx - PW, ry - PH
                rect = Gdk.Rectangle(int(rx), int(ry), PLUGINWIDTH, PLUGINHEIGHT)
#                self.get_window().invalidate_rect(rect, False)
        return True

    def redraw(self):
        if self.get_window():
            self.surface = None
            alloc_rect = self.get_allocation()
            rect = Gdk.Rectangle(0, 0, alloc_rect.width, alloc_rect.height)
            self.get_window().invalidate_rect(rect, False)
            self.queue_draw()

    def draw_leds(self, ctx, pango_layout):
        """
        Draws only the leds into the offscreen buffer.
        """
        player = com.get('neil.core.player')
        if player.is_loading():
            return

        cfg = config.get_config()
        rect = self.get_allocation()

        w, h = rect.width, rect.height
        cx, cy = w * 0.5, h * 0.5

        def get_pixelpos(x, y):
            return cx * (1 + x), cy * (1 + y)
        PW, PH = PLUGINWIDTH / 2, PLUGINHEIGHT / 2
        driver = com.get('neil.core.driver.audio')
        cpu_scale = driver.get_cpu_load()
        max_cpu_scale = 1.0 / player.get_plugin_count()
        for mp, (rx, ry) in ((mp, get_pixelpos(*mp.get_position())) for mp in player.get_plugin_list()):
            pi = common.get_plugin_infos().get(mp)

            if not pi.songplugin:
                continue

            if self.dragging and mp in player.active_plugins:
                pinfo = self.get_plugin_info(mp)
                rx, ry = get_pixelpos(*pinfo.dragpos)

            rx, ry = rx - PW, ry - PH
            pi = common.get_plugin_infos().get(mp)

            if not pi:
                continue

            brushes = self.plugintype2brushes.get(
                get_plugin_type(mp),
                self.plugintype2brushes[PluginType.Generator])

            def flag2col(flag):
                return brushes[flag]

            if pi.plugingfx:
                pluginctx = cairo.Context(pi.plugingfx)
            else:
                pi.plugingfx = cairo.ImageSurface(
                    cairo.Format.ARGB32,
                    PLUGINWIDTH,
                    PLUGINHEIGHT
                )

                pluginctx = cairo.Context(pi.plugingfx)

                # adjust colour for muted plugins
                color = brushes[self.COLOR_MUTED if pi.muted else self.COLOR_DEFAULT]
                pluginctx.set_source_rgb(*color)
#                if pi.muted:
#                    gc.set_foreground(cm.alloc_color(brushes[self.COLOR_MUTED]))
#                else:
#                    gc.set_foreground(cm.alloc_color(brushes[self.COLOR_DEFAULT]))
                pluginctx.rectangle(0, 0, PLUGINWIDTH, PLUGINHEIGHT)
                pluginctx.fill()

                # outer border
                pluginctx.set_source_rgb(*flag2col(self.COLOR_BORDER_OUT))
                pluginctx.rectangle(0, 0, PLUGINWIDTH, PLUGINHEIGHT)
                pluginctx.stroke()

                #  inner border
                border = blend_float(color, (1, 1, 1), 0.65)
                pluginctx.set_source_rgb(*border)
                pluginctx.rectangle(1, 1, PLUGINWIDTH - 2, PLUGINHEIGHT - 2)
                pluginctx.stroke()

                if (player.solo_plugin and player.solo_plugin != mp
                    and is_generator(mp)):
                    title = prepstr('[' + mp.get_name() + ']')
                elif pi.muted or pi.bypassed:
                    title = prepstr('(' + mp.get_name() + ')')
                else:
                    title = prepstr(mp.get_name())

                pango_layout.set_markup("<small>%s</small>" % title)
                lw, lh = pango_layout.get_pixel_size()
                if mp in player.active_plugins:
                    pluginctx.set_source_rgb(*flag2col(self.COLOR_BORDER_SELECT))
                    pluginctx.rectangle(
                        PLUGINWIDTH / 2 - lw / 2 - 3,
                        PLUGINHEIGHT / 2 - lh / 2,
                        lw + 6,
                        lh)
                    pluginctx.stroke()

                pencol = flag2col(self.COLOR_MUTED if pi.muted else self.COLOR_DEFAULT)
                blendedpen = blend_float(pencol, (1,1,1), 0.7)
                pluginctx.set_source_rgb(*blendedpen)
                pluginctx.move_to(PLUGINWIDTH / 2 - lw / 2 + 1, PLUGINHEIGHT / 2 - lh / 2 + 1)
                PangoCairo.show_layout(pluginctx, pango_layout)

                pluginctx.set_source_rgb(*flag2col(self.COLOR_TEXT))
                pluginctx.move_to(PLUGINWIDTH / 2 - lw / 2, PLUGINHEIGHT / 2 - lh / 2)
                PangoCairo.show_layout(pluginctx, pango_layout)

            if config.get_config().get_led_draw() == True:
                # led border
                col = flag2col(self.COLOR_MUTED if pi.muted else self.COLOR_DEFAULT)
                border = blend_float(col, [0,0,0], 0.5)
                pluginctx.set_source_rgb(*border)
                pluginctx.rectangle(LEDOFSX, LEDOFSY, LEDWIDTH - 1, LEDHEIGHT - 1)
                pluginctx.stroke()

                maxl, maxr = mp.get_last_peak()
                amp = min(max(maxl, maxr), 1.0)
                if amp != amp:   # occasionally getting a nan during srtartup
                    amp = 0
                if amp != pi.amp:
                    if amp >= 1:
                        pluginctx.set_source_rgb(*brushes[self.COLOR_LED_WARNING])
                        pluginctx.rectangle(LEDOFSX + 1, LEDOFSY + 1, LEDWIDTH - 2, LEDHEIGHT - 2)
                        pluginctx.fill()
                    else:
                        pluginctx.set_source_rgb(*brushes[self.COLOR_LED_OFF])
                        pluginctx.rectangle(LEDOFSX, LEDOFSY, LEDWIDTH, LEDHEIGHT)
                        pluginctx.fill()

                        amp = 1.0 - (linear2db(amp, -76.0) / -76.0)
                        height = int((LEDHEIGHT - 4) * amp + 0.5)
                        if (height > 0):
                            # led fill
                            pluginctx.set_source_rgb(*brushes[self.COLOR_LED_ON])
                            pluginctx.rectangle(LEDOFSX+1, (LEDOFSY + LEDHEIGHT - height - 1), LEDWIDTH-2, height)
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
            pluginctx.rectangle(rx + 3, ry + 3, PLUGINWIDTH, PLUGINHEIGHT)
            pluginctx.fill()

                # flip plugin pixmap to screen
            ctx.set_source_surface(pi.plugingfx, int(rx), int(ry))
            ctx.paint()

            if self.showing_cvports:
                pl = mp.get_pluginloader()
                port_names = pl.get_cv_port_names()
                if not port_names:
                    continue

                ports = pl.get_cv_port_names().split("\n")

                pango_layout.set_text(port_names)
                tx, ty = pango_layout.get_pixel_size()

                ctx.set_source_rgb(*cfg.get_float_color("MV Background"))

                ctx.rectangle(rx+PLUGINWIDTH, ry, tx+3, ty+3)
                ctx.fill()

                ctx.set_source_rgb(*cfg.get_float_color("MV Border"))
                ctx.rectangle(rx+PLUGINWIDTH-1, ry-1, tx+4, ty+4)
                ctx.stroke()

                ctx.move_to(rx+PLUGINWIDTH+1, ry+1)
                ctx.set_source_rgb(*cfg.get_float_color("MV Text"))
                PangoCairo.show_layout(ctx, pango_layout)

    def on_draw(self, widget, ctx):
        self.draw(ctx)
        return False

    def draw(self, ctx):
        """
        Draws plugins, connections and arrows to an offscreen buffer.
        """
        player = com.get('neil.core.player')
        if player.is_loading():
            return

        cfg = config.get_config()
        rect = self.get_allocation()
        w, h = rect.width, rect.height
        arrowcolors = {
                zzub.zzub_connection_type_audio: [
                        cfg.get_float_color("MV Arrow"),
                        cfg.get_float_color("MV Arrow Border In"),
                        cfg.get_float_color("MV Arrow Border Out"),
                ],
                zzub.zzub_connection_type_event: [
                        cfg.get_float_color("MV Controller Arrow"),
                        cfg.get_float_color("MV Controller Arrow Border In"),
                        cfg.get_float_color("MV Controller Arrow Border Out"),
                ],
        }

        cx, cy = w * 0.5, h * 0.5


        pango_layout = Pango.Layout(self.get_pango_context())
        #~ layout.set_font_description(self.fontdesc)
        pango_layout.set_width(-1)

        def get_pixelpos(x, y):
            return cx * (1 + x), cy * (1 + y)

        if not self.surface:
            w = self.get_allocated_width()
            h = self.get_allocated_height()
            self.surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, w, h)
            surfctx = cairo.Context(self.surface)

            surfctx.translate(0.5, 0.5)
            bg_color = cfg.get_float_color('MV Background')
            surfctx.set_source_rgb(*bg_color)
            surfctx.rectangle(0, 0, w, h)
            surfctx.fill()

            surfctx.set_line_width(1)
            mplist = [(mp, get_pixelpos(*mp.get_position()))
                      for mp in player.get_plugin_list()]

            for mp, (rx, ry) in mplist:
                if self.dragging and mp in player.active_plugins:
                    pinfo = self.get_plugin_info(mp)
                    rx, ry = get_pixelpos(*pinfo.dragpos)

                for index in range(mp.get_input_connection_count()):
                    targetmp = mp.get_input_connection_plugin(index)
                    pi = common.get_plugin_infos().get(targetmp)
                    if not pi.songplugin:
                        continue
                    tmppos = targetmp.get_position()
                    if self.dragging and targetmp in player.active_plugins:
                        pinfo = self.get_plugin_info(targetmp)
                        tmppos = pinfo.dragpos


                    crx, cry = get_pixelpos(*tmppos)
                    if (mp.get_input_connection_type(index) != zzub.zzub_connection_type_event):
                        amp = mp.get_parameter_value(0, index, 0)
                        amp /= 16384.0
                        amp = amp ** 0.5
                        blended = blend_float(cfg.get_float_color("MV Arrow"), (0.0, 0.0, 0.0), amp)
                        arrowcolors[zzub.zzub_connection_type_audio][0] = blended

                    draw_line_arrow(surfctx, arrowcolors[mp.get_input_connection_type(index)], int(crx), int(cry), int(rx), int(ry), cfg)
            surfctx.translate(-0.5, -0.5)

        ctx.set_source_surface(self.surface, 0.0, 0.0)
        ctx.paint()

        if self.connecting:
            ctx.set_line_width(1)
            crx, cry = get_pixelpos(*player.active_plugins[0].get_position())
            rx, ry = self.connectpos
            linepen = cfg.get_float_color("MV Line")
            draw_line(ctx, linepen, int(crx), int(cry), int(rx), int(ry))



        self.draw_leds(ctx, pango_layout)

    # This method is not *just* for key-jazz, it handles all key-events in router. Rename?
    def on_key_jazz(self, widget, event, plugin):
        mask = event.get_state()
        kv = event.keyval
        k = Gdk.keyval_name(kv)
        if mask & Gdk.ModifierType.CONTROL_MASK:
            if k == 'Return':
                com.get('neil.core.pluginbrowser', self)
                return
            if mask & Gdk.ModifierType.META_MASK:
                self.showing_cvports = True
                self.redraw()
                return
        player = com.get('neil.core.player')
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
        player = com.get('neil.core.player')
        kv = event.keyval
        mask = event.get_state()

        if self.showing_cvports == True:
            if not mask & (Gdk.ModifierType.CONTROL_MASK|Gdk.ModifierType.META_MASK):
                self.showing_cvports = False
                self.redraw()
        if not plugin:
            if player.active_plugins:
                plugin = player.active_plugins[0]
            else:
                return

        if kv < 256:
            player = com.get('neil.core.player')
            octave = player.octave
            note = key_to_note(kv)
            if note in self.chordnotes:
                self.chordnotes.remove(note)
                n = ((note[0] + octave) << 4 | note[1] + 1)
                plugin.play_midi_note(zzub.zzub_note_value_off, n, 0)

__all__ = [
    'ParameterDialog',
    'ParameterDialogManager',
    'PresetDialog',
    'PresetDialogManager',
    'AttributesDialog',
    'RoutePanel',
    'VolumeSlider',
    'RouteView',
]

__neil__ = dict(
    classes = [
        ParameterDialog,
        ParameterDialogManager,
        PresetDialog,
        PresetDialogManager,
        AttributesDialog,
        RoutePanel,
        VolumeSlider,
        RouteView,
    ],
)

if __name__ == '__main__':
    import testplayer
    import utils
    player = testplayer.get_player()
    player.load_ccm(utils.filepath('demosongs/paniq-knark.ccm'))
    window = testplayer.TestWindow()
    window.add(RoutePanel(window))
    window.PAGE_ROUTE = 1
    window.index = 1
    window.show_all()
    Gtk.main()
