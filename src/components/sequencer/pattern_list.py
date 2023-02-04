
import gi
from gi.repository import Gtk
from neil.com import com


def on_create(item, treeview, _):
    treeview.sequencer_panel.treeview_create_pattern(treeview)


def on_clone(item, treeview, pattern_index):
    treeview.sequencer_panel.treeview_clone_pattern(treeview, pattern_index)


def on_rename(item, treeview, pattern_index):
    treeview.sequencer_panel.treeview_rename_pattern(treeview, pattern_index)
    

def on_clear(item, treeview, pattern_index):
    plugin = treeview.sequencer_panel.get_active_plugin()
    length = plugin.get_pattern_length(pattern_index)
    name = plugin.get_pattern_name(pattern_index)
    new_pattern = plugin.create_pattern(length)
    new_pattern.set_name(name)
    plugin.update_pattern(pattern_index, new_pattern)
    player = com.get('neil.core.player')
    player.history_commit("clear pattern")


def on_delete(item, treeview, pattern_index):
    if pattern_index >= 0:
        treeview.sequencer_panel.get_active_plugin().remove_pattern(pattern_index)
        player = com.get('neil.core.player')
        player.history_commit("remove pattern")

def on_delete_list(item, treeview, pattern_indexes):
    for pattern_index in pattern_indexes:
        treeview.sequencer_panel.get_active_plugin().remove_pattern(pattern_index)

    player = com.get('neil.core.player')
    player.history_commit("remove patterns")

# the pattern list embedded on left of panel.py 
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
        tvpname = Gtk.TreeViewColumn("Pattern Name")

        tvkey.set_resizable(True)
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

        self.set_can_focus(False)
        self.set_rubber_banding(True)
        self.get_selection().set_mode(Gtk.SelectionMode.MULTIPLE)


    def on_mouse_over(self, widget, event):
        widget.grab_focus()


    def on_visit_pattern(self, treeview, treeiter, path):
        self.sequencer_panel.jump_to_pattern(treeiter[0] - 2)


    def add_menu_item(self, menu, label, callback, callback_data):
        item = Gtk.MenuItem(label)
        menu.append(item)
        item.connect('activate', callback, self, callback_data)
        item.show()

    def build_menu_for_selected_rows(self, menu):
        (model, paths) = self.get_selection().get_selected_rows()
        pattern_nums = [path[0] - 2 for path in paths]

        menu_entries = [
            ("Delete patterns", on_delete_list, pattern_nums),
        ]

        for label, callback, callback_data in menu_entries:
            self.add_menu_item(menu, label, callback, callback_data)


    # when no patterns are selected then
    def build_menu_for_row_at_clickpos(self, menu, path):
        print("clickpos path", path[0][0] - 2)
        menu_entries = [
            ("Clone pattern", on_clone, path[0][0] - 2),
            ("Pattern properties", on_rename, path[0][0] - 2),
            ("Clear pattern", on_clear, path[0][0] - 2),
            ("Delete pattern", on_delete, path[0][0] - 2),
        ]

        for label, callback, callback_data in menu_entries:
            self.add_menu_item(menu, label, callback, callback_data)

    def on_pattern_list_button(self, treeview, event):
        if not self.sequencer_panel.get_active_plugin() or not event.button == 3:
            return False

        menu = Gtk.Menu()

        click_path = self.get_path_at_pos(int(event.x), int(event.y))
        selected_count = self.get_selection().count_selected_rows()

        self.add_menu_item(menu, "New pattern", on_create, None)

        if selected_count > 0:
            self.build_menu_for_selected_rows(menu)
        elif click_path != None:
            self.build_menu_for_row_at_clickpos(menu, click_path)

        menu.popup(None, None, None, None, event.button, event.time)

        return True
