import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk
from neil.com import com


# a couple of helper function for the context menu

# clear one of the squencer patterns
def clear_pattern(plugin, pattern_index):
    length = plugin.get_pattern_length(pattern_index)
    name = plugin.get_pattern_name(pattern_index)
    new_pattern = plugin.create_pattern(length)
    new_pattern.set_name(name)
    plugin.update_pattern(pattern_index, new_pattern)




# these are actions for the context menu of the pattern list when a single pattern is selected 
def on_create(item, treeview, _):
    treeview.sequencer_panel.treeview_create_pattern(treeview)
    treeview.update_list()


def on_clone(item, treeview, pattern_index):
    treeview.sequencer_panel.treeview_clone_pattern(treeview, pattern_index)
    treeview.update_list()


def on_rename(item, treeview, pattern_index):
    treeview.sequencer_panel.treeview_rename_pattern(treeview, pattern_index)
    treeview.update_list()


def on_clear(item, treeview, pattern_index):
    plugin = treeview.sequencer_panel.get_active_plugin()
    clear_pattern(plugin, pattern_index)

    player = com.get('neil.core.player')
    player.history_commit("clear pattern")



def on_delete(item, treeview, pattern_index):
    if pattern_index >= 0:
        treeview.sequencer_panel.get_active_plugin().remove_pattern(pattern_index)
        player = com.get('neil.core.player')
        player.history_commit("remove pattern")
        treeview.update_list()


# these are the actions for when a group of items are selected
def on_clear_list(item, treeview, pattern_indexes):
    plugin = treeview.sequencer_panel.get_active_plugin()
    for index in pattern_indexes:
        clear_pattern(plugin, index)

    player = com.get('neil.core.player')
    player.history_commit("clear patterns")


def on_clone_list(item, treeview, pattern_indexes):
    for pattern_index in pattern_indexes:
        treeview.sequencer_panel.treeview_clone_pattern(treeview, pattern_index)
    treeview.update_list()


def on_delete_list(item, treeview, pattern_indexes):
    for pattern_index in pattern_indexes:
        treeview.sequencer_panel.get_active_plugin().remove_pattern(pattern_index)

    player = com.get('neil.core.player')
    player.history_commit("remove patterns")
    treeview.update_list()




# the pattern list. embedded on left of the sequencer panel defined in panel.py 
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


    def build_menu_for_multiple_rows(self, menu, selected_rows):
        menu_entries = [
            ("Clone patterns", on_clone_list, selected_rows),
            ("Clear patterns", on_clear_list, selected_rows),
            ("Delete patterns", on_delete_list, selected_rows),
        ]

        for label, callback, callback_data in menu_entries:
            self.add_menu_item(menu, label, callback, callback_data)


    def build_menu_for_single_row(self, menu, selected_index):
        # the "-2" is because the first two items in the pattern list are not real patterns, ie not found with plugin.get_pattern(selected_index)
        # they are the pattern list entries for auto generated mute and silence patterns 
        menu_entries = [
            ("Clone pattern", on_clone, selected_index - 2),
            ("Pattern properties", on_rename, selected_index - 2),
            ("Clear pattern", on_clear, selected_index - 2),
            ("Delete pattern", on_delete, selected_index - 2),
        ]

        for label, callback, callback_data in menu_entries:
            self.add_menu_item(menu, label, callback, callback_data)


    # get the selected rows in the pattern list
    #   but if they're selected then remove the first two rows, 
    #   they are not real patterns, they're the mute and silence patterns which can't be cleared, deleted or copied.
    def get_viable_selected_rows(self):
        (model, paths) = self.get_selection().get_selected_rows()
        return [path[0] - 2 for path in paths if path[0] > 1]

    def update_list(self):
        self.sequencer_panel.update_list()

    def on_pattern_list_button(self, treeview, event):
        # only show context menu on right clicks when an plugin in the sequencer panel is active is highlighted
        if not self.sequencer_panel.get_active_plugin() or not event.button == 3:
            return False

        menu = Gtk.Menu()

        # show different menu depending on whether any actual rows in the pattern list are selected
        click_path = self.get_path_at_pos(int(event.x), int(event.y))
        selected_rows = self.get_viable_selected_rows()

        self.add_menu_item(menu, "New pattern", on_create, None)

        # want the row highlight to appear when right clicking items
        suppress_event = True

        # the ordering of these tests is deliberately odd
        # the handling was for len(selected_rows) > 1 was originally the second if statement  
        # but this meant: if row 2 was highlighted but row 4 was right clicked then the menu would popup over row 2. the popup menu over row 4 felt better
        # so rearranged the tests so right click over a row happens earlier
        if len(selected_rows) > 1:
            self.build_menu_for_multiple_rows(menu, selected_rows)
        elif click_path != None and click_path[0][0] >= 2:
            self.build_menu_for_single_row(menu, click_path[0][0])
            suppress_event = False
        elif len(selected_rows) == 1:
            self.build_menu_for_single_row(menu, selected_rows[0])


        menu.popup(None, None, None, None, event.button, event.time)

        return suppress_event
        
