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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

"""
Contains all classes and functions needed to render the sequence
editor and its associated components.
"""



import gi
gi.require_version('Gtk', '3.0')
gi.require_version('PangoCairo', '1.0')
from gi.repository import Gdk, GObject, Gtk, Pango, PangoCairo

import random
import config

from neil import components
import neil.common as common
from neil.utils import (from_hsb, prepstr, synchronize_list, to_hsb)

MARGIN = common.MARGIN
MARGIN2 = common.MARGIN2
MARGIN3 = common.MARGIN3
MARGIN0 = common.MARGIN0

SEQROWSIZE = 24

class Track(Gtk.HBox):
    """
    Track header. Displays controls to mute or solo the track.
    """
    __neil__ = dict(
        id = 'neil.core.track',
        categories = [
        ]
    )

    def __init__(self, track, hadjustment=None):
        GObject.GObject.__init__(self)
        self.track = track
        self.hadjustment = hadjustment
        self.header = Gtk.VBox()
        self.label = Gtk.Label(label=prepstr(self.track.get_plugin().get_name()))
        self.label.set_alignment(0.0, 0.5)
        hbox = Gtk.HBox()
        hbox.pack_start(self.label, True, True, 5)
        self.header.pack_start(hbox, True, True)
        separator = Gtk.HSeparator()
        self.header.pack_end(separator, False, False)
        self.view = components.get('neil.core.trackview', track, hadjustment)
        self.pack_start(self.header, False, False)
        self.pack_end(self.view, True, True)

class View(Gtk.DrawingArea):
    """
    base class for track-like views.
    """
    def __init__(self, hadjustment=None):
        GObject.GObject.__init__(self)
        self.step = 64
        self.patterngfx = {}
        self.hadjustment = hadjustment
        self.add_events(Gdk.EventMask.ALL_EVENTS_MASK)
        self.connect("draw", self.on_draw)
        if hadjustment:
            self.hadjustment.connect('value-changed', self.on_adjustment_value_changed)
            self.hadjustment.connect('changed', self.on_adjustment_changed)
        self.connect('size-allocate', self.on_size_allocate)

    def get_ticks_per_pixel(self):
        w,h = self.get_client_size()
        # size of view
        pagesize = int(self.hadjustment.page_size+0.5)
        # pixels per tick
        return pagesize / float(w)

    def on_size_allocate(self, *args):
        self.patterngfx = {}
        self.redraw()

    def on_adjustment_value_changed(self, adjustment):
        self.redraw()

    def on_adjustment_changed(self, adjustment):
        self.patterngfx = {}
        self.redraw()

    def redraw(self):
        if self.get_window():
            rect = self.get_allocation()
            self.get_window().invalidate_rect((0,0,rect.width,rect.height), False)

    def get_client_size(self):
        rect = self.get_allocation()
        return rect.width, rect.height

    def on_draw(self, widget, ctx):
        self.draw(ctx)
        return False

class TimelineView(View):
    """
    timeline view. shows a horizontal sequencer timeline.
    """
    __neil__ = dict(
        id = 'neil.core.timelineview',
        categories = [
        ]
    )

    def __init__(self, hadjustment=None):
        View.__init__(self, hadjustment)
        self.set_size_request(-1, 16)

    def draw(self, ctx):
        player = components.get('neil.core.player')
        w,h = self.get_client_size()

        cfg = config.get_config()
        bg_color = cfg.get_float_color('SE BG')
        pen1_color = cfg.get_float_color('SE BG Very Dark')
        pen2_color = cfg.get_float_color('SE BG Dark')
        text_color = cfg.get_float_color('SE Text')

        # drawable.draw_rectangle(gc, True, 0, 0, w, h)
        ctx.set_source_rgb(*bg_color)
        ctx.rectangle(0, 0, w, h)
        ctx.fill()

        pango_ctx = PangoCairo.CairoContext(ctx)
        pango_ctx.set_source_rgb(*text_color)
        pango_layout = ctx.create_layout()
        pango_layout.set_font_description(Pango.setFontDescription('Sans 7.5'))
        pango_layout.set_width(-1)

        # first visible tick
        start = int(self.hadjustment.get_value()+0.5)
        # size of view
        pagesize = int(self.hadjustment.page_size+0.5)
        # last visible tick
        end = int(self.hadjustment.get_value() + self.hadjustment.page_size + 0.5)

        # pixels per tick
        tpp = self.get_ticks_per_pixel()

        # distance of indices to print
        stepsize = int(4*self.step + 0.5)

        # first index to print
        startindex = (start / stepsize) * stepsize

        i = startindex
        while i < end:
            x = int((i - start)/tpp + 0.5)
            if i == (i - i%(stepsize*4)):
                ctx.set_source_rgb(*pen1_color)
            else:
                ctx.set_source_rgb(*pen2_color)
            ctx.move_to(x-1, 0)
            ctx.line_to(x-1, h)
            ctx.stroke()

            # layout.set_text("%i" % i)
            pango_layout.set_text("%i" % i)
            px,py = pango_layout.get_size()

            pango_ctx.moveto(x, int((h-py) / 2))
            pango_ctx.set_source_rgb(*text_color)
            pango_ctx.show_layout(pango_layout)
            pango_ctx.stroke()
            i += stepsize

class TrackView(View):
    """
    Track view. Displays the content of one track.
    """
    __neil__ = dict(
        id = 'neil.core.trackview',
        categories = [
        ]
    )

    def __init__(self, track, hadjustment=None):
        self.track = track
        View.__init__(self, hadjustment)
        self.set_size_request(-1, 22)

    def draw_pattern_event(self, ctx, pango_layout, evt, colors):
        evt_pos, evt_value = evt

        m = self.track.get_plugin()
        mname = m.get_name()
        title = prepstr(mname)

        if evt_value >= 0x10:
            pat = m.get_pattern(evt_value - 0x10)
            name, length = prepstr(pat.get_name()), pat.get_row_count()
        elif evt_value == 0x00:
            name, length = "X", 1
        elif evt_value == 0x01:
            name, length = "<", 1
        else:
            print(("unknown value:", evt_value))
            name,length = "???",0

        w, h = self.get_client_size()

        # first visible tick
        offset = int(self.hadjustment.get_value() + 0.5)

        # start = pos
        end = evt_pos + length
        tpp = self.get_ticks_per_pixel()
        start_x = int(((evt_pos - offset) / tpp) + 0.5)
        end_x = int(((end - offset) / tpp) + 0.5)
        psize = max(end_x - start_x, 2) # max(int(((SEQROWSIZE * length) / self.step) + 0.5),2)
        bbh = h-2

        if evt_value < 0x10:
            if ((start_x + psize - 1) < 0) or (start_x > w):
                return

            ctx.set_source_rgb(*colors['events'][evt_value])
            ctx.rectangle(0, 0, psize-1, bbh-1)
            ctx.fill()
        else:
            if ((start_x + psize - 2) < 0) or (start_x > w):
                return

            random.seed(mname + name)
            hue = random.random()

            ctx.set_source_rbg(*from_hsb(hue, 1.0, colors['bg_brightness'] * 0.7))
            ctx.rectangle(0, 0, psize-2, bbh-2)
            ctx.fill()

            pango_layout.set_text(name)
            px, py = pango_layout.get_pixel_size()
            pango_ctx = pango_layout.get_context()
            pango_ctx.move_to(2, 0 + bbh/2 - py / 2)
            pango_ctx.show_layout(pango_layout)
            pango_layout.stroke()


    def get_colors(self):
        cfg = config.get_config()
        colors = {}

        colors['bg'] = cfg.get_float_color('SE BG')
        colors['bg_brightness'] = max(to_hsb(*colors['bg'])[2], 0.1)
        colors['events'] = (cfg.get_float_color('SE Mute'), cfg.get_float_color('SE Break'))
        # colors['select'] = cfg.get_float_color('SE Sel BG')
        # colors['vline'] = cfg.get_float_color('SE BG Dark')
        colors['pen1'] = cfg.get_float_color('SE BG Very Dark')
        colors['pen2'] = cfg.get_float_color('SE BG Dark')
        # colors['pen'] = cfg.get_float_color('SE Line')
        # colors['loop'] = cfg.get_float_color('SE Loop Line')
        # colors['inv'] = (1.0, 1.0, 1.0)
        colors['text'] = cfg.get_float_color('SE Text')

        return colors

    def draw(self, ctx):
        player = components.get('neil.core.player')
        w, h = self.get_client_size()

        colors = self.get_colors()

        ctx.set_source_rgb(*colors['bg'])
        ctx.rectangle(0, 0, w, h)
        ctx.fill()

        # first visible tick
        start = int(self.hadjustment.get_value() + 0.5)
        # size of view
        pagesize = int(self.hadjustment.page_size + 0.5)
        # last visible tick
        end = int(self.hadjustment.get_value() + self.hadjustment.page_size + 0.5)

        # pixels per tick
        tpp = self.get_ticks_per_pixel()

        # distance of indices to print
        stepsize = int(4 * self.step + 0.5)

        # first index to print
        startindex = (start / stepsize) * stepsize

        # timeline
        i = startindex
        while i < end:
            x = int((i - start) / tpp + 0.5)
            if i == (i - i%(stepsize * 4)):
                ctx.set_source_rbg(*colors['pen1'])
            else:
                ctx.set_source_rbg(*colors['pen2'])
            ctx.move_to(x-1, 0)
            ctx.line_to(x-1, h)
            ctx.stroke()
            i += stepsize

        pango_ctx = PangoCairo.CairoContext(ctx)
        pango_ctx.set_source_rgb(*colors['text'])
        pango_layout = pango_ctx.create_layout()
        #~ layout.set_font_description(self.fontdesc)
        pango_layout.set_width(-1)
        for evt in self.track.get_event_list():
            self.draw_pattern_event(ctx, pango_layout, evt, colors)


#                               if intrack and (pos >= selstart[1]) and (pos <= selend[1]):
#                                       gc.set_foreground(invbrush)
#                                       gc.set_function(Gdk.XOR)
#                                       drawable.draw_rectangle(gc, True, x+ofs, y+1, bbw-ofs, bbh)
#                                       gc.set_function(Gdk.COPY)
        #gc.set_foreground(vlinepen)
        #drawable.draw_line(gc, 0, y, w, y)

#               gc.set_foreground(pen)
#               x = SEQLEFTMARGIN-1
#               drawable.draw_line(gc, x, 0, x, h)
#               se = player.get_song_end()
#               x,y = self.track_row_to_pos((0,se))
#               if (x >= SEQLEFTMARGIN):
#                       gc.set_foreground(pen)
#                       drawable.draw_line(gc, x-1, 0, x-1, h)
#               gc.set_foreground(loop_pen)
#               gc.line_style = Gdk.LINE_ON_OFF_DASH
#               gc.set_dashes(0, (1,1))
#               lb,le = player.get_loop()
#               x,y = self.track_row_to_pos((0,lb))
#               if (x >= SEQLEFTMARGIN):
#                       drawable.draw_line(gc, x-1, 0, x-1, h)
#               x,y = self.track_row_to_pos((0,le))
#               if (x >= SEQLEFTMARGIN):
#                       drawable.draw_line(gc, x-1, 0, x-1, h)
#               self.draw_xor()

        return False

class TrackViewPanel(Gtk.VBox):
    """
    Sequencer pattern panel.

    Displays all the patterns available for the current track.
    """
    __neil__ = dict(
        id = 'neil.core.trackviewpanel',
        singleton = True,
        categories = [
                'neil.viewpanel',
                'view',
        ]
    )

    __view__ = dict(
        label = "Tracks",
        stockid = "neil_sequencer",
        shortcut = '<Shift>F4',
        order = 4,
    )

    def __init__(self):
        """
        Initialization.
        """
        GObject.GObject.__init__(self)
        self.sizegroup = Gtk.SizeGroup(Gtk.SizeGroupMode.HORIZONTAL)
        self.hscroll = Gtk.HScrollbar()
        hadjustment = self.hscroll.get_adjustment()
        hadjustment.set_all(0, 0, 16384, 1, 1024, 2300)
        self.timeline = components.get('neil.core.timelineview', hadjustment)
        self.trackviews = Gtk.VBox()

        vbox = Gtk.VBox()

        hbox = Gtk.HBox()
        timeline_padding = Gtk.HBox()
        self.sizegroup.add_widget(timeline_padding)
        hbox.pack_start(timeline_padding, False, False)
        hbox.pack_start(self.timeline, True, True, 0)

        vbox.pack_start(hbox, False, False)
        vbox.pack_start(self.trackviews, True, True, 0)

        hbox = Gtk.HBox()
        scrollbar_padding = Gtk.HBox()
        self.sizegroup.add_widget(scrollbar_padding)
        hbox.pack_start(scrollbar_padding, False, False)
        hbox.pack_start(self.hscroll, True, True, 0)

        vbox.pack_end(hbox, False, False)

        self.pack_start(vbox, True, True, 0)
        eventbus = components.get('neil.core.eventbus')
        eventbus.zzub_sequencer_changed += self.update_tracks
        eventbus.zzub_set_sequence_tracks += self.update_tracks
        eventbus.zzub_sequencer_remove_track += self.update_tracks
        self.update_tracks()
        self.show_all()
        self.trackviews.connect('size-allocate', self.on_size_allocate)

    def on_size_allocate(self, *args):
        rect = self.get_allocation()
        w,h = rect.width, rect.height
        hadjustment = self.hscroll.get_adjustment()
        hadjustment.page_size = int(((64.0 * w) / 24.0) + 0.5)

    def update_tracks(self, *args):
        player = components.get('neil.core.player')
        tracklist = list(player.get_sequence_list())

        def insert_track(i,track):
            print(("insert",i,track))
            trackview = components.get('neil.core.track', track, self.hscroll.get_adjustment())
            self.trackviews.pack_start(trackview, False, False)
            self.trackviews.reorder_child(trackview, i)
            self.sizegroup.add_widget(trackview.header)
            trackview.show_all()

        def del_track(i):
            print(("del",i))
            trackview = self.trackviews.get_children()[i]
            trackview.track = None
            self.sizegroup.remove_widget(trackview.header)
            self.trackviews.remove(trackview)
            trackview.destroy()

        def swap_track(i,j):
            print(("swap",i,j))

        tracks = [trackview.track for trackview in self.trackviews]
        synchronize_list(tracks, tracklist, insert_track, del_track, swap_track)

__neil__ = dict(
    classes = [
        TrackViewPanel,
        Track,
        TrackView,
        TimelineView,
    ],
)

if __name__ == '__main__':
    import os, sys
    os.system('../../bin/neil-combrowser neil.core.trackviewpanel')
    sys.exit()
