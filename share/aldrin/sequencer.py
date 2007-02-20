#encoding: latin-1

# Aldrin
# Modular Sequencer
# Copyright (C) 2006 The Aldrin Development Team
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

from gtkimport import gtk
import pango
import gobject
from utils import prepstr, from_hsb, to_hsb, get_item_count, get_clipboard_text, set_clipboard_text, add_scrollbars
import random
import ctypes
import zzub
import config
import time
import common
player = common.get_player()
from common import MARGIN, MARGIN2, MARGIN3, MARGIN0

SEQKEYS = '0123456789abcdefghijklmnopqrstuvwxyz'
SEQKEYMAP = dict(zip(SEQKEYS,range(0x10,len(SEQKEYS)+0x10)))
SEQKEYMAP[chr(45)] = 0x00
SEQKEYMAP[chr(44)] = 0x01

class PatternNotFoundException(Exception):
	"""	
	Exception thrown when pattern is not found.
	"""
	pass


class SequencerToolBar(gtk.HBox):
	"""
	Sequencer Toolbar
	
	Allows to set the step size for the sequencer view.
	"""
	def __init__(self):
		"""
		Initialization.
		"""
		# begin wxGlade: SequencerFrame.__init__
		# kwds['style'] = wx.TB_HORIZONTAL|wx.TB_FLAT
		gtk.HBox.__init__(self, False, MARGIN)
		self.set_border_width(MARGIN)
		self.steplabel = gtk.Label()
		self.steplabel.set_text_with_mnemonic("_Step")
		self.stepselect = gtk.combo_box_new_text()
		self.steps = [1,2,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60,64,96,128,192,256,512,1024]
		self.stepselect.connect('changed', self.on_stepselect)
		self.steplabel.set_mnemonic_widget(self.stepselect)

		self.pack_start(self.steplabel, expand=False)
		self.pack_start(self.stepselect, expand=False)
		
	def increase_step(self):
		if self.parent.view.step < 1024:
			self.parent.view.step *= 2
		self.update_stepselect()
		self.parent.update_all()
		
	def decrease_step(self):
		if self.parent.view.step > 1:
			self.parent.view.step /= 2
		self.update_stepselect()
		self.parent.update_all()
		
	def update_all(self):
		"""
		Updates the toolbar to reflect sequencer changes.
		"""
		self.update_stepselect()
	
	def update_stepselect(self):
		"""
		Updates the step selection choice box.
		"""
		self.stepselect.get_model().clear()
		for i in self.steps:
			self.stepselect.append_text("%i" % i)
		self.stepselect.set_active(self.steps.index(self.parent.view.step))
		config.get_config().set_default_int('SequencerStep', self.parent.view.step)
		
	def on_stepselect(self, widget):
		"""
		Handles events sent from the choice box when a step size is being selected.
		"""
		if widget.get_active() == -1:
			return
		if self.parent.view.step == self.steps[widget.get_active()]:
			return
		self.parent.view.step = self.steps[widget.get_active()]
		self.parent.update_all()

class SequencerPanel(gtk.VBox):
	"""
	Sequencer pattern panel.
	
	Displays all the patterns available for the current track.
	"""
	def __init__(self, rootwindow):
		"""
		Initialization.
		"""
		self.rootwindow = rootwindow
		gtk.VBox.__init__(self)
		self.splitter = gtk.HPaned()
		
		self.seqliststore = gtk.ListStore(str, str)
		self.seqpatternlist = gtk.TreeView(self.seqliststore)
		self.seqpatternlist.set_rules_hint(True)
		tvkey = gtk.TreeViewColumn("Key")
		tvkey.set_resizable(True)
		tvpname = gtk.TreeViewColumn("Pattern Name")
		tvpname.set_resizable(True)
		cellkey = gtk.CellRendererText()
		cellpname = gtk.CellRendererText()
		tvkey.pack_start(cellkey)
		tvpname.pack_start(cellpname)
		tvkey.add_attribute(cellkey, 'text', 0)
		tvpname.add_attribute(cellpname, 'text', 1)
		self.seqpatternlist.append_column(tvkey)
		self.seqpatternlist.append_column(tvpname)
		self.seqpatternlist.set_search_column(0)
		tvkey.set_sort_column_id(0)
		tvpname.set_sort_column_id(1)

		self.seqview = SequencerView(rootwindow, self)
		self.splitter.pack1(self.seqview, True, True)
		self.splitter.pack2(add_scrollbars(self.seqpatternlist), False, False)
		self.view = self.seqview
		self.toolbar = SequencerToolBar()
		self.statusbar = gtk.HBox(False, MARGIN)
		self.statusbar.set_border_width(MARGIN0)
		self.pack_start(self.toolbar, expand=False)
		self.pack_start(self.splitter)
		self.pack_end(self.statusbar, expand=False)
		self.__set_properties()
		self.__do_layout()
		# end wxGlade
		self.update_list()
		self.toolbar.update_all()
		self.seqview.connect('size-allocate', self.on_sash_pos_changed)
		
	def update_all(self):
		"""
		Updates everything to reflect changes in the sequencer.
		"""
		self.update_list()
		self.toolbar.update_all()
		for k,v in self.view.plugin_info.iteritems():
			v.patterngfx = {}
		self.view.update()
		
	def update_list(self):
		"""
		Updates the panel to reflect a sequence view change.
		"""
		self.seqliststore.clear()
		self.seqliststore.append(['-', 'Mute'])
		self.seqliststore.append([',', 'Break'])
		seq = player.get_current_sequencer()
		if (self.seqview.track != -1) and (self.seqview.track < seq.get_track_count()):
			track = seq.get_track(self.seqview.track)
			for pattern, key in zip(track.get_plugin().get_pattern_list(), SEQKEYS):
				self.seqliststore.append([key, pattern.get_name()])
				
	def on_sash_pos_changed(self, widget, *args):
		"""
		Sent when the sash position changes.
		
		@param event: Event.
		@type event: wx.Event
		"""
		if not self.splitter.window:
			return
		if not self.splitter.window.is_visible():
			return
		config.get_config().save_window_pos("SequencerSplitter", self.splitter)
		
	def __set_properties(self):
		"""
		Sets properties during initialization.
		"""
		# begin wxGlade: SequencerFrame.__set_properties
		self.statuslabels = []
		for i in range(2):
			label = gtk.Label()
			self.statuslabels.append(label)
			self.statusbar.pack_start(label, expand=False)
			self.statusbar.pack_start(gtk.VSeparator(), expand=False)
		# end wxGlade

	def __do_layout(self):
		"""
		Arranges children components during initialization.
		"""
		self.show_all()
		config.get_config().load_window_pos("SequencerSplitter", self.splitter)
		# end wxGlade

# end of class SequencerFrame

SEQTRACKSIZE = 16
SEQSTEP = 16
SEQLEFTMARGIN = 64
SEQTOPMARGIN = SEQTRACKSIZE
SEQROWSIZE = 24

class SequencerView(gtk.DrawingArea):
	"""
	Sequence viewer class.
	"""	
	CLIPBOARD_SEQUENCER = "SEQUENCERDATA"
	
	def __init__(self, rootwindow, panel):
		"""
		Initialization.
		"""
		self.panel = panel
		self.rootwindow = rootwindow
		self.plugin_info = common.get_plugin_infos()
		self.playpos = player.get_position()
		self.row = 0
		self.track = 0
		self.startseqtime = 0
		self.starttrack = 0
		self.step = config.get_config().get_default_int('SequencerStep', SEQSTEP)
		player.set_loop_end(self.step)
		self.selection_start = None
		self.selection_end = None
		self.dragging = False
		gtk.DrawingArea.__init__(self)
		self.add_events(gtk.gdk.ALL_EVENTS_MASK)
		self.set_property('can-focus', True)
		self.connect("expose_event", self.expose)
		self.connect('key-press-event', self.on_key_down)
		self.connect('button-press-event', self.on_left_down)
		self.connect('motion-notify-event', self.on_motion)
		self.connect('button-release-event', self.on_left_up)
		self.connect('scroll-event', self.on_mousewheel)
		gobject.timeout_add(100, self.update_position)
		
		#~ wx.EVT_MOUSEWHEEL(self, self.on_mousewheel)
		#~ wx.EVT_LEFT_DOWN(self, self.on_left_down)
		#~ wx.EVT_MOTION(self, self.on_motion)
		#~ wx.EVT_LEFT_UP(self, self.on_left_up)
		#~ wx.EVT_KEY_DOWN(self, self.on_key_down)
		#~ wx.EVT_CONTEXT_MENU(self, self.on_context_menu)
		
	def on_focus(self, event):		
		seq = player.get_current_sequencer()
		if seq.get_track_count() > 0:
			self.set_cursor_pos(self.track, self.row)
		self.panel.update_list()
		self.ReDraw()
		
	def track_row_to_pos(self, (track,row)):
		"""
		Converts track and row to a pixel coordinate.
		
		@param track: Track index.
		@type track: int
		@param row: Row index.
		@type row: int
		@return: Pixel coordinate.
		@rtype: (int, int)
		"""
		if row == -1:
			x = 0
		else:
			x = int((((float(row) - self.startseqtime)/self.step) * SEQROWSIZE) + SEQLEFTMARGIN + 0.5)
		if track == -1:
			y = 0
		else:
			y = ((track - self.starttrack) * SEQTRACKSIZE) + SEQTOPMARGIN
		return x,y
		
	def pos_to_track_row(self, (x,y)):
		"""
		Converts pixel coordinate to a track and row.
		
		@param x: Pixel coordinate.
		@type x: int
		@param y: Pixel coordinate.
		@type y: int
		@return: Tuple containing track and row index.
		@rtype: (int, int)
		"""
		if x < SEQLEFTMARGIN:
			row = -1
		else:
			row = (((x - SEQLEFTMARGIN) / SEQROWSIZE) * self.step) + self.startseqtime
		if y < SEQTOPMARGIN:
			track = -1
		else:
			track = ((y - SEQTOPMARGIN) / SEQTRACKSIZE) + self.starttrack
		return track,row
		
	def get_endtrack(self):
		"""
		Get the last visible track.
		"""
		w,h = self.get_client_size()
		return self.pos_to_track_row((0,h))[0]
		
	def get_endrow(self):
		"""
		Get the last visible row.
		"""
		w,h = self.get_client_size()
		return self.pos_to_track_row((w,0))[1]
		
	def set_cursor_pos(self, track, row):
		"""
		Updates the cursor position to a track and row.
		
		@param track: Pattern index.
		@type track: int
		@param row: Row index.
		@type row: int
		"""
		seq = player.get_current_sequencer()
		track = max(min(track, seq.get_track_count()-1),0)
		row = max(row,0)
		if (track,row) == (self.track,self.row):
			return
		if self.track != track:
			self.track = track
			endtrack = self.get_endtrack()
			if self.track >= endtrack:
				while self.track >= endtrack:
					self.starttrack += 1
					endtrack = self.get_endtrack()
				self.redraw()
			elif self.track < self.starttrack:
				while self.track < self.starttrack:
					self.starttrack -= 1
				self.redraw()
			self.panel.update_list()
		if self.row != row:
			self.row = row
			endrow = self.get_endrow()
			if self.row >= endrow:
				while self.row >= endrow:
					self.startseqtime += self.step
					endrow = self.get_endrow()
				self.redraw()
			elif self.row < self.startseqtime:
				while self.row < self.startseqtime:
					self.startseqtime -= self.step
				self.redraw()
		t = seq.get_track(self.track)		
		plugin = t.get_plugin()
		self.panel.statuslabels[0].set_label(prepstr('%s' % (self.row)))
		self.panel.statuslabels[1].set_label(prepstr('%s' % (plugin.get_name())))
		self.redraw()

	def insert_at_cursor(self, index = -1):
		"""
		Inserts a space at cursor.
		"""
		seq = player.get_current_sequencer()
		t = seq.get_track(self.track)
		if index != -1:
			pcount = t.get_plugin().get_pattern_count()
			t.set_event(self.row, min(index, 0x10 + pcount-1))
		else:
			t.move_events(self.row, self.step)
		self.redraw()
		
	def delete_at_cursor(self):
		"""
		Deletes pattern at cursor.
		"""		
		seq = player.get_current_sequencer()
		t = seq.get_track(self.track)
		t.remove_event_at(self.row)
		t.move_events(self.row, -self.step)
		self.redraw()
	
	def selection_range(self):
		seq = player.get_current_sequencer()		
		start = (min(self.selection_start[0], self.selection_end[0]), 
					min(self.selection_start[1], self.selection_end[1]))
		end = (max(self.selection_start[0], self.selection_end[0]), 
					max(self.selection_start[1], self.selection_end[1]))
		for track in range(start[0], end[0]+1):
			t = seq.get_track(track)
			events = dict(t.get_event_list())
			for row in range(start[1], end[1]+1):
				if row in events:
					yield track, row, events[row]
				else:
					yield track, row, -1			
				
	def unpack_clipboard_data(self, d):
		"""
		Unpacks clipboard data
		
		@param d: Data that is to be unpacked.
		@type d: unicode
		"""
		magic,d = d[:len(self.CLIPBOARD_SEQUENCER)], d[len(self.CLIPBOARD_SEQUENCER):]
		assert magic == self.CLIPBOARD_SEQUENCER
		while d:
			track,d = int(d[:4],16),d[4:]
			row,d = int(d[:8],16),d[8:]			
			value,d = int(d[:4],16),d[4:]			
			yield track, row, value
	
	def on_popup_copy(self, event=None):
		"""
		Copies the current selection into the clipboard
		"""
		if self.selection_start == None:
			return		
		data = self.CLIPBOARD_SEQUENCER		
		startrow = min(self.selection_start[1], self.selection_end[1])
		for track,row,value in self.selection_range():
			data += "%04x%08x%04x" % (track, row - startrow, value)
		set_clipboard_text(data)
		
	def on_popup_cut(self, event=None):
		self.on_popup_copy(event)
		self.on_popup_delete(event)
		
	def on_popup_paste(self, event=None):	
		seq = player.get_current_sequencer()
		data = get_clipboard_text()
		for track,row,value in self.unpack_clipboard_data(data.strip()):
			t = seq.get_track(track)
			if value == -1:
				t.remove_event_at(self.row + row)
			else:
				t.set_event(self.row + row, value)
			
		self.ReDraw()
		
	def on_popup_delete(self, event):
		seq = player.get_current_sequencer()
		start = (min(self.selection_start[0], self.selection_end[0]), 
			min(self.selection_start[1], self.selection_end[1]))
		end = (max(self.selection_start[0], self.selection_end[0]), 
					max(self.selection_start[1], self.selection_end[1]))
		for track in range(start[0], end[0]+1):
			t = seq.get_track(track)
			for row in range(start[1], end[1]+1):
				t.remove_event_at(row)
		self.ReDraw()
		
	def on_popup_delete_track(self, event):
		"""
		Callback that handles track deletion via the popup menu
		
		@param event: Menu event.
		@type event: wx.CommandEvent
		"""
		seq = player.get_current_sequencer()
		seq.remove_track(self.track)
		track_count = seq.get_track_count()
		# moves cursor if beyond existing tracks
		if self.track > track_count-1:			
			self.set_cursor_pos(track_count-1, self.row)
		self.ReDraw()
		
	def on_popup_add_track(self, event):
		"""
		Callback that handles track addition via the popup menu
		
		@param event: Menu event.
		@type event: wx.CommandEvent
		"""
		seq = player.get_current_sequencer()
		seq.create_track(self.popup_plugins[event.GetId()])
		self.ReDraw()		

	def on_context_menu(self, event):
		"""
		Callback that constructs and displays the popup menu
		
		@param event: Menu event.
		@type event: wx.CommandEvent
		"""
		# create main items only once
		if not hasattr(self, "popup_add_track"):
			self.popup_add_track = wx.NewId()
			self.popup_delete_track = wx.NewId()
			self.popup_cut = wx.NewId()
			self.popup_copy = wx.NewId()
			self.popup_paste = wx.NewId()
			self.popup_delete = wx.NewId()

		# refresh the add plugin listing
		self.popup_plugins = {}
		for plugin in player.get_plugin_list():			
			self.popup_plugins[wx.NewId()] = plugin		
		
		self.Bind(wx.EVT_MENU, self.on_popup_delete_track, id=self.popup_delete_track)
		for plugin_id in self.popup_plugins.keys():
			self.Bind(wx.EVT_MENU, self.on_popup_add_track, id=plugin_id)
			
		menu = wx.Menu()		
		sm = wx.Menu()
		for id, plugin in self.popup_plugins.iteritems():
			sm.Append(id, prepstr(plugin.get_name()))		
		menu.AppendMenu(self.popup_add_track, "Add Track\tCtrl+Enter", sm)
		menu.Append(self.popup_delete_track, "Delete track\tCtrl+Del")
		menu.AppendSeparator()
		menu.Append(self.popup_cut, "Cut")
		menu.Append(self.popup_copy, "Copy")
		menu.Append(self.popup_paste, "Paste")
		menu.Append(self.popup_delete, "Delete")
		self.Bind(wx.EVT_MENU, self.on_popup_copy, id=self.popup_copy)
		self.Bind(wx.EVT_MENU, self.on_popup_paste, id=self.popup_paste)
		self.Bind(wx.EVT_MENU, self.on_popup_cut, id=self.popup_cut)
		self.Bind(wx.EVT_MENU, self.on_popup_delete, id=self.popup_delete)
		# Popup the menu.  If an item is selected then its handler
		# will be called before PopupMenu returns.
		self.PopupMenu(menu)
		menu.Destroy()
		
	def show_plugin_dialog(self):		
		choices = []
		for plugin in player.get_plugin_list():			
			choices.append(prepstr(plugin.get_name()))
		dlg = wx.SingleChoiceDialog(
			self, 
			'', 
			'Select Plugin', 
			choices,
			style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER | wx.CENTRE)
		#~ for child in dlg.GetChildren():
			#~ if isinstance(child,wx.ListBox):
				#~ child.SetFocus()
				#~ break
		if dlg.ShowModal() == wx.ID_OK:			
			seq = player.get_current_sequencer()
			seq.create_track(player.get_plugin(dlg.GetSelection()))
			self.ReDraw()
		dlg.Destroy()
		
	def on_key_down(self, widget, event):
		"""
		Callback that responds to key stroke in sequence view.
		
		@param event: Key event
		@type event: wx.KeyEvent
		"""
		seq = player.get_current_sequencer()
		mask = event.state
		kv = event.keyval
		k = gtk.gdk.keyval_name(event.keyval)
		#~ print kv, k
		arrow_down = k in ['Left', 'Right', 'Up', 'Down', 'KP_Left', 'KP_Right', 'KP_Up', 'KP_Down']
		is_selecting = arrow_down and (mask & gtk.gdk.SHIFT_MASK)
		if is_selecting:
			# starts the selection if nothing selected
			if self.selection_start == None:
				self.selection_start = (self.track, self.row)				
		elif arrow_down:
			self.deselect()
		if (mask & gtk.gdk.SHIFT_MASK):
			if k == 'KP_Add' or k == 'Left' or k == 'KP_Left':
				self.panel.toolbar.increase_step()
				self.set_cursor_pos(self.track, self.row)
			elif k == 'KP_Substract' or k == 'Right' or k == 'KP_Right':
				self.panel.toolbar.decrease_step()
				self.set_cursor_pos(self.track, self.row)
		elif (mask & gtk.gdk.CONTROL_MASK):
			if k == 'Return':
				self.show_plugin_dialog()
			elif k == 'Delete':
				self.on_popup_delete_track(event)
			elif k == 'b':				
				player.set_loop_start(self.row)
				if player.get_loop_end() <= self.row:
					player.set_loop_end(self.row + self.step)
				self.redraw()
			elif k == 'e':
				pos = self.row# + self.step
				if player.get_loop_end() != pos:
					player.set_loop_end(pos)
					if pos > player.get_song_end():
						player.set_song_end(pos)
					if player.get_loop_start() >= pos:
						player.set_loop_start(0)
				else:
					player.set_song_end(pos)
				self.redraw()
			elif k == 'l':			
				mp = seq.get_track(self.track).get_plugin()
				self.rootwindow.routeframe.view.solo(mp)
				self.redraw()
			elif k == 'i':
				for track in seq.get_track_list():
					track.move_events(self.row, self.step)
				self.redraw()	
			elif k == 'c':	
				self.on_popup_copy()
			elif k == 'x':	
				self.on_popup_cut()
			elif k == 'v':	
				self.on_popup_paste()
			elif k == 'Up' or k == 'KP_Up':
				if self.track > 0:
					seq.move_track(self.track, self.track-1)
					self.track -= 1
					self.ReDraw()	
			elif k == 'Down' or k == 'KP_Down':
				if self.track < (seq.get_track_count()-1):
					seq.move_track(self.track, self.track+1)
					self.track += 1
					self.ReDraw()
			elif k == 'Left' or k == 'KP_Left':
				self.set_cursor_pos(self.track, self.row - (self.step*16))
			elif k == 'Right' or k == 'KP_Right':
				self.set_cursor_pos(self.track, self.row + (self.step*16))
			else:
				return False
		elif k == 'Left' or k == 'KP_Left':
			self.set_cursor_pos(self.track, self.row - self.step)
		elif k == 'Right' or k == 'KP_Right':
			self.set_cursor_pos(self.track, self.row + self.step)
		elif k == 'Up' or k == 'KP_Up':
			self.set_cursor_pos(self.track-1, self.row)
		elif k == 'Down' or k == 'KP_Down':
			self.set_cursor_pos(self.track+1, self.row)
		elif (kv < 256) and (chr(kv).lower() in SEQKEYMAP):
			idx = SEQKEYMAP[chr(kv).lower()]
			mp = seq.get_track(self.track).get_plugin()
			if (idx < 0x10) or ((idx-0x10) < mp.get_pattern_count()):
				if (idx >= 0x10):
					newrow = self.row + mp.get_pattern(idx-0x10).get_row_count()
					newrow = newrow - (newrow % self.step)
				else:
					newrow = self.row + self.step
				self.insert_at_cursor(idx)
				self.set_cursor_pos(self.track, newrow)
		elif k == 'space': # space
			spl = self.panel.seqpatternlist
			store, row = spl.get_selection().get_selected_rows()
			row = (row and row[0][0]) or 0
			sel = min(max(row,0),get_item_count(spl.get_model())-1)
			if sel >= 2:
				sel = sel - 2 + 0x10
			self.insert_at_cursor(sel)
			self.set_cursor_pos(self.track, self.row + self.step)
		elif k == 'Delete':
			self.delete_at_cursor()
		elif k == 'Insert':
			self.insert_at_cursor()
		elif k == 'period': # dot
			m,pat,bp = self.get_pattern_at(self.track, self.row, includespecial=True)
			if pat != None:
				if pat >= 0x10:
					pat = m.get_pattern(pat - 0x10)
					length = pat.get_row_count()
				else:
					length = self.step
			else:
				length = 0
			if (self.row < (bp + length)):
				newrow = bp + length
				t = seq.get_track(self.track)
				t.remove_event_at(bp)
				self.set_cursor_pos(self.track, newrow - (newrow % self.step))
				self.redraw()
		elif k == 'Page_Up' or k == 'KP_Page_Up':
			spl = self.panel.seqpatternlist
			store, sel = spl.get_selection().get_selected_rows()
			sel = (sel and sel[0][0]) or 0
			sel = min(max(sel-1,0),get_item_count(spl.get_model())-1)
			spl.get_selection().select_path((sel,))
		elif k == 'Page_Down' or k == 'KP_Page_Down':
			spl = self.panel.seqpatternlist
			store, sel = spl.get_selection().get_selected_rows()
			sel = (sel and sel[0][0]) or 0
			sel = min(max(sel+1,0),get_item_count(spl.get_model())-1)
			spl.get_selection().select_path((sel,))
		elif k == 'Return':
			m, index, bp = self.get_pattern_at(self.track, self.row)
			if index == None:
				track = seq.get_track(self.track)
				self.jump_to_pattern(track.get_plugin())
				return
			self.jump_to_pattern(m, index)
		else:
			return False
		# update selection after cursor movement
		if is_selecting:
			self.selection_end = (self.track, self.row)	
			self.redraw()
		return True
			
	def jump_to_pattern(self, plugin, index=0):
		"""
		Views a pattern in the pattern view.
		
		@param plugin: Plugin.
		@type plugin: zzub.Plugin
		@param index: Pattern index.
		@type index: int
		"""
		mainwindow = self.panel.rootwindow
		pf = mainwindow.patternframe
		tb = pf.toolbar
		tb.plugin = 0
		tb.pattern = 0
		for i in range(player.get_plugin_count()):
			if player.get_plugin(i) == plugin:
				tb.plugin = i
				break
		tb.pattern = index
		pf.update_all()
		mainwindow.select_page(mainwindow.PAGE_PATTERN)
		
	def get_pattern_at(self, track, row, includespecial=False):
		"""
		Gets the pattern and plugin given a sequencer track and row.
		
		@param track: Track index.
		@type track: int
		@param row: Row index.
		@type row: int
		@return: Tuple containing plugin and pattern index.
		@rtype: (zzub.Plugin, int)		
		"""		
		seq = player.get_current_sequencer()
		track = seq.get_track(track)
		plugin = track.get_plugin()
		bestmatch = None
		bestpos = row
		for pos, value in track.get_event_list():
			if pos > row:
				break
			elif includespecial:
				bestpos = pos
				bestmatch = value
			elif (value >= 0x10):
				bestpos = pos
				bestmatch = value - 0x10
		return plugin, bestmatch, bestpos
	
	def deselect(self):
		"""
		Deselects the current selection.
		"""		
		if self.selection_end != None:
			self.dragging = False
			self.selection_end = None
			self.selection_start = None
			self.redraw()
	
	def on_mousewheel(self, event):
		"""
		Callback that responds to mousewheeling in sequencer.
		
		@param event: Mouse event
		@type event: wx.MouseEvent
		"""		
		if event.GetWheelRotation() > 0:
			self.set_cursor_pos(self.track, self.row - self.step)
		else:
			self.set_cursor_pos(self.track, self.row + self.step)
	
	def on_left_down(self, widget, event):
		"""
		Callback that responds to left click down in sequence view.
		
		@param event: Mouse event
		@type event: wx.MouseEvent
		"""
		self.grab_focus()
		if event.button == 1:
			seq = player.get_current_sequencer()
			track_count = seq.get_track_count()
			x, y = int(event.x), int(event.y)
			track, row = self.pos_to_track_row((x,y))		
			if track < track_count:
				if track == -1:
					player.set_position(max(row,0))
				elif row == -1:
					mp = seq.get_track(track).get_plugin()				
					self.rootwindow.routeframe.view.toggle_mute(mp)
					self.redraw()
				else:
					self.set_cursor_pos(track,row)
					self.deselect()
					self.dragging = True
					self.grab_add()
	
	def on_motion(self, widget, event):
		"""
		Callback that responds to mouse motion in sequence view.
		
		@param event: Mouse event
		@type event: wx.MouseEvent
		"""	
		x,y,state = self.window.get_pointer()
		x, y = int(x),int(y)
		if self.dragging:
			select_track, select_row = self.pos_to_track_row((x,y))			
			# start selection if nothing selected			
			if self.selection_start == None:				
				self.selection_start = (self.track, self.row)
			if self.selection_start:
				seq = player.get_current_sequencer()
				select_track = min(seq.get_track_count()-1, max(select_track, 0))
				select_row = max(select_row, 0)
				self.selection_end = (select_track, select_row)
				self.redraw()
				
	def get_client_size(self):
		rect = self.get_allocation()
		return rect.width, rect.height
			
	def expose(self, widget, event):
		self.context = widget.window.cairo_create()
		self.draw(self.context)
		return False
		
	def redraw(self):
		if self.window:
			rect = self.get_allocation()
			self.window.invalidate_rect((0,0,rect.width,rect.height), False)

	def on_left_up(self, widget, event):
		"""
		Callback that responds to left click up in sequence view.
		
		@param event: Mouse event
		@type event: wx.MouseEvent
		"""
		if event.button == 1:
			if self.dragging:
				self.dragging = False
				self.grab_remove()

	def update_position(self):
		"""
		Updates the position.
		"""
		playpos = player.get_position()
		if self.playpos != playpos:
			self.playpos = playpos
			self.redraw()
		return True

	def draw_xor(self):
		"""
		Overriding a L{Canvas} method that is called after painting is completed. 
		Draws an XOR play cursor over the pattern view.
		
		@param dc: wx device context.
		@type dc: wx.PaintDC
		"""
		gc = self.window.new_gc()
		cm = gc.get_colormap()
		drawable = self.window
		w,h = self.get_client_size()
		bbrush = cm.alloc_color('#ffffff')
		gc.set_function(gtk.gdk.XOR)
		gc.set_foreground(bbrush)
		gc.set_background(bbrush)

		seq = player.get_current_sequencer()
		track_count = seq.get_track_count()
		# draw cursor
		if track_count > 0:
			x,y = self.track_row_to_pos((self.track,self.row))
			drawable.draw_rectangle(gc,True,x,y+1,SEQROWSIZE-1,SEQTRACKSIZE-1)
		if self.playpos >= self.startseqtime:
			# draw play cursor
			x = SEQLEFTMARGIN + int((float(self.playpos - self.startseqtime) / self.step) * SEQROWSIZE)
			drawable.draw_rectangle(gc,True,x, 0, 2, h)
			
	def update(self):
		"""
		Updates the view after a lot of data has changed. This will also
		reset selection.
		"""
		self.startseqtime = self.startseqtime - (self.startseqtime % self.step)
		self.selection_start = None
		if self.row != -1:
			self.row = self.row - (self.row % self.step)
		self.redraw()
		
	def draw(self, ctx):
		"""
		Overriding a L{Canvas} method that paints onto an offscreen buffer.
		Draws the pattern view graphics.
		"""	
		st = time.time()
		w,h = self.get_client_size()
		gc = self.window.new_gc()
		cm = gc.get_colormap()
		drawable = self.window
		seq = player.get_current_sequencer()
		cfg = config.get_config()
		bghsb = to_hsb(*cfg.get_float_color('SE BG'))
		bgb = max(bghsb[2],0.1)
		#font = wx.Font(7.5, wx.FONTFAMILY_DEFAULT, wx.FONTSTYLE_NORMAL, wx.FONTWEIGHT_NORMAL)
		type2brush = {
			zzub.zzub_plugin_type_effect : cm.alloc_color(cfg.get_color('MV Effect')),
			zzub.zzub_plugin_type_generator : cm.alloc_color(cfg.get_color('MV Generator')),
			zzub.zzub_plugin_type_master : cm.alloc_color(cfg.get_color('MV Master')),
		}
		bgbrush = cm.alloc_color(cfg.get_color('SE BG'))
		fbrush1 = cm.alloc_color(cfg.get_color('SE BG Very Dark'))
		fbrush2 = cm.alloc_color(cfg.get_color('SE BG Dark'))
		sbrushes = [cm.alloc_color(cfg.get_color('SE Mute')), cm.alloc_color(cfg.get_color('SE Break'))]
		select_brush = cm.alloc_color(cfg.get_color('SE Sel BG'))
		vlinepen = cm.alloc_color(cfg.get_color('SE BG Dark'))
		pen1 = cm.alloc_color(cfg.get_color('SE BG Very Dark'))
		pen2 = cm.alloc_color(cfg.get_color('SE BG Dark'))
		pen = cm.alloc_color(cfg.get_color('SE Line'))
		loop_pen = cm.alloc_color(cfg.get_color('SE Loop Line'))
		invbrush = cm.alloc_color('#ffffff')
		textcolor = cm.alloc_color(cfg.get_color('SE Text'))

		gc.set_foreground(bgbrush)
		gc.set_background(bgbrush)
		drawable.draw_rectangle(gc, True, 0, 0, w, h)
		
		layout = pango.Layout(self.get_pango_context())
		#~ layout.set_font_description(self.fontdesc)
		layout.set_width(-1)

		# 14
		x, y = SEQLEFTMARGIN, SEQTOPMARGIN
		i = self.startseqtime
		while x < w:
			if (i % (16*self.step)) == 0:
				gc.set_foreground(pen1)
				drawable.draw_line(gc, x-1, 0, x-1, h)
				gc.set_foreground(textcolor)
				layout.set_text(str(i))
				px,py = layout.get_pixel_size()
				drawable.draw_layout(gc, x, SEQTRACKSIZE/2 - py/2, layout)
			elif (i % (4*self.step)) == 0:
				gc.set_foreground(pen2)
				drawable.draw_line(gc, x-1, 0, x-1, h)
				gc.set_foreground(textcolor)
				layout.set_text(str(i))
				px,py = layout.get_pixel_size()
				drawable.draw_layout(gc, x, SEQTRACKSIZE/2 - py/2, layout)
			x += SEQROWSIZE
			i += self.step
		gc.set_foreground(pen)
		drawable.draw_line(gc, 0, y, w, y)
		endrow = self.startseqtime + (w / SEQROWSIZE) * self.step
		tracklist = seq.get_track_list()
		if hasattr(self.rootwindow, 'routeframe'):
			solo_plugin = self.rootwindow.routeframe.view.solo_plugin
		else:
			solo_plugin = None
		sel = False
		if self.selection_start != None and self.selection_end != None:
			sel = True
			selstart = (min(self.selection_start[0], self.selection_end[0]), 
				min(self.selection_start[1], self.selection_end[1]))
			selend = (max(self.selection_start[0], self.selection_end[0]), 
				max(self.selection_start[1], self.selection_end[1]))
		for track_index in range(self.starttrack, len(tracklist)):
			track = tracklist[track_index]
			m = track.get_plugin()
			pi = self.plugin_info[m]
			pgfx = pi.patterngfx
			mname = m.get_name()
			title = prepstr(mname)
			if solo_plugin and solo_plugin != m and m.get_type() == zzub.zzub_plugin_type_generator:
				title = "[%s]" % title
			elif self.plugin_info[m].muted:
				title = "(%s)" % title
			gc.set_foreground(type2brush[m.get_type()])
			drawable.draw_rectangle(gc, True, 0, y, SEQLEFTMARGIN-1, SEQTRACKSIZE)
			gc.set_foreground(pen)
			drawable.draw_rectangle(gc, False, 0, y, SEQLEFTMARGIN-1, SEQTRACKSIZE)
			gc.set_foreground(textcolor)
			layout.set_text(title)
			px,py = layout.get_pixel_size()
			drawable.draw_layout(gc, SEQLEFTMARGIN-4 -px, y + SEQTRACKSIZE/2 - py/2, layout)
			intrack = sel and ((track_index >= selstart[0]) and (track_index <= selend[0]))
			# draw selected block if it falls within selection
			if intrack:
				x = SEQLEFTMARGIN
				i = self.startseqtime
				while x < w:
					if (i >= selstart[1]) and (i <= selend[1]):
						gc.foreground = select_brush
						drawable.draw_rectangle(gc, True, x, y+1, SEQROWSIZE-2,SEQTRACKSIZE-2)
					x += SEQROWSIZE
					i += self.step
			for pos, value in track.get_event_list():
				bb = pgfx.get(value, None)
				if not bb:
					if value >= 0x10:
						pat = m.get_pattern(value-0x10)
						name,length = prepstr(pat.get_name()), pat.get_row_count()
					elif value == 0x00:
						name,length = "X", 1
						special = True
					elif value == 0x01:
						name,length = "<", 1
						special = True
					else:
						print "unknown value:",value
						name,length = "???",0
					psize = max(int(((SEQROWSIZE * length) / self.step) + 0.5),2)
					bb = gtk.gdk.Pixmap(self.window, psize-1, SEQTRACKSIZE-1, -1)
					pgfx[value] = bb					
					if value < 0x10:
						gc.set_foreground(sbrushes[value])
						bb.draw_rectangle(gc, True, 0, 0, psize-1, SEQTRACKSIZE-1)
					else:
						random.seed(mname+name)
						hue = random.random()
						cb = 1.0
						r,g,b = from_hsb(hue, 0.2, cb*bgb)
						gc.set_foreground(cm.alloc_color('#%02X%02X%02X' % (int(r*255),int(g*255),int(b*255))))
						bb.draw_rectangle(gc, True, 0,0, psize-2, SEQTRACKSIZE-2)
						r,g,b = from_hsb(hue, 1.0, cb*bgb*0.7)
						gc.set_foreground(cm.alloc_color('#%02X%02X%02X' % (int(r*255),int(g*255),int(b*255))))
						bb.draw_rectangle(gc, False, 0, 0, psize-2, SEQTRACKSIZE-2)
						ofs = 0
						layout.set_text(name)
						px,py = layout.get_pixel_size()
						gc.set_foreground(textcolor)
						bb.draw_layout(gc, 2, 0 + SEQTRACKSIZE/2 - py/2, layout)
				bbw,bbh = bb.get_size()
				x = SEQLEFTMARGIN + (((pos - self.startseqtime) * SEQROWSIZE) / self.step)
				if ((x+bbw) >= SEQLEFTMARGIN) and (x < w):
					ofs = max(SEQLEFTMARGIN - x,0)
					drawable.draw_drawable(gc, bb, ofs, 0, x+ofs, y+1, bbw-ofs, bbh)
					if intrack and (pos >= selstart[1]) and (pos <= selend[1]):
						gc.set_foreground(invbrush)
						gc.set_function(gtk.gdk.XOR)
						drawable.draw_rectangle(gc, True, x+ofs, y+1, bbw-ofs, bbh)
						gc.set_function(gtk.gdk.COPY)
			y += SEQTRACKSIZE			
			gc.set_foreground(vlinepen)
			drawable.draw_line(gc, SEQLEFTMARGIN, y, w, y)
		gc.set_foreground(pen)
		x = SEQLEFTMARGIN-1
		drawable.draw_line(gc, x, 0, x, h)
		se = player.get_song_end()
		x,y = self.track_row_to_pos((0,se))
		if (x >= SEQLEFTMARGIN):
			gc.set_foreground(pen)
			drawable.draw_line(gc, x-1, 0, x-1, h)
		gc.set_foreground(loop_pen)
		gc.line_style = gtk.gdk.LINE_ON_OFF_DASH
		gc.set_dashes(0, (1,1))
		lb,le = player.get_song_loop()
		x,y = self.track_row_to_pos((0,lb))
		if (x >= SEQLEFTMARGIN):
			drawable.draw_line(gc, x-1, 0, x-1, h)
		x,y = self.track_row_to_pos((0,le))
		if (x >= SEQLEFTMARGIN):
			drawable.draw_line(gc, x-1, 0, x-1, h)
		self.draw_xor()
		#print "%.2fms" % ((time.time() - st)*1000)

__all__ = [
'PatternNotFoundException',
'PluginDialog',
'SequencerPanel',
'SequencerView',
]

if __name__ == '__main__':
	import testplayer, utils
	player = testplayer.get_player()
	player.load_ccm(utils.filepath('demosongs/paniq-knark.ccm'))
	window = testplayer.TestWindow()
	window.add(SequencerPanel(window))
	window.show_all()
	gtk.main()
