import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Gdk, Pango, PangoCairo

import config
from neil.utils import db2linear, linear2db
from .utils import router_sizes

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
        self.parent_widget = parent
        self.plugin = None
        self.index = -1
        Gtk.Window.__init__(self, Gtk.WindowType.POPUP)
        self.drawingarea = Gtk.DrawingArea()
        self.add(self.drawingarea)
        self.drawingarea.add_events(Gdk.EventMask.ALL_EVENTS_MASK)
        self.drawingarea.set_property('can-focus', True)
        self.resize(router_sizes.get('volbarwidth'), router_sizes.get('volbarheight'))
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
        self.amp = max(min(self.amp + (float(delta) / router_sizes.get('volbarheight')), 1.0), 0.0)
        amp = min(max(int(db2linear(self.amp * -48.0, -48.0) * 16384.0), 0), 16384)
        self.plugin.set_parameter_value_direct(0, self.index, 0, amp, False)
        self.redraw()
        return False

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
            pos = int(self.amp * (router_sizes.get('volbarheight') - router_sizes.get('volknobheight')))
            ctx.rectangle(1, pos + 1, router_sizes.get('volbarwidth') - 2, router_sizes.get('volknobheight') - 2)
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

    def display(self, xy, mp, index, orig_event_y):
        """
        Called by the router view to show the control.

        @param mx: X coordinate of the control center in pixels.
        @type mx: int
        @param my: Y coordinate of the control center in pixels.
        @type my: int
        @param conn: Connection to control.
        @type conn: zzub.Connection
        """
        (mx, my) = xy

        self.set_attached_to(self.parent_widget)
        self.set_transient_for(self.parent_widget.get_toplevel())

        self.set_attached_to(self.parent_widget)
        self.set_transient_for(self.parent_widget.get_toplevel())

        self.plugin = mp
        self.index = index
        self.amp = abs((linear2db((self.plugin.get_parameter_value(0, index, 0) / 16384.0), -48.0) / -48.0))
        self.y = orig_event_y
        self.oy = int(my - router_sizes.get('volbarheight') * self.amp)

        self.move(int(mx - router_sizes.get('volbarwidth') * 0.5), int(my - router_sizes.get('volbarheight') * self.amp))
        self.show_all()

        self.drawingarea.grab_add()


    def on_left_up(self, widget, event):
        """
        Event handler for left mouse button releases. Will
        hide control.

        @param event: Mouse event.
        @type event: wx.MouseEvent
        """
        self.parent_widget.redraw()
        self.hide()
        self.drawingarea.grab_remove()