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
Provides dialogs, classes and controls to edit samples.
"""

import gtk
import pango
import pangocairo
import os, sys
from utils import prepstr, db2linear, linear2db, note2str, file_filter
from utils import read_int, write_int, add_scrollbars, new_image_button,\
     filepath, add_hscrollbar, error, message
import zzub
import config
import common
from common import MARGIN, MARGIN2, MARGIN3

import neil.com as com

# size of border
BORDER = 5
# size of the envelope dots
DOTSIZE = 8
# matches existing points exactly or approximately
EXACT = 0
NEXT = 1

class WaveEditPanel(gtk.VBox):
    def __init__(self, wavetable):
	gtk.VBox.__init__(self, False, MARGIN)
	self.wavetable = wavetable
	self.view = WaveEditView(wavetable)
        self.viewport = gtk.Viewport()
        self.viewport.add(self.view)
        self.pack_start(self.viewport)
	self.set_border_width(MARGIN)

    def update(self, *args):
	self.view.update()

    def on_store_range(self, widget):
	self.view.store_range()

    def on_apply_slices(self, widget):
	self.view.apply_slices()

    def on_delete_range(self, widget):
	self.view.delete_range()
	self.wavetable.update_sampleprops()

    def on_loop_range(self, widget):
	player = com.get('neil.core.player')
        begin, end = self.view.selection
        self.view.level.set_loop_start(begin)
        self.view.level.set_loop_end(end)
        player.history_commit("set loop range")
        
    def on_xfade_range(self, widget):
        player = com.get('neil.core.player')
        if self.view.selection == None:
            message(self, "Select a region of the wave first.")
            return
        begin, end = self.view.selection
        if (end - begin) < begin:
            self.view.level.xfade(begin, end)
            self.update()
            self.wavetable.update_sampleprops()
        else:
            message(self, "Not enough data at the start of selection.")

    def on_normalize(self, widget):
        player = com.get('neil.core.player')
        self.view.level.normalize()
        self.update()
        
class WaveEditView(gtk.DrawingArea):
    """
    Envelope viewer.

    A drawing surface where you can specify how the volume of a sample changes over time.
    """

    def __init__(self, wavetable):
	"""
	Initialization.
	"""
        self.zoom_level = 0
	self.wavetable = wavetable
	self.wave = None
	self.level = None
	self.offpeak = 0.4
	self.onpeak = 0.9
	self.dragging = False
        self.right_dragging = False
        self.right_drag_start = 0
	self.stretching = False
	gtk.DrawingArea.__init__(self)
	self.add_events(gtk.gdk.ALL_EVENTS_MASK)
	self.connect('button-press-event', self.on_button_down)
	self.connect('button-release-event', self.on_button_up)
	self.connect('motion-notify-event', self.on_motion)
	self.connect('enter-notify-event', self.on_enter)
	self.connect('leave-notify-event', self.on_leave)
	self.connect('scroll-event', self.on_mousewheel)
	self.connect("expose_event", self.expose)

        self.context_menu = gtk.Menu()
        
        self.menu_delete = gtk.MenuItem("Delete")
        self.menu_delete.show()
        self.menu_delete.connect('activate', self.on_delete_range)
        self.context_menu.append(self.menu_delete)
        
        self.menu_loop = gtk.MenuItem("Loop")
        self.menu_loop.show()
        self.menu_loop.connect('activate', self.on_loop_range)
        self.context_menu.append(self.menu_loop)

        self.menu_xfade = gtk.MenuItem("XFade")
        self.menu_xfade.show()
        self.menu_xfade.connect('activate', self.on_xfade_range)
        self.context_menu.append(self.menu_xfade)
        
        separator = gtk.SeparatorMenuItem()
        separator.show()
        self.context_menu.append(separator)

        self.menu_normalize = gtk.MenuItem("Normalize")
        self.menu_normalize.show()
        self.menu_normalize.connect('activate', self.on_normalize)
        self.context_menu.append(self.menu_normalize)
       
	self.loop_start = 0
	self.loop_end = 150

    def expose(self, widget, event):
	self.context = widget.window.cairo_create()
	self.draw(self.context)
	return False

    def get_client_size(self):
	rect = self.get_allocation()
	return rect.width, rect.height

    def delete_range(self):
	player = com.get('neil.core.player')
	if self.selection:
	    begin,end = self.selection
	    self.level.remove_sample_range(begin, end - 1)
	    self.selection = None
	    player.history_commit("remove sample range")
	    self.sample_changed()

    def redraw(self):
	if self.window:
	    w, h = self.get_client_size()
	    self.window.invalidate_rect((0, 0, w, h), False)

    def update_digest(self, channel=0):
	w, h = self.get_client_size()
	self.minbuffer, self.maxbuffer, self.ampbuffer = \
	    self.level.get_samples_digest(channel, self.range[0], self.range[1],  w)

    def fix_range(self):
	begin,end = self.range
	begin = max(min(begin, self.level.get_sample_count() - 1), 0)
	end = max(min(end, self.level.get_sample_count()), begin + 1)
	self.range = [begin, end]

    def set_range(self, begin, end):
	self.range = [begin, end]
	self.view_changed()

    def set_selection(self, begin, end):
	if begin == end:
	    self.selection = None
	else:
	    begin = max(min(begin, self.level.get_sample_count() - 1), 0)
	    end = max(min(end, self.level.get_sample_count()), begin + 1)
	    self.selection = [begin, end]
	self.redraw()

    def client_to_sample(self, x, y, db=False):
	w, h = self.get_client_size()
	sample = self.range[0] + (float(x) / float(w)) * (self.range[1] - self.range[0])
	if db:
	    amp = 1.0 - (float(y) / float(h))
	else:
	    amp = (float(y) / float(h)) * 2.0 - 1.0
	return int(sample + 0.5), amp

    def sample_to_client(self, s, a):
	w, h = self.get_client_size()
	x = ((float(s) - self.range[0]) / (self.range[1] - self.range[0])) * w
	y = (a + 1.0) * float(h) / 2.0
	return x, y

    def on_mousewheel(self, widget, event):
	"""
	Callback that responds to mousewheeling in pattern view.
	"""		
	mx, my = int(event.x), int(event.y)
	s, a = self.client_to_sample(mx,my)
	b, e = self.range
	diffl = s - b
	diffr = e - s
	if event.direction == gtk.gdk.SCROLL_DOWN:
	    diffl *= 2
	    diffr *= 2
	elif event.direction == gtk.gdk.SCROLL_UP:
	    diffl /= 2
	    diffr /= 2
	self.set_range(s - diffl, s + diffr)
	self.redraw()

    def store_range(self):
	w = self.wave
	origpath = w.get_path().replace('/',os.sep).replace('\\',os.sep)
	if origpath:
	    filename = os.path.splitext(os.path.basename(origpath))[0]
	else:
	    filename = w.get_name()
	if self.selection:
	    filename += '_selection'
	else:
	    filename += '_slice'
	filename += '.wav'
	if self.selection:
	    title = "Export Selection"
	else:
	    title = "Export Slices"
	dlg = gtk.FileChooserDialog(title, parent=self.get_toplevel(), action=gtk.FILE_CHOOSER_ACTION_SAVE,
		buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_SAVE, gtk.RESPONSE_OK))
	dlg.set_current_name(filename)
	dlg.set_do_overwrite_confirmation(True)
	dlg.add_filter(file_filter('Wave Files (*.wav)', '*.wav'))
	response = dlg.run()
	filepath = dlg.get_filename()
	dlg.destroy()
	if response != gtk.RESPONSE_OK:
	    return
	if self.selection:
	    begin,end = self.selection
	    self.wave.save_sample_range(0, filepath, begin, end)
	else:
	    filename, fileext = os.path.splitext(filepath)
	    for i,x in enumerate(self.peaks):
		if i == (len(self.peaks)-1):
		    end = self.level.get_sample_count()
		else:
		    end = self.peaks[i+1]
		filepath = "%s%03i%s" % (filename, i, fileext)
		print filepath
		self.wave.save_sample_range(0, filepath, x, end)

    def on_enter(self, widget, event):
	"""
	Called when the mouse enters the wave editor.
	"""
	self.redraw()

    def on_leave(self, widget, event):
	"""
	Called when the mouse leaves the wave editor.
	"""
	self.redraw()

    def on_button_down(self, widget, event):
	"""
	Callback that responds to left mouse down over the wave view.
	"""
	mx, my = int(event.x), int(event.y)
	if (event.button == 1):
	    s, a = self.client_to_sample(mx,my)
            # If a user double-clicks - clear the selection.
	    if (event.type == gtk.gdk._2BUTTON_PRESS):
		self.selection = None
		self.dragging = False
		self.redraw()
	    else:
                if self.selection:
                    begin, end = self.selection
                    x1, y1 = self.sample_to_client(begin, 0)
                    x2, y2 = self.sample_to_client(end, 0)
                    if ((mx > x1 - 10) and (mx < x1 + 10)):
                        self.dragging = True
                        self.startpos = end
                    elif ((mx > x2 - 10) and (mx < x2 + 10)):
                        self.dragging = True
                        self.startpos = begin
		else:
		    self.startpos = s
                    self.dragging = True
		self.grab_add()
		self.redraw()
        elif (event.button == 2):
	    s, a = self.client_to_sample(mx, my)
            self.right_dragging = True
            self.right_drag_start = s
        elif (event.button == 3):
            # Menu items that need selection to function properly.
            sensitives = [self.menu_delete, self.menu_loop, self.menu_xfade]
            if not self.selection:
                # If there is no selection disable those menu items,
                for sensitive in sensitives:
                    sensitive.set_sensitive(False)
            else:
                # otherwise enable them.
                for sensitive in sensitives:
                    sensitive.set_sensitive(True)
            self.context_menu.popup(None, None, None, event.button, event.time)

    def on_delete_range(self, widget):
	self.delete_range()
        self.sample_changed()

    def on_loop_range(self, widget):
	player = com.get('neil.core.player')
        begin, end = self.selection
        self.level.set_loop_start(begin)
        self.level.set_loop_end(end)
        w = self.wave
        flags = w.get_flags()
        flags = flags | zzub.zzub_wave_flag_loop
        w.set_flags(flags)
        player.history_commit("set loop range")
        
    def on_xfade_range(self, widget):
        player = com.get('neil.core.player')
        if self.selection == None:
            message(self, "Select a region of the wave first.")
            return
        begin, end = self.selection
        if (end - begin) < begin:
            self.level.xfade(begin, end)
        else:
            message(self, "Not enough data at the start of selection.")
        self.sample_changed()

    def on_normalize(self, widget):
        player = com.get('neil.core.player')
        self.level.normalize()
        self.sample_changed()

    def sample_changed(self):
	self.view_changed()

    def view_changed(self, *args):
	self.fix_range()		
	self.update_digest()
	self.redraw()

    def on_button_up(self, widget, event):
	"""
	Callback that responds to mouse button up over the wave view.
	"""
        if event.button == 1:
            if self.dragging == True:
                self.dragging = False
                self.grab_remove()
                mx, my = int(event.x), int(event.y)
                s, a = self.client_to_sample(mx,my)
                if s < self.startpos:
                    self.set_selection(s, self.startpos)
                else:
                    self.set_selection(self.startpos, s)
                self.redraw()
        elif event.button == 2:
            self.right_dragging = False

    def on_motion(self, widget, event):
	"""
	Callback that responds to mouse motion over the wave view.
	"""
	mx, my = int(event.x), int(event.y)
	s, a = self.client_to_sample(mx, my)
        # Change the cursor if the mouse pointer is near selection borders.
        if self.selection:
            begin, end = self.selection
            x1, y1 = self.sample_to_client(begin, 0)
            x2, y2 = self.sample_to_client(end, 0)
            if (((mx > x1 - 10) and (mx < x1 + 10)) or
                ((mx > x2 - 10) and (mx < x2 + 10))):
                resizer = gtk.gdk.Cursor(gtk.gdk.SB_H_DOUBLE_ARROW)
                self.window.set_cursor(resizer)
            else:
                if not self.dragging:
                    arrow = gtk.gdk.Cursor(gtk.gdk.ARROW)
                    self.window.set_cursor(arrow)
	if self.dragging == True:
	    if s < self.startpos:
		self.set_selection(s, self.startpos)
	    else:
		self.set_selection(self.startpos, s)
	    self.redraw()
        elif self.right_dragging == True:
            begin, end = self.range
            diff = self.right_drag_start - s
            self.set_range(begin + diff, end + diff)

    def update(self):
	"""
	Updates the envelope view based on the sample selected in the sample list.
	"""
	sel = self.wavetable.get_sample_selection()
	self.wave = None
	self.level = None
	self.selection = None
	player = com.get('neil.core.player')
	if sel:
	    self.wave = player.get_wave(sel[0])
	    if self.wave.get_level_count() >= 1:
		self.level = self.wave.get_level(0)
		self.range = [0,self.level.get_sample_count()]
		self.sample_changed()
	self.redraw()

    def set_sensitive(self, enable):
	gtk.DrawingArea.set_sensitive(self, enable)
	self.redraw()

    def draw_zoom_indicator(self, ctx):
        w, h = self.get_client_size()
        ctx.set_source_rgb(0.5, 0.5, 0.5)
        ctx.set_line_width(1)
        ctx.rectangle(10, h - 20, w - 20, 10)
        ctx.stroke()
        sample_count = self.level.get_sample_count()
        begin, end = self.range
        start = 1.0 - (sample_count - begin) / float(sample_count)
        finish = 1.0 - (sample_count - end) / float(sample_count)
        ctx.rectangle(11 + (w - 20) * start, h - 19,
                      (w - 22) * finish - (w - 20) * start, 8)
        ctx.fill()

    def draw_loop_points(self, ctx):
        width, height = self.get_client_size()
        begin, end = self.range
        if self.level.get_wave().get_flags() & zzub.zzub_wave_flag_loop:
            loop_start = self.level.get_loop_start()
            loop_end = self.level.get_loop_end()
            ctx.set_source_rgb(1.0, 0.0, 0.0)
            ctx.set_line_width(1)
            if loop_start > begin and loop_start < end:
                scale = (loop_start - begin) / float(end - begin)
                x = int(width * scale)
                ctx.move_to(x, 0)
                ctx.line_to(x + 10, 0)
                ctx.line_to(x, 10)
                ctx.line_to(x, 0)
                ctx.line_to(x, height)
                ctx.stroke_preserve()
                ctx.fill()
            if loop_end > begin and loop_end < end:
                scale = (loop_end - begin) / float(end - begin)
                x = int(width * scale)
                ctx.move_to(x, 0)
                ctx.line_to(x - 10, 0)
                ctx.line_to(x, 10)
                ctx.line_to(x, 0)
                ctx.line_to(x, height)
                ctx.stroke_preserve()
                ctx.fill()

    def draw(self, ctx):
	"""
	Overriding a L{Canvas} method that paints onto an offscreen buffer.
	Draws the envelope view graphics.
	"""
	w, h = self.get_client_size()
	cfg = config.get_config()

	bgbrush = cfg.get_float_color('WE BG')
	pen = cfg.get_float_color('WE Line')
	brush = cfg.get_float_color('WE Fill')
	brush2 = cfg.get_float_color('WE Peak Fill')
	gridpen = cfg.get_float_color('WE Grid')
	selbrush = cfg.get_float_color('WE Selection')
	stretchbrush = cfg.get_float_color('WE Stretch Cue')
	splitbar = cfg.get_float_color('WE Split Bar')
	slicebar = cfg.get_float_color('WE Slice Bar')
	onpeak = cfg.get_float_color('WE Wakeup Peaks')
	offpeak = cfg.get_float_color('WE Sleep Peaks')

	ctx.translate(0.5, 0.5)
	ctx.set_source_rgb(*bgbrush)
	ctx.rectangle(0, 0, w, h)
	ctx.fill()
	ctx.set_line_width(1)

	if self.level == None:
	    return

	player = com.get('neil.core.player')
	ctx.set_source_rgb(*gridpen)
	rb, re = self.range
	rsize = re - rb
        x = 0
        y = 0
        ctx.set_source_rgba(*(gridpen))
        for i in range(8):
            ctx.move_to(x, 0)
            ctx.line_to(x, h)
            ctx.move_to(x + 2, 0)
            pango_ctx = pangocairo.CairoContext(ctx)
            layout = pango_ctx.create_layout()
            layout.set_width(-1)
            layout.set_font_description(pango.FontDescription("sans 8"))
            sample_number = rb + i * (rsize / 8)
            second = sample_number / float(self.level.get_samples_per_second())
            layout.set_markup("<small>%.3fs</small>" % second)
            pango_ctx.update_layout(layout)
            pango_ctx.show_layout(layout)
            x += (w / 8)
        for i in range(8):
            ctx.move_to(0, y)
            ctx.line_to(w, y)
            y += (h / 8)
        ctx.stroke()

        ctx.move_to(2, 14)
        ctx.set_source_rgba(0.4, 0.4, 0.4, 1.0)
        pango_ctx = pangocairo.CairoContext(ctx)
        layout = pango_ctx.create_layout()
        layout.set_width(-1)
        layout.set_font_description(pango.FontDescription("sans 8"))
        layout.set_markup("<b>%s</b>" % self.wave.get_path())
        pango_ctx.update_layout(layout)
        pango_ctx.show_layout(layout)
        ctx.stroke()

        channels = 1
        if self.wave.get_flags() & zzub.zzub_wave_flag_stereo:
            channels = 2
            
        for channel in range(channels):
            #self.update_digest(channel)
            minbuffer, maxbuffer, ampbuffer = \
                self.minbuffer, self.maxbuffer, self.ampbuffer
            # Draw the waveform.
            ctx.set_source_rgb(0.7, 0.9, 0.7)
            hm = (h / (2 * channels) - 1) * (1 + channel * 2)
            ctx.move_to(0, hm)
            for x in xrange(0, w):
                ctx.line_to(x, hm - (h / channels) * maxbuffer[x] * 0.4)
            for x in xrange(0, w):
                ctx.line_to(w - x, hm - (h / channels) * minbuffer[w - x - 1] * 0.4)
            ctx.fill_preserve()
            ctx.set_source_rgb(*pen)
            ctx.stroke()
	# Draw the selection rectangle.
 	if self.selection:
 	    begin, end = self.selection
 	    x1 = self.sample_to_client(begin, 0.0)[0]
 	    x2 = self.sample_to_client(end, 0.0)[0]
 	    if (x2 >= 0) and (x1 <= w):
 		ctx.rectangle(x1, 0, x2 - x1, h - 1)
                ctx.set_source_rgba(0.0, 1.0, 0.0, 1.0)
                ctx.set_line_width(1)
                ctx.stroke_preserve()
 		ctx.set_source_rgba(0.0, 1.0, 0.0, 0.2)
 		ctx.fill()
        self.draw_loop_points(ctx)
        self.draw_zoom_indicator(ctx)
