

import gi
gi.require_version("Gtk", "3.0")
gi.require_version('PangoCairo', '1.0')
from gi.repository import Gtk, Gdk, Pango, PangoCairo

import time
import cairo
import sys

from neil.utils import (
    prepstr, get_new_pattern_name, ui,
    get_plugin_color_name, is_instrument
)

import random
import config

import neil.common as common
from neil import components
import zzub

from .add_track import AddSequencerTrackDialog
from .utils import Seq


def get_random_color(seed):
    """Generates a random color in float format."""
    random.seed(seed)
    r = random.random() * 0.4 + 0.599
    g = random.random() * 0.4 + 0.599
    b = random.random() * 0.4 + 0.599
    return (r, g, b)


class PatternNotFoundException(Exception):
    """
    Exception thrown when pattern is not found.
    """
    pass



class SequencerView(Gtk.DrawingArea):
    """
    Sequence viewer class.
    """
    CLIPBOARD_SEQUENCER = "SEQUENCERDATA"

    def __init__(self, panel, hscroll, vscroll):
        """
        Initialization.
        """
        self.panel = panel
        self.hscroll = hscroll
        self.vscroll = vscroll

        self.needfocus = True

        # Variables that were previously defined as globals.
        self.seq_track_size = 36
        self.seq_step = 16
        self.seq_left_margin = 96
        self.seq_top_margin = 20
        self.seq_row_size = 38

        self.pango_ctx = None

        self.plugin_info = common.get_plugin_infos()
        player = components.get('neil.core.player')
        self.playpos = player.get_position()
        self.row = 0
        self.track = 0
        self.startseqtime = 0
        self.starttrack = 0
        self.step = config.get_config().get_default_int('SequencerStep', self.seq_step)
        self.wmax = 0
        player.set_loop_end(self.step)
        self.selection_start = None
        self.selection_end = None
        self.dragging = False
        Gtk.DrawingArea.__init__(self)

        self.add_events(Gdk.EventMask.ALL_EVENTS_MASK)
        self.set_property('can-focus', True)
        self.connect("draw", self.on_draw)
        self.connect('key-press-event', self.on_key_down)
        self.connect('button-press-event', self.on_left_down)
        self.connect('motion-notify-event', self.on_motion)
        self.connect('button-release-event', self.on_left_up)
        self.connect('scroll-event', self.on_mousewheel)
        self.hscroll.connect('change-value', self.on_hscroll_window)
        self.vscroll.connect('change-value', self.on_vscroll_window)
        # GObject.timeout_add(100, self.update_position)
        eventbus = components.get('neil.core.eventbus')
        eventbus.zzub_sequencer_changed += self.redraw
        eventbus.zzub_set_sequence_event += self.redraw
        eventbus.document_loaded += self.redraw
        ui.set_clipboard_text("invalid_clipboard_data")

    def track_row_to_pos(self, xxx_todo_changeme):
        """
        Converts track and row to a pixel coordinate.

        @param track: Track index.
        @type track: int
        @param row: Row index.
        @type row: int
        @return: Pixel coordinate.
        @rtype: (int, int)
        """
        (track, row) = xxx_todo_changeme
        if row == -1:
            x = 0
        else:
            x = (int((float(row) - self.startseqtime) / self.step) * self.seq_row_size) + self.seq_left_margin
        if track == -1:
            y = 0
        else:
            y = ((track - self.starttrack) * self.seq_track_size) + self.seq_top_margin
        return x, y

    def pos_to_track_row(self, xxx_todo_changeme1):
        """
        Converts pixel coordinate to a track and row.

        @param x: Pixel coordinate.
        @type x: int
        @param y: Pixel coordinate.
        @type y: int
        @return: Tuple containing track and row index.
        @rtype: (int, int)
        """
        (x, y) = xxx_todo_changeme1
        if x < self.seq_left_margin:
            row = -1
        else:
            row = (((x - self.seq_left_margin) / self.seq_row_size) * self.step) + self.startseqtime
        if y < self.seq_top_margin:
            track = -1
        else:
            track = ((y - self.seq_top_margin) / self.seq_track_size) + self.starttrack
        return int(track), int(row)

    def get_endtrack(self):
        """
        Get the last visible track.
        """
        w, h = self.get_client_size()
        return self.pos_to_track_row((0, h))[0]

    def get_endrow(self):
        """
        Get the last visible row.
        """
        w, h = self.get_client_size()
        return self.pos_to_track_row((w, 0))[1]

    def set_cursor_pos(self, track, row):
        """
        Updates the cursor position to a track and row.

        @param track: Pattern index.
        @type track: int
        @param row: Row index.
        @type row: int
        """
        player = components.get('neil.core.player')
        seq = player.get_current_sequencer()
        track = max(min(track, seq.get_sequence_track_count() - 1), 0)
        row = max(row, 0)
        if (track, row) == (self.track, self.row):
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
        self.panel.statuslabels[0].set_label(prepstr('%s' % (self.row)))
        t = self.get_track()
        if t:
            plugin = t.get_plugin()
            self.panel.statuslabels[1].set_label(prepstr('%s' % (plugin.get_name())))
        else:
            self.panel.statuslabels[1].set_label("")
        self.redraw()

    def get_track(self):
        player = components.get('neil.core.player')
        seq = player.get_current_sequencer()
        if (self.track != -1) and (self.track < seq.get_sequence_track_count()):
            return seq.get_sequence(self.track)
        return None

    # FIXME why does this fail when master is the only plugin and no patterns exist?
    def create_track(self, plugin):
        # get sequencer and add the track
        player = components.get('neil.core.player')
        seq = player.get_current_sequencer()
        track = seq.create_sequence(plugin, zzub.zzub_sequence_type_pattern)
        # if it has no existing patterns, make one (even if it has no parameters, it might have incoming connections)
        if plugin.get_pattern_count() == 0:
            pattern = plugin.create_pattern(player.sequence_step)
            pattern.set_name('00')
            plugin.add_pattern(pattern)
            # add a pattern trigger-event
            track.set_event(0, 16)
        player.history_commit("add track")
        self.adjust_scrollbars()
        self.redraw()

    def insert_at_cursor(self, index = -1):
        """
        Inserts a space at cursor.
        """
        player = components.get('neil.core.player')
        t = self.get_track()
        if not t:
            return
        if index != -1:
            pcount = t.get_plugin().get_pattern_count()
            t.set_event(self.row, min(index, 0x10 + pcount - 1))
        else:
            t.insert_events(self.row, self.step)
        player.history_commit("insert sequence")

    def delete_at_cursor(self):
        """
        Deletes pattern at cursor.
        """
        player = components.get('neil.core.player')
        t = player.get_sequence(self.track)
        t.remove_events(self.row, self.step)
        player.history_commit("delete sequences")

    def selection_range(self):
        player = components.get('neil.core.player')
        start = (min(self.selection_start[0], self.selection_end[0]),
                                min(self.selection_start[1], self.selection_end[1]))
        end = (max(self.selection_start[0], self.selection_end[0]),
                                max(self.selection_start[1], self.selection_end[1]))
        for track in range(start[0], end[0] + 1):
            t = player.get_sequence(track)
            events = dict(t.get_event_list())
            for row in range(start[1], end[1] + 1):
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
        magic, d = d[:len(self.CLIPBOARD_SEQUENCER)], d[len(self.CLIPBOARD_SEQUENCER):]
        if magic != self.CLIPBOARD_SEQUENCER:
            raise ValueError
        while d:
            track, d = int(d[:4], 16), d[4:]
            row, d = int(d[:8], 16), d[8:]
            value, d = int(d[:4], 16), d[4:]
            yield track, row, value

    def on_popup_copy(self, *args):
        """
        Copies the current selection into the clipboard
        """
        #print self.selection_start, self.selection_end
        if self.selection_start == None:
            return
        data = self.CLIPBOARD_SEQUENCER
        startrow = min(self.selection_start[1], self.selection_end[1])
        for track, row, value in self.selection_range():
            data += "%04x%08x%04x" % (track, row - startrow, value)
        ui.set_clipboard_text(data)

    def on_popup_create_pattern(self, *args):
        player = components.get('neil.core.player')
        seq = player.get_current_sequencer()
        if not self.selection_start:
            self.selection_start = (self.track, self.row)
            self.selection_end = (self.track, self.row)
        try:
            start = (min(self.selection_start[0], self.selection_end[0]),
                     min(self.selection_start[1], self.selection_end[1]))
            end = (max(self.selection_start[0], self.selection_end[0]),
                   max(self.selection_start[1], self.selection_end[1]))
        except TypeError:
            # There is no selection.
            return
        for t in seq.get_track_list()[start[0]:end[0] + 1]:
            m = t.get_plugin()
            patternsize = end[1] + self.step - start[1]
            name = get_new_pattern_name(m)
            p = m.create_pattern(patternsize)
            p.set_name(name)
            m.add_pattern(p)
            for i in range(m.get_pattern_count()):
                pattern = m.get_pattern(i)
                if pattern.get_name() == name:
                    t.set_event(start[1], 0x10 + i)
                    break
            player.history_commit("new pattern")

    def on_popup_clone_pattern(self, *args):
        """Create a copy of the pattern at cursor and replace the current one.
        """
        from neil.utils import get_new_pattern_name
        player = components.get('neil.core.player')
        machine, pattern_id, _ = self.get_pattern_at(self.track, self.row, includespecial=True)
        if pattern_id is None:
            return

        pattern_id -= 0x10
        name = get_new_pattern_name(machine)
        pattern = machine.get_pattern(pattern_id)
        pattern.set_name(name)
        machine.add_pattern(pattern)
        self.insert_at_cursor(machine.get_pattern_count() + 0x10 - 1)
        player.history_commit("clone pattern")
        self.update_list()

    def on_popup_merge(self, *args):
        player = components.get('neil.core.player')
        player.set_callback_state(False)
        seq = player.get_current_sequencer()
        try:
            start = (min(self.selection_start[0], self.selection_end[0]),
                     min(self.selection_start[1], self.selection_end[1]))
            end = (max(self.selection_start[0], self.selection_end[0]),
                   max(self.selection_start[1], self.selection_end[1]))
        except TypeError:
            # There is no selection.
            return

        for t in seq.get_track_list()[start[0]:(end[0] + 1)]:
            patternsize = 0
            eventlist = []
            m = t.get_plugin()
            for time, value in t.get_event_list():
                if (time >= start[1]) and (time < (end[1] + self.step)):
                    if value >= 0x10:
                        value -= 0x10
                        # copy contents between patterns
                        eventlist.append((time, m.get_pattern(value)))
                        patternsize = max(patternsize, time - start[1] +
                                          m.get_pattern(value).get_row_count())
            if patternsize:
                name = get_new_pattern_name(m)
                p = m.create_pattern(patternsize)
                p.set_name(name)
                #m.add_pattern(p)
                group_track_count = [m.get_input_connection_count(),
                                     1, m.get_track_count()]
                for time, pattern in eventlist:
                    t.set_event(time, -1)
                    for r in range(pattern.get_row_count()):
                        rowtime = time - start[1] + r
                        for g in range(3):
                            for ti in range(group_track_count[g]):
                                for i in range(m.get_pluginloader().\
                                                    get_parameter_count(g)):
                                    p.set_value(rowtime, g, ti, i,
                                                pattern.get_value(r, g, ti, i))
                m.add_pattern(p)
                for i in range(m.get_pattern_count()):
                    if m.get_pattern(i).get_name() == name:
                        t.set_event(start[1], 0x10 + i)
                        break
        player.history_commit("merge patterns")
        player.set_callback_state(True)
        eventbus = components.get('neil.core.eventbus')
        eventbus.document_loaded()
        self.update_list()

    def on_popup_cut(self, *args):
        self.on_popup_copy(*args)
        self.on_popup_delete(*args)

    # FIXME it would be nice if pasting moved the cursor forward, so that
    # repeated pasting would work
    def on_popup_paste(self, *args):
        player = components.get('neil.core.player')
        player.set_callback_state(False)
        seq = player.get_current_sequencer()
        data = ui.get_clipboard_text()
        try:
            for track, row, value in self.unpack_clipboard_data(data.strip()):
                t = seq.get_sequence(track)
                if value == -1:
                    t.set_event(self.row + row, -1)
                else:
                    t.set_event(self.row + row, value)
            player.history_commit("paste selection")
        except ValueError:
            pass
        player.set_callback_state(True)
        eventbus = components.get('neil.core.eventbus')
        eventbus.document_loaded()
        self.update_list()

    def on_popup_delete(self, *args):
        player = components.get('neil.core.player')
        player.set_callback_state(False)
        seq = player.get_current_sequencer()
        #print self.selection_start
        try:
            start = (min(self.selection_start[0], self.selection_end[0]),
                     min(self.selection_start[1], self.selection_end[1]))
            end = (max(self.selection_start[0], self.selection_end[0]),
                   max(self.selection_start[1], self.selection_end[1]))
        except TypeError:
            # There is no selection.
            return
        for track in range(start[0], end[0] + 1):
            t = seq.get_sequence(track)
            for row in range(start[1], end[1] + 1):
                t.set_event(row, -1)
        player.history_commit("delete selection")
        player.set_callback_state(True)
        eventbus = components.get('neil.core.eventbus')
        eventbus.document_loaded()
        self.update_list()

    def on_popup_delete_track(self, *args):
        """
        Callback that handles track deletion via the popup menu

        @param event: Menu event.
        @type event: wx.CommandEvent
        """
        player = components.get('neil.core.player')
        seq = player.get_current_sequencer()
        t = seq.get_track_list()[self.track]
        t.destroy()
        track_count = seq.get_sequence_track_count()
        player.history_commit("delete track")
        # moves cursor if beyond existing tracks
        if self.track > track_count - 1:
            self.set_cursor_pos(track_count - 1, self.row)
        self.adjust_scrollbars()
        self.redraw()

    def on_popup_add_track(self, widget, plugin):
        """
        Callback that handles track addition via the popup menu

        @param event: Menu event.
        @type event: wx.CommandEvent
        """
        self.create_track(plugin)

    def on_popup_record_to_wave(self, widget, index):
        """
        Callback that is used to record a looped song section
        to an instrument slot.
        """
        player = components.get('neil.core.player')
        loader = player.get_pluginloader_by_name('@zzub.org/recorder/wavetable')
        if not loader:
            print("Can't find instrument recorder plugin loader.", file=sys.stderr)
            return
        flags = zzub.zzub_plugin_flag_no_undo | zzub.zzub_plugin_flag_no_save
        recorder = zzub.Player.create_plugin(player, None, 0, "_IRecorder", loader, flags)
        if not recorder:
            print("Can't create instrument recorder plugin instance.", file=sys.stderr)
            return
        master = player.get_plugin(0)
        recorder.add_input(master, zzub.zzub_connection_type_audio)
        player.set_machine_non_song(recorder, True)
        recorder.set_parameter_value(zzub.zzub_parameter_group_global, 0, 0, index, False)
        recorder.set_parameter_value(zzub.zzub_parameter_group_global, 0, 1, 1, False)
        old_song_end = player.get_song_end()
        old_position = player.get_position()
        old_loop_enabled = player.get_loop_enabled()
        player.set_song_end(player.get_loop_end())
        player.set_position(player.get_loop_start())
        player.set_loop_enabled(0)
        player.play()
        dialog = Gtk.Dialog(
            "Recording",
            buttons=(Gtk.STOCK_OK, True)
            )
        dialog.get_content_area().add(Gtk.Label(label="Press OK when the recording is done."))
        dialog.show_all()
        dialog.run()
        recorder.destroy()
        dialog.destroy()
        player.set_song_end(old_song_end)
        player.set_position(old_position)
        player.set_loop_enabled(old_loop_enabled)
        player.flush(None, None)
        player.history_flush_last()

    def on_context_menu(self, event):
        """
        Callback that constructs and displays the popup menu

        @param event: Menu event.
        @type event: wx.CommandEvent
        """
        player = components.get('neil.core.player')
        seq = player.get_current_sequencer()
        x, y = int(event.x), int(event.y)
        track, row = self.pos_to_track_row((x, y))
        self.set_cursor_pos(max(min(track, seq.get_sequence_track_count()), 0), self.row)
        if self.selection_start != None:
            sel_sensitive = True
        else:
            sel_sensitive = False
        if ui.get_clipboard_text().startswith(self.CLIPBOARD_SEQUENCER):
            paste_sensitive = True
        else:
            paste_sensitive = False
        menu = ui.EasyMenu()
        pmenu = ui.EasyMenu()
        wavemenu = ui.EasyMenu()
        for plugin in sorted(list(player.get_plugin_list()), key=lambda plugin: plugin.get_name().lower()):
            pmenu.add_item(prepstr(plugin.get_name().replace("_", "__")), self.on_popup_add_track, plugin)
        for i, name in enumerate(ui.wave_names_generator()):
            wavemenu.add_item(name, self.on_popup_record_to_wave, i + 1)
        menu.add_submenu("Add track", pmenu)
        menu.add_item("Delete track", self.on_popup_delete_track)
        menu.add_separator()
        menu.add_item("Set loop start", self.set_loop_start)
        menu.add_item("Set loop end", self.set_loop_end)
        menu.add_separator()
        menu.add_submenu("Record loop", wavemenu)
        menu.add_separator()
        menu.add_item("Cut", self.on_popup_cut).set_sensitive(sel_sensitive)
        menu.add_item("Copy", self.on_popup_copy).set_sensitive(sel_sensitive)
        menu.add_item("Paste", self.on_popup_paste).set_sensitive(paste_sensitive)
        menu.add_item("Delete", self.on_popup_delete).set_sensitive(sel_sensitive)
        menu.add_separator()
        menu.add_item("Create pattern", self.on_popup_create_pattern)
        menu.add_item("Clone pattern", self.on_popup_clone_pattern)
        menu.add_item("Merge patterns", self.on_popup_merge).set_sensitive(sel_sensitive)
        menu.show_all()
        menu.attach_to_widget(self, None)
        menu.popup(self, event)

    def show_plugin_dialog(self):
        pmenu = []
        player = components.get('neil.core.player')
        for plugin in player.get_plugin_list():
            pmenu.append(prepstr(plugin.get_name()))
        dlg = AddSequencerTrackDialog(self, pmenu)
        response = dlg.run()
        dlg.hide()
        if response == Gtk.ResponseType.OK:
            name = dlg.combo.get_active_text()
            for plugin in player.get_plugin_list():
                if plugin.get_name() == name:
                    self.create_track(plugin)
                    break
        dlg.destroy()

    def set_loop_start(self, *args):
        """
        Set loop startpoint
        """
        player = components.get('neil.core.player')
        player.set_loop_start(self.row)
        if player.get_loop_end() <= self.row:
            player.set_loop_end(self.row + self.step)
        self.redraw()

    def set_loop_end(self, *args):
        player = components.get('neil.core.player')
        pos = self.row  # + self.step
        if player.get_loop_end() != pos:
            player.set_loop_end(pos)
            if pos > player.get_song_end():
                player.set_song_end(pos)
            if player.get_loop_start() >= pos:
                player.set_loop_start(0)
        else:
            player.set_song_end(pos)
        self.redraw()

    def on_key_down(self, widget=None, event=None):
        """
        Callback that responds to key stroke in sequence view.

        @param event: Key event
        @type event: wx.KeyEvent
        """
        player = components.get('neil.core.player')
        seq = player.get_current_sequencer()
        mask = event.get_state()
        kv = event.keyval
        # convert keypad numbers
        if Gdk.keyval_from_name('KP_0') <= kv <= Gdk.keyval_from_name('KP_9'):
            kv = kv - Gdk.keyval_from_name('KP_0') + Gdk.keyval_from_name('0')
        k = Gdk.keyval_name(event.keyval)
        #print kv, k, event.keyval
        arrow_down = k in ['Left', 'Right', 'Up', 'Down', 'KP_Left', 'KP_Right', 'KP_Up', 'KP_Down']
        is_selecting = arrow_down and (mask & Gdk.ModifierType.SHIFT_MASK)
        if is_selecting:
            # starts the selection if nothing selected
            if self.selection_start == None:
                self.selection_start = (self.track, self.row)
        elif arrow_down:
            self.deselect()
        if mask & Gdk.ModifierType.SHIFT_MASK and (k in ('KP_Add', 'plus', 'asterisk')):
            self.panel.toolbar.increase_step()
            self.set_cursor_pos(self.track, self.row)
        elif mask & Gdk.ModifierType.SHIFT_MASK and (k in ('KP_Subtract', 'underscore')):
            self.panel.toolbar.decrease_step()
            self.set_cursor_pos(self.track, self.row)
        elif (mask & Gdk.ModifierType.CONTROL_MASK):
            if k == 'Return':
                self.show_plugin_dialog()
            elif k == 'Delete':
                self.on_popup_delete_track(event)
                self.adjust_scrollbars()
            elif k == 'b':
                self.set_loop_start()
            elif k == 'e':
                self.set_loop_end()
            elif k == 'l':
                t = self.get_track()
                if t:
                    mp = t.get_plugin()
                    player.solo(mp)
                    self.redraw()
            elif k == 'i':
                for track in seq.get_track_list():
                    track.insert_events(self.row, self.step)
                player.history_commit("insert event")
            elif k == 'd':
                for track in seq.get_track_list():
                    track.remove_events(self.row, self.step)
                player.history_commit("remove event")
            elif k == 'c':
                self.on_popup_copy()
            elif k == 'x':
                self.on_popup_cut()
            elif k == 'v':
                self.on_popup_paste()
            elif k == 'Up' or k == 'KP_Up':
                if self.track > 0:
                    t = seq.get_track_list()[self.track]
                    t.move(self.track - 1)
                    self.track -= 1
                    player.history_commit("move track")
                    self.redraw()
            elif k == 'Down' or k == 'KP_Down':
                if self.track < (seq.get_sequence_track_count() - 1):
                    t = seq.get_track_list()[self.track]
                    t.move(self.track + 1)
                    self.track += 1
                    player.history_commit("move track")
                    self.redraw()
            elif k == 'Left' or k == 'KP_Left':
                self.set_cursor_pos(self.track, self.row - (self.step * 16))
            elif k == 'Right' or k == 'KP_Right':
                self.set_cursor_pos(self.track, self.row + (self.step * 16))
            else:
                return False
        elif k == 'Left' or k == 'KP_Left':
            self.set_cursor_pos(self.track, self.row - self.step)
            self.adjust_scrollbars()
        elif k == 'Right' or k == 'KP_Right':
            pass
            self.set_cursor_pos(self.track, self.row + self.step)
            self.adjust_scrollbars()
        elif k == 'Up' or k == 'KP_Up':
            self.set_cursor_pos(self.track - 1, self.row)
            self.adjust_scrollbars()
        elif k == 'Down' or k == 'KP_Down':
            self.set_cursor_pos(self.track + 1, self.row)
            self.adjust_scrollbars()
        elif ((kv < 256) and (chr(kv).lower() in Seq.map) and
              self.selection_start == None and self.selection_end == None):
            idx = Seq.map[chr(kv).lower()]
            t = self.get_track()
            if t:
                mp = t.get_plugin()
                if (idx < 0x10) or ((idx - 0x10) < mp.get_pattern_count()):
                    if (idx >= 0x10):
                        newrow = self.row + mp.get_pattern(idx - 0x10).get_row_count()
                        newrow = newrow - (newrow % self.step)
                    else:
                        newrow = self.row + self.step

                    self.insert_at_cursor(idx)
                    player.history_commit("add pattern reference")
                    self.set_cursor_pos(self.track, newrow)
                    #print self.track, self.row
                    self.adjust_scrollbars()
        elif k == 'space':  # space
            spl = self.panel.seqpatternlist
            store, row = spl.get_selection().get_selected_rows()
            row = (row and row[0][0]) or 0
            sel = min(max(row, 0), ui.get_item_count(spl.get_model()) - 1)
            if sel >= 2:
                sel = sel - 2 + 0x10
            self.insert_at_cursor(sel)
            self.set_cursor_pos(self.track, self.row + self.step)
        elif k == 'Delete':
            self.delete_at_cursor()
            self.adjust_scrollbars()
        elif k == 'Insert' or  k == 'KP_Insert':
            self.insert_at_cursor()
            self.adjust_scrollbars()
        elif k == 'period':  # dot
            m, pat, bp = self.get_pattern_at(self.track, self.row, includespecial=True)
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
                t = seq.get_sequence(self.track)
                t.set_event(bp, -1)
                player.history_commit("remove pattern reference")
                self.set_cursor_pos(self.track, newrow - (newrow % self.step))
            else:
                self.set_cursor_pos(self.track, self.row + self.step)
        elif k == 'Home' or k == 'KP_Home':
            self.set_cursor_pos(self.track, 0)
        elif k == 'End' or k == 'KP_End':
            self.set_cursor_pos(self.track, player.get_song_end() - self.step)
        elif k == 'Page_Up' or k == 'KP_Page_Up':
            spl = self.panel.seqpatternlist
            store, sel = spl.get_selection().get_selected_rows()
            sel = (sel and sel[0][0]) or 0
            sel = min(max(sel - 1, 0), ui.get_item_count(spl.get_model()) - 1)
            spl.get_selection().select_path((sel,))
        elif k == 'Page_Down' or k == 'KP_Page_Down':
            spl = self.panel.seqpatternlist
            store, sel = spl.get_selection().get_selected_rows()
            sel = (sel and sel[0][0]) or 0
            sel = min(max(sel + 1, 0), ui.get_item_count(spl.get_model()) - 1)
            spl.get_selection().select_path((sel,))
        elif k == 'Return':
            m, index, bp = self.get_pattern_at(self.track, self.row)
            if index == None:
                track = self.get_track()
                if track:
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
        eventbus = components.get('neil.core.eventbus')
        eventbus.edit_pattern_request(plugin, index)

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
        track = self.get_track()
        if not track:
            return None, None, -1
        plugin = track.get_plugin()
        bestmatch = None
        bestpos = row
        event_list = track.get_event_list()

        pos, value= 0, 0x10
        for pos, value in event_list:
            if pos > row:
                break
            elif includespecial:
                bestpos = pos
                bestmatch = value
            elif (value >= 0x10):
                bestpos = pos
                bestmatch = value - 0x10

        pattern = plugin.get_pattern(value - 0x10)

        nrows = pattern.get_row_count()
        if pos + nrows < row + 1:
            return plugin, None, -1
        else:
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

    def on_mousewheel(self, widget, event):
        """
        Callback that responds to mousewheeling in sequencer.

        @param event: Mouse event
        @type event: wx.MouseEvent
        """
        if event.get_state() & Gdk.ModifierType.CONTROL_MASK:
            if event.direction == Gdk.ScrollDirection.DOWN:
                self.panel.toolbar.increase_step()
                self.set_cursor_pos(self.track, self.row)
            elif event.direction == Gdk.ScrollDirection.UP:
                self.panel.toolbar.decrease_step()
                self.set_cursor_pos(self.track, self.row)
        elif event.direction == Gdk.ScrollDirection.UP:
            self.set_cursor_pos(self.track, self.row - self.step)
        elif event.direction == Gdk.ScrollDirection.DOWN:
            self.set_cursor_pos(self.track, self.row + self.step)

    def on_left_down(self, widget, event):
        """
        Callback that responds to left click down in sequence view.

        @param event: Mouse event
        @type event: wx.MouseEvent
        """
        self.grab_focus()
        player = components.get('neil.core.player')
        track_count = player.get_sequence_track_count()
        x, y = int(event.x), int(event.y)
        track, row = self.pos_to_track_row((x, y))
        if event.button == 1:
            if track < track_count:
                if track == -1:
                    player.set_position(max(row, 0))
                elif row == -1:
                    mp = player.get_sequence(track).get_plugin()
                    player.toggle_mute(mp)
                    self.redraw()
                else:
                    self.set_cursor_pos(track, row - row % self.step)
                    self.deselect()
                    self.dragging = True
                    self.grab_add()
            if event.type == Gdk.EventType._2BUTTON_PRESS:  # double-click
                m, index, bp = self.get_pattern_at(self.track, self.row)
                if index == None:
                    track = self.get_track()
                    if track:
                        self.jump_to_pattern(track.get_plugin())
                        return
                self.jump_to_pattern(m, index)

        elif event.button == 3:
            if (x < self.seq_left_margin) and (track < track_count):
                mp = player.get_sequence(track).get_plugin()
                menu = components.get('neil.core.contextmenu.singleplugin', mp)
                menu.popup(self, event)
                return
            self.on_context_menu(event)

    def on_motion(self, widget, event):
        x, y, state = int(event.x), int(event.y), event.state
        x = max(int(x), self.seq_left_margin)
        if self.dragging:
            select_track, select_row = self.pos_to_track_row((x, y))
            # start selection if nothing selected
            if self.selection_start == None:
                self.selection_start = (self.track, self.row)
            if self.selection_start:
                player = components.get('neil.core.player')
                seq = player.get_current_sequencer()
                select_track = min(seq.get_sequence_track_count() - 1,
                                   max(select_track, 0))
                select_row = max(select_row, 0)
                self.selection_end = (select_track, select_row)
                # If the user didn't drag enough to select more than one cell,
                # we reset.
                if (self.selection_start[0] == self.selection_end[0] and
                    self.selection_start[1] == self.selection_end[1]):
                    self.selection_start = None
                    self.selection_end = None
                self.redraw()

    def get_client_size(self):
        rect = self.get_allocation()
        return rect.width, rect.height

    def redraw(self, *args):
        if self.get_window() and self.is_visible():
            rect = self.get_allocation()
            self.get_window().invalidate_rect(Gdk.Rectangle(0, 0, rect.width, rect.height), False)

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

    # def update_position(self):
    #     """
    #     Updates the position.
    #     """
    #     #TODO: find a better way to find out whether we are visible
    #     #if self.rootwindow.get_current_panel() != self.panel:
    #     #       return True:

    #     if not self.get_window():
    #         return
    #     player = components.get('neil.core.player')
    #     playpos = player.get_position()
    #     print("set playpos", self.playpos, playpos)
    #     if self.playpos != playpos:
    #         if self.panel.toolbar.followsong.get_active():
    #             if playpos >= self.get_endrow() or playpos < self.startseqtime:
    #                 self.startseqtime = int(playpos / self.step) * self.step
    #                 self.redraw()
    #         #self.draw_cursors()
    #         ctx = self.get_window().cairo_create()
    #         self.draw_playpos(ctx)
    #         self.playpos = playpos
    #         self.draw_playpos(ctx)
    #         #self.redraw()
    #     return True

    def on_vscroll_window(self, widget, scroll, value):
        """
        Handles vertical window scrolling.
        """
        adj = widget.get_adjustment()
        minv = adj.get_property('lower')
        maxv = adj.get_property('upper')
        pagesize = adj.get_property('page-size')
        value = int(max(min(value, maxv - pagesize), minv) + 0.5)
        widget.set_value(value)
        self.redraw()
        if self.starttrack != value:
            self.starttrack = value
            self.redraw()
        return True

    def on_hscroll_window(self, widget, scroll, value):
        """
        Handles horizontal window scrolling.
        """
        adj = widget.get_adjustment()
        minv = adj.get_property('lower')
        maxv = adj.get_property('upper')
        pagesize = adj.get_property('page-size')
        value = int(max(min(value, maxv - pagesize), minv) + 0.5)
        widget.set_value(value)
        if self.startseqtime != value * self.step:
            self.startseqtime = value * self.step
            self.redraw()
        return True

    def adjust_scrollbars(self):
        w, h = self.get_client_size()
        vw, vh = self.get_virtual_size()
        pw, ph = (int((w - self.seq_left_margin) /
                     float(self.seq_row_size) + 0.5),
                  int((h - self.seq_top_margin) /
                      float(self.seq_track_size) + 0.5))
        #print w, h
        #print vw, vh
        #print pw, ph
        hrange = vw - pw
        vrange = vh - ph
        if hrange <= 0:
            self.hscroll.hide()
        else:
            self.hscroll.show()
        if vrange <= 0:
            self.vscroll.hide()
        else:
            self.vscroll.show()
        adj = self.hscroll.get_adjustment()
        adj.configure(self.startseqtime / self.step, 0,
                    int(vw + (w - self.seq_left_margin) /
                        float(self.seq_row_size) - 2),
                    1, 1, pw)
        adj = self.vscroll.get_adjustment()
        adj.configure(self.starttrack, 0, vh, 1, 1, ph)
        #self.redraw()

    def get_virtual_size(self):
        """
        Returns the size in characters of the virtual view area.
        """
        player = components.get('neil.core.player')
        seq = player.get_current_sequencer()
        tracklist = seq.get_track_list()
        total_length = 0
        for track_index in range(self.starttrack, len(tracklist)):
            track = tracklist[track_index]
            m = track.get_plugin()
            track_length = 0
            for pos, value in track.get_event_list():
                if value >= 0x10:
                    pat = m.get_pattern(value - 0x10)
                    length = pat.get_row_count()
                    pat.destroy()
                elif value == 0x00:
                    length = self.step
                elif value == 0x01:
                    length = self.step
                track_length += length
            if track_length > total_length:
                total_length = track_length
        h = seq.get_sequence_track_count()
        w = (max(self.row, player.get_song_end(), player.get_loop_end(), \
                total_length) / self.step + 3)
        return w, h

    def draw_cursors(self, ctx):
        """
        Overriding a Canvas method that is called after painting is completed.
        Draws an XOR play cursor over the pattern view.

        @param dc: wx device context.
        @type dc: wx.PaintDC
        """
        if not self.is_visible():
            return
        player = components.get('neil.core.player')
        width, height = self.get_client_size()
        ctx.set_source_rgba(1, 0, 0, 0.7)
        ctx.set_line_width(2)
        sequencer = player.get_current_sequencer()
        track_count = sequencer.get_sequence_track_count()
        if track_count > 0:
            if self.row >= self.startseqtime and self.track >= self.starttrack:
                if (self.selection_start != None and
                    self.selection_end != None):
                    start_track = min(self.selection_start[0],
                                      self.selection_end[0])
                    start_row = min(self.selection_start[1],
                                    self.selection_end[1])
                    end_track = max(self.selection_start[0],
                                    self.selection_end[0])
                    end_row = max(self.selection_start[1],
                                  self.selection_end[1])
                    x1, y1 = self.track_row_to_pos((start_track, start_row))
                    x2, y2 = self.track_row_to_pos((end_track + 1,
                                                    end_row + self.step))
                    cursor_x, cursor_y = x1, y1
                    cursor_width, cursor_height = x2 - x1, y2 - y1
                else:
                    cursor_x, cursor_y = self.track_row_to_pos((self.track,
                                                                self.row))
                    cursor_width = self.seq_row_size
                    cursor_height = self.seq_track_size
                ctx.rectangle(cursor_x + 0.5, cursor_y + 0.5, cursor_width, cursor_height)
                ctx.set_source_rgba(1.0, 0.0, 0.0, 1.0)
                ctx.set_line_width(1)
                ctx.stroke_preserve()
                ctx.set_source_rgba(1.0, 0.0, 0.0, 0.3)
                ctx.fill()

    def draw_playpos(self, ctx):
        if not self.is_visible():
            return

        width, height = self.get_client_size()
        if self.playpos >= self.startseqtime:
            ctx.set_source_rgba(1, 1, 1, 0.5)
            ctx.set_operator(cairo.OPERATOR_XOR)
            x = self.seq_left_margin + int((float(self.playpos - self.startseqtime) / self.step) * self.seq_row_size) + 1
            ctx.set_line_width(1)
            ctx.move_to(x, 1)
            ctx.line_to(x, height-1)
            ctx.stroke()
            ctx.set_operator(cairo.OPERATOR_OVER)


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

    def get_bounds(self):
        width, height = self.get_client_size()
        start = self.startseqtime
        width_in_bars = (width / self.seq_row_size) * self.step
        end = start + width_in_bars
        return (start, end)

    class memoize:
        def __init__(self, function):
            self.function = function
            self.memoized = {}

        def __call__(self, *args):
            try:
                return self.memoized[args]
            except KeyError:
                self.memoized[args] = self.function(*args)
                return self.memoized[args]


    def draw_markers(self, ctx, pango_layout, colors):
        """
        Draw the vertical lines every few bars.
        """
        width, height = self.get_client_size()
        x, y = self.seq_left_margin, self.seq_top_margin

        start = self.startseqtime
        while (x < width):
            if start % (4 * self.step) == 0:
                ctx.set_source_rgb(*colors['Strong Line'])
                ctx.move_to(x, 0)
                ctx.line_to(x, height)
                ctx.stroke()
                ctx.set_source_rgb(*colors['Text'])
                pango_layout.set_markup("<small>%s</small>" % str(start))
                px, py = pango_layout.get_pixel_size()
                ctx.move_to(x + 2, self.seq_top_margin / 2 - py / 2)
                PangoCairo.show_layout(ctx, pango_layout)
            else:
                ctx.set_source_rgb(*colors['Weak Line'])
                ctx.move_to(x, self.seq_top_margin)
                ctx.line_to(x, height)
                ctx.stroke()
            x += self.seq_row_size
            start += self.step
        ctx.set_source_rgb(*colors['Border'])
        ctx.move_to(0, y)
        ctx.line_to(width, y)
        ctx.move_to(self.seq_left_margin, 0)
        ctx.line_to(self.seq_left_margin, height)
        ctx.stroke()
        ctx.set_source_rgb(*colors['Track Background'])
        ctx.rectangle(0, 0, self.seq_left_margin, height)
        ctx.fill()

    def draw_tracks(self, ctx, pango_layout, colors):
        """
        Draw tracks and pattern boxes.
        """
        player = components.get('neil.core.player')
        width, height = self.get_client_size()
        x, y = self.seq_left_margin, self.seq_top_margin
        pango_layout.set_font_description(Pango.FontDescription("sans 8"))
        start = time.time()

        sequencer = player.get_current_sequencer()
        tracks = sequencer.get_track_list()
        
        for track_index in range(self.starttrack, len(tracks)):
            track = tracks[track_index]
            plugin = track.get_plugin()
            plugin_info = self.plugin_info.get(plugin)
            # Draw the pattern boxes
            event_list = list(track.get_event_list())

            for (position, value), index in zip(event_list, range(len(event_list))):
                pattern = None
                if value >= 0x10:
                    pattern = plugin.get_pattern(value - 0x10)
                    length = pattern.get_row_count()
                    end = position + length
                    width_in_bars = (width / self.seq_row_size) * self.step
                    if ((end >= self.startseqtime) and
                        (position < self.startseqtime + width_in_bars)):
                        if value in plugin_info.patterngfx:
                            gfx = plugin_info.patterngfx[value]
                        else:
                            name = prepstr(pattern.get_name())
                            # Handle the case where the pattern overlaps with the next one.
                            # This is done by shortening the current pattern so they display nice.
                            try:
                                if position + length > event_list[index + 1][0]:
                                    length -= position + length - event_list[index + 1][0]
                            except IndexError:
                                pass
                            box_size = max(int(((self.seq_row_size * length) / self.step) + 0.5), 4)
                            gfx_w, gfx_h = box_size - 2, self.seq_track_size - 2
                            gfx = cairo.ImageSurface(cairo.Format.ARGB32, gfx_w, gfx_h)
                            gfxctx = cairo.Context(gfx)

                            pattern_color = get_random_color(plugin.get_name() + name)
                            gfxctx.set_source_rgb(*pattern_color)
                            gfxctx.rectangle(0, 0, gfx_w, gfx_h)
                            gfxctx.fill()

                            pango_layout.set_markup("<small>%s</small>" % name)
                            px, py = pango_layout.get_pixel_size()
                            gfxctx.set_source_rgb(*colors['Text'])
                            gfxctx.move_to(2, 2)
                            PangoCairo.show_layout(gfxctx, pango_layout)
                            plugin_info.patterngfx[value] = gfx
                        x = self.seq_left_margin + ((position - self.startseqtime) * self.seq_row_size / self.step)
                        ctx.set_source_surface(gfx, int(x+1), int(y+1))
                        ctx.paint()
                    if pattern != None:
                        pattern.destroy()
                elif value == 0x00 or value == 0x01:
                    x = (self.seq_left_margin +
                         ((position - self.startseqtime) * self.seq_row_size) /
                         self.step)
                    if value == 0x00:
                        ctx.set_source_rgb(1, 0, 0)
                    else:
                        ctx.set_source_rgb(0, 1, 0)
                    ctx.set_line_width(3)
                    ctx.move_to(x, y + 2)
                    ctx.line_to(x, y + self.seq_track_size - 1)
                    ctx.stroke()
                    ctx.set_line_width(1)
                else:
                    print(("Weird pattern id value: ", value))

                    
            # Draw the track name boxes.
            name = plugin.get_name()
            title = prepstr(name)
            if (player.solo_plugin and
                player.solo_plugin != plugin and
                is_instrument(plugin)):
                title = "[%s]" % title
            elif self.plugin_info[plugin].muted or self.plugin_info[plugin].bypassed:
                title = "(%s)" % title

            # Draw a box that states the name of the machine on that track.
            if self.plugin_info[plugin].muted or self.plugin_info[plugin].bypassed:
                name = get_plugin_color_name(plugin, "Bg Mute")
            else:
                name = get_plugin_color_name(plugin, "Bg")

            ctx.set_source_rgb(*colors[name])


            ctx.rectangle(0, y, self.seq_left_margin, self.seq_track_size)
            ctx.fill()
            ctx.set_source_rgb(*colors['Border'])
            ctx.rectangle(0, y, self.seq_left_margin, self.seq_track_size)
            ctx.stroke()
            pango_layout.set_markup("%s" % title)
            px, py = pango_layout.get_pixel_size()
            # Draw the label with the track name
            ctx.move_to(self.seq_left_margin - 4 - px, y + self.seq_track_size / 2 - py / 2)
            PangoCairo.show_layout(ctx, pango_layout)
            y += self.seq_track_size
            # Draw the horizontal lines separating tracks
            ctx.set_source_rgb(*colors['Weak Line'])
            ctx.move_to(self.seq_left_margin + 1, y)
            ctx.line_to(width - 1, y)
            ctx.stroke()

        # ctx.rectangle(self.seq_left_margin, 0, 5, height)
        # ctx.set_source_rgba(0.0, 0.0, 0.0, 0.15)
        # ctx.fill()

    def draw_loop_points(self, ctx, colors):
        player = components.get('neil.core.player')
        width, height = self.get_client_size()
        ctx.set_line_width(1)
        loop_start, loop_end = player.get_loop()
        window_start, window_end = self.get_bounds()
        if (loop_start >= window_start and loop_start <= window_end):
            # The right facing loop delimiter line with arrow.
            x, y = self.track_row_to_pos((0, loop_start))
            ctx.set_source_rgb(*colors['Loop Line'])
            ctx.move_to(x, 0)
            ctx.line_to(x, height)
            ctx.stroke()
            ctx.move_to(x, 0)
            ctx.line_to(x+10, 0)
            ctx.line_to(x, 10)
            ctx.close_path()
            ctx.fill()
        if (loop_end >= window_start and loop_end <= window_end):
            # The left facing loop delimiter with arrow.
            x, y = self.track_row_to_pos((0, loop_end))
            ctx.set_source_rgb(*colors['Loop Line'])
            ctx.move_to(x, 0)
            ctx.line_to(x, height)
            ctx.stroke()
            ctx.move_to(x, 0)
            ctx.line_to(x-10, 0)
            ctx.line_to(x, 10)
            ctx.close_path()
            ctx.fill()
        # Draw song end marker.
        ctx.set_line_width(3)
        song_end = player.get_song_end()
        if (song_end > window_start and song_end < window_end):
            x, y = self.track_row_to_pos((0, song_end))
            ctx.set_source_rgb(*colors['End Marker'])
            ctx.move_to(x, 0)
            ctx.line_to(x, height)
            ctx.stroke()
        ctx.set_line_width(1)

    def on_draw(self, widget, ctx):
        if (self.needfocus):
            self.grab_focus()
            self.needfocus = False
        self.adjust_scrollbars()
        self.draw(ctx)
        return False

    def draw(self, ctx):
        """
        Overriding a L{Canvas} method that paints onto an offscreen buffer.
        Draws the pattern view graphics.
        """
        width, height = self.get_client_size()
        cfg = config.get_config()

        colors = {
            'Background': cfg.get_float_color('SE Background'),
            'Border': cfg.get_float_color('SE Border'),
            'Strong Line': cfg.get_float_color('SE Strong Line'),
            'Weak Line': cfg.get_float_color('SE Weak Line'),
            'Text': cfg.get_float_color('SE Text'),
            'Track Background': cfg.get_float_color('SE Track Background'),
            'Track Foreground': cfg.get_float_color('SE Track Foreground'),
            'Loop Line': cfg.get_float_color('SE Loop Line'),
            'End Marker': cfg.get_float_color('SE End Marker'),
            'Master Bg': cfg.get_float_color('MV Master'),
            'Effect Bg': cfg.get_float_color('MV Effect'),
            'Generator Bg': cfg.get_float_color('MV Generator'),
            'Controller Bg': cfg.get_float_color('MV Controller'),
            'Master Bg Mute': cfg.get_float_color('MV Master Mute'),
            'Effect Bg Mute': cfg.get_float_color('MV Effect Mute'),
            'Generator Bg Mute': cfg.get_float_color('MV Generator Mute'),
            'Controller Bg Mute': cfg.get_float_color('MV Controller Mute')
        }

        player = components.get('neil.core.player')
        self.playpos = player.get_position()
        if not self.pango_ctx:
            self.pango_ctx = self.get_pango_context()
            self.pango_layout = Pango.Layout(self.pango_ctx)
            self.pango_layout.set_width(-1)
        ctx.set_source_rgb(*colors['Background'])
        ctx.rectangle(0, 0, width, height)
        ctx.fill()
        self.draw_markers(ctx, self.pango_layout, colors)
        self.draw_tracks(ctx, self.pango_layout, colors)
        self.draw_loop_points(ctx, colors)
        self.draw_cursors(ctx)
        self.draw_playpos(ctx)
