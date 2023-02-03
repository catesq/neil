import gi
from gi.repository import Gtk

from neil.utils import add_scrollbars, get_new_pattern_name

from .view import SequencerView
from .toolbar import SequencerToolBar
from .utils import Seq

from neil.com import com
from neil.common import MARGIN, MARGIN0
import config

from patterns import show_pattern_dialog, DLGMODE_NEW, DLGMODE_COPY, DLGMODE_CHANGE

class SequencerPatternListTreeView(Gtk.TreeView):
    def __init__(self, sequencer_panel, seqliststore):
        Gtk.TreeView.__init__(self, seqliststore)
        self.sequencer_panel = sequencer_panel
        self.seqpatternlist = Gtk.TreeView(seqliststore)
        self.set_rules_hint(True)
        self.connect("button-press-event", self.on_pattern_list_button)
        self.connect("enter-notify-event", self.on_mouse_over)
        self.connect("row-activated", self.on_visit_pattern)

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
        self.append_column(tvkey)
        self.append_column(tvpname)
        self.set_search_column(0)
        tvkey.set_sort_column_id(0)
        tvpname.set_sort_column_id(1)

    def on_mouse_over(self, widget, event):
        widget.grab_focus()

    def on_visit_pattern(self, treeview, treeiter, path):
        self.sequencer_panel.jump_to_pattern(treeiter[0] - 2)

    def on_pattern_list_button(self, treeview, event):
        def on_create(item):
            self.sequencer_panel.treeview_create_pattern(treeview)

        def on_clone(item, pattern):
            self.sequencer_panel.treeview_clone_pattern(treeview, pattern)

        def on_rename(item, pattern):
            self.sequencer_panel.treeview_rename_pattern(treeview, pattern)
            
        def on_clear(item, pattern):
            plugin = self.sequencer_panel.get_active_plugin()
            length = plugin.get_pattern_length(pattern)
            name = plugin.get_pattern_name(pattern)
            new_pattern = plugin.create_pattern(length)
            new_pattern.set_name(name)
            plugin.update_pattern(pattern, new_pattern)
            player = com.get('neil.core.player')
            player.history_commit("clear pattern")

        def on_delete(item, pattern):
            if pattern >= 0:
                self.sequencer_panel.get_active_plugin().remove_pattern(pattern)
                player = com.get('neil.core.player')
                player.history_commit("remove pattern")

        def add_menu_item(menu, label, callback, callback_data):
            item = Gtk.MenuItem(label)
            menu.append(item)
            item.connect('activate', callback, callback_data)
            item.show()

        if event.button == 3:
            if not self.sequencer_panel.get_active_plugin():
                return

            menu = Gtk.Menu()

            add_menu_item(menu, "New pattern", on_create, None)

            path = self.get_path_at_pos(int(event.x), int(event.y))

            if path != None:
                menu_entries = [
                    ("Clone pattern", on_clone, path[0][0] - 2),
                    ("Pattern properties", on_rename, path[0][0] - 2),
                    ("Clear pattern", on_clear, path[0][0] - 2),
                    ("Delete pattern", on_delete, path[0][0] - 2),
                ]

                for label, callback, callback_data in menu_entries:
                    add_menu_item(menu, label, callback, callback_data)
                
            menu.popup(None, None, None, None, event.button, event.time)


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
        self.seqpatternlist = SequencerPatternListTreeView(self, self.seqliststore)

        vscroll = Gtk.VScrollbar()
        hscroll = Gtk.HScrollbar()

        self.plugin = None
        self.seqview = SequencerView(self, hscroll, vscroll)
        self.seqview.connect("enter-notify-event", self.on_mouse_over)
        self.viewport = Gtk.Viewport()
        self.viewport.add(self.seqview)
        scrollwin = Gtk.Table(2, 2)
        scrollwin.attach(
            self.viewport, 
            0, 1, 0, 1,
            Gtk.AttachOptions.FILL | Gtk.AttachOptions.EXPAND,
            Gtk.AttachOptions.FILL | Gtk.AttachOptions.EXPAND
        )
        
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

    def jump_to_patterm(self, pattern_num):
        if pattern_num >= 0:
            self.seqview.jump_to_pattern(pattern_num)

    def get_active_plugin(self):
        return self.plugin

    # called from on_copy in SequencerTreeView
    def treeview_create_pattern(self, treeview):
        result = show_pattern_dialog(
            treeview,
            get_new_pattern_name(self.plugin),
            self.seqview.step, 
            DLGMODE_NEW, 
            False
        )

        if result == None:
            return

        name, length, switch = result
        plugin = self.plugin
        pattern = plugin.create_pattern(length)
        pattern.set_name(name)
        plugin.add_pattern(pattern)
        player = com.get('neil.core.player')
        player.history_commit("new pattern")

    ## called from on)clone in SequencerTreeView
    def treeview_clone_pattern(self, treeview, pattern):
        result = show_pattern_dialog(
            treeview,
            get_new_pattern_name(self.plugin),
            self.seqview.step, 
            DLGMODE_COPY, 
            False
        )

        if result == None:
            return

        name, patternsize, switch = result
        m = self.plugin
        p = m.get_pattern(pattern)
        p.set_name(name)
        m.add_pattern(p)
        player = com.get('neil.core.player')
        player.history_commit("clone pattern")


    def treeview_rename_pattern(self, treeview, pattern):
        result = show_pattern_dialog(
            treeview,
            self.plugin.get_pattern_name(pattern),
            self.plugin.get_pattern_length(pattern),
            DLGMODE_CHANGE, 
            False
        )

        if result == None:
            return

        name, length, switch = result
        
        self.plugin.set_pattern_name(pattern, name)
        self.plugin.set_pattern_length(pattern, length)
        player = com.get('neil.core.player')
        player.history_commit("change pattern properties")
        self.view.redraw()

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
