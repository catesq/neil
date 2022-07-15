import gi
from gi.repository import Gtk

from neil.utils import add_scrollbars

from .view import SequencerView
from .toolbar import SequencerToolBar
from neil.common import MARGIN, MARGIN0
import config
from neil.com import com
from .utils import Seq

class SequencerPanel(Gtk.VBox):
    """
    Sequencer pattern panel.

    Displays all the patterns available for the current track.
    """
    __neil__ = dict(
        id = 'neil.core.sequencerpanel',
        singleton = True,
        categories = [
            'neil.viewpanel',
            'view',
        ]
    )

    __view__ = dict(
        label = "Sequencer",
        stockid = "neil_sequencer",
        shortcut = 'F4',
        order = 4,
    )

    def __init__(self):
        """
        Initialization.
        """
        Gtk.VBox.__init__(self)
        self.splitter = Gtk.HPaned()
        self.seqliststore = Gtk.ListStore(str, str)

        self.seqpatternlist = Gtk.TreeView(self.seqliststore)
        self.seqpatternlist.set_rules_hint(True)
        self.seqpatternlist.connect("button-press-event", self.on_pattern_list_button)
        self.seqpatternlist.connect("enter-notify-event", self.on_mouse_over)
        self.seqpatternlist.connect("row-activated", self.on_visit_pattern)

        tvkey = Gtk.TreeViewColumn("Key")
        tvkey.set_resizable(True)
        tvpname = Gtk.TreeViewColumn("Pattern Name")
        tvpname.set_resizable(True)
        cellkey = Gtk.CellRendererText()
        cellpname = Gtk.CellRendererText()
        tvkey.pack_start(cellkey, True)
        tvpname.pack_start(cellpname, True)
        tvkey.add_attribute(cellkey, 'text', 0)
        tvpname.add_attribute(cellpname, 'text', 1)
        self.seqpatternlist.append_column(tvkey)
        self.seqpatternlist.append_column(tvpname)
        self.seqpatternlist.set_search_column(0)
        tvkey.set_sort_column_id(0)
        tvpname.set_sort_column_id(1)

        vscroll = Gtk.VScrollbar()
        hscroll = Gtk.HScrollbar()

        self.seqview = SequencerView(self, hscroll, vscroll)
        self.seqview.connect("enter-notify-event", self.on_mouse_over)
        self.viewport = Gtk.Viewport()
        self.viewport.add(self.seqview)
        scrollwin = Gtk.Table(2, 2)
        scrollwin.attach(self.viewport, 0, 1, 0, 1,
                         Gtk.AttachOptions.FILL | Gtk.AttachOptions.EXPAND,
                         Gtk.AttachOptions.FILL | Gtk.AttachOptions.EXPAND)
        scrollwin.attach(vscroll, 1, 2, 0, 1, 0, Gtk.AttachOptions.FILL)
        scrollwin.attach(hscroll, 0, 1, 1, 2, Gtk.AttachOptions.FILL, 0)

        self.splitter.pack1(add_scrollbars(self.seqpatternlist), False, False)
        self.splitter.pack2(scrollwin, True, True)
        self.view = self.seqview
        self.toolbar = SequencerToolBar(self.seqview)

        self.statusbar = Gtk.HBox(False, MARGIN)
        self.statusbar.set_border_width(MARGIN0)

        self.pack_start(self.toolbar, False, True, 0)
        self.pack_start(self.splitter, True, True, 0)
        self.pack_end(self.statusbar, False, True, 0)

        self.__set_properties()
        self.__do_layout()
        # end wxGlade
        self.update_list()
        self.toolbar.update_all()
        self.seqview.connect('size-allocate', self.on_sash_pos_changed)
        self.seqview.grab_focus()
        eventbus = com.get('neil.core.eventbus')
        eventbus.edit_sequence_request += self.edit_sequence_request

    def on_visit_pattern(self, treeview, treeiter, path):
        pattern = treeiter[0] - 2
        if pattern < 0:
            return
        else:
            self.seqview.jump_to_pattern(self.plugin, pattern)

    def on_pattern_list_button(self, treeview, event):
        def on_create(item):
            from patterns import show_pattern_dialog
            from patterns import DLGMODE_NEW
            from neil.utils import get_new_pattern_name
            result = show_pattern_dialog(treeview,
                                         get_new_pattern_name(self.plugin),
                                         self.seqview.step, DLGMODE_NEW, False)
            if result == None:
                return
            else:
                name, length, switch = result
                plugin = self.plugin
                pattern = plugin.create_pattern(length)
                pattern.set_name(name)
                plugin.add_pattern(pattern)
                player = com.get('neil.core.player')
                player.history_commit("new pattern")

        def on_clone(item, pattern):
            from patterns import show_pattern_dialog
            from patterns import DLGMODE_COPY
            from neil.utils import get_new_pattern_name
            result = show_pattern_dialog(treeview,
                                         get_new_pattern_name(self.plugin),
                                         self.seqview.step, DLGMODE_COPY, False)
            if result == None:
                return
            else:
                name, patternsize, switch = result
                m = self.plugin
                p = m.get_pattern(pattern)
                p.set_name(name)
                m.add_pattern(p)
                player = com.get('neil.core.player')
                player.history_commit("clone pattern")

        def on_rename(item, pattern):
            from patterns import show_pattern_dialog
            from patterns import DLGMODE_CHANGE
            from neil.utils import get_new_pattern_name
            result = show_pattern_dialog(treeview,
                                         self.plugin.get_pattern_name(pattern),
                                         self.plugin.get_pattern_length(pattern),
                                         DLGMODE_CHANGE, False)
            if result == None:
                return
            else:
                name, length, switch = result
                plugin = self.plugin
                plugin.set_pattern_name(pattern, name)
                plugin.set_pattern_length(pattern, length)
                player = com.get('neil.core.player')
                player.history_commit("change pattern properties")
            self.view.redraw()

        def on_clear(item, pattern):
            plugin = self.plugin
            length = plugin.get_pattern_length(pattern)
            name = plugin.get_pattern_name(pattern)
            new_pattern = plugin.create_pattern(length)
            new_pattern.set_name(name)
            plugin.update_pattern(pattern, new_pattern)
            player = com.get('neil.core.player')
            player.history_commit("clear pattern")

        def on_delete(item, pattern):
            plugin = self.plugin
            if pattern >= 0:
                plugin.remove_pattern(pattern)
                player = com.get('neil.core.player')
                player.history_commit("remove pattern")

        if event.button == 3:
            x = int(event.x)
            y = int(event.y)
            path = treeview.get_path_at_pos(x, y)
            menu = Gtk.Menu()
            new = Gtk.MenuItem("New pattern")
            clone = Gtk.MenuItem("Clone pattern")
            rename = Gtk.MenuItem("Pattern properties")
            clear = Gtk.MenuItem("Clear pattern")
            delete = Gtk.MenuItem("Delete pattern")
            menu.append(new)
            new.connect('activate', on_create)
            menu.append(clone)
            menu.append(rename)
            menu.append(clear)
            menu.append(delete)
            if hasattr(self, 'plugin') and self.plugin != None:
                new.show()
            if path != None:
                clone.connect('activate', on_clone, path[0][0] - 2)
                clone.show()
                rename.connect('activate', on_rename, path[0][0] - 2)
                rename.show()
                clear.connect('activate', on_clear, path[0][0] - 2)
                clear.show()
                delete.connect('activate', on_delete, path[0][0] - 2)
                delete.show()
            if hasattr(self, 'plugin') and self.plugin != None:
                menu.popup(None, None, None, None, event.button, event.time)

    def on_mouse_over(self, widget, event):
        widget.grab_focus()

    def edit_sequence_request(self, track=None, row=None):
        framepanel = com.get('neil.core.framepanel')
        framepanel.select_viewpanel(self)
        #TODO: add active_tracks option to allow track, row position change
        #player.active_tracks = [(track, row)]
        #framepanel = com.get('neil.core.framepanel')
        #framepanel.select_viewpanel(self)

    def handle_focus(self):
        self.view.needfocus = True

    def update_all(self):
        """
        Updates everything to reflect changes in the sequencer.
        """
        self.update_list()
        self.toolbar.update_all()
        for k, v in self.view.plugin_info.items():
            v.patterngfx = {}
        self.view.update()
        self.seqview.set_cursor_pos(0, 0)
        self.seqview.adjust_scrollbars()
        self.seqview.redraw()
        self.seqview.adjust_scrollbars()

    def update_list(self):
        """
        Updates the panel to reflect a sequence view change.
        """
        self.seqliststore.clear()
        self.seqliststore.append(['-', 'Mute'])
        self.seqliststore.append([',', 'Break'])
        track = self.seqview.get_track()
        if track:
            for pattern, key in zip(track.get_plugin().get_pattern_list(),
                                    Seq.keys):
                self.seqliststore.append([key, pattern.get_name()])
            self.plugin = track.get_plugin()

    def on_sash_pos_changed(self, widget, *args):
        """
        Sent when the sash position changes.

        @param event: Event.
        @type event: wx.Event
        """
        if not self.splitter.is_visible():
            return
        config.get_config().save_window_pos("SequencerSplitter", self.splitter)

    def __set_properties(self):
        """
        Sets properties during initialization.
        """
        # begin wxGlade: SequencerFrame.__set_properties
        self.statuslabels = []

        label = Gtk.Label()
        vsep = Gtk.VSeparator()
        self.statuslabels.append(label)
        self.statusbar.pack_start(label, False, True, 0)
        self.statusbar.pack_start(vsep, False, True, 0)

        label = Gtk.Label()
        vsep = Gtk.VSeparator()
        self.statuslabels.append(label)
        self.statusbar.pack_start(label, False, True, 0)
        self.statusbar.pack_start(vsep, False, True, 0)
        # end wxGlade

    def __do_layout(self):
        """
        Arranges children components during initialization.
        """
        self.show_all()
        config.get_config().load_window_pos("SequencerSplitter", self.splitter)
        # end wxGlade
