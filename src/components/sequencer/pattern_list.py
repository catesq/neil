
import gi
from gi.repository import Gtk
from neil.com import com

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

        self.set_can_focus(True)
        self.set_rubber_banding(True)
        self.get_selection().set_mode(Gtk.SelectionMode.MULTIPLE)


    def on_mouse_over(self, widget, event):
        widget.grab_focus()

    def on_visit_pattern(self, treeview, treeiter, path):
        self.sequencer_panel.jump_to_pattern(treeiter[0] - 2)

    def on_pattern_list_button(self, treeview, event):
        def on_create(item, _):
            self.sequencer_panel.treeview_create_pattern(self)

        def on_clone(item, pattern):
            self.sequencer_panel.treeview_clone_pattern(self, pattern)

        def on_rename(item, pattern):
            self.sequencer_panel.treeview_rename_pattern(self, pattern)
            
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
