import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

from neil.controller import learn_controller
from neil.utils import ui, sizes

from config import get_config

class ControllerPanel(Gtk.VBox):
    """
    Panel which allows to set up midi controller mappings.
    """


    __neil__ = dict(
        id = 'neil.core.pref.controller',
        categories = [
            'neil.prefpanel',
        ]
    )


    __prefpanel__ = dict(
        label = "Controllers",
    )


    def __init__(self):
        self.sort_column = 0
        Gtk.VBox.__init__(self)

        self.set_border_width(sizes.get('margin'))
        frame1 = Gtk.Frame.new("Controllers")
        sizer1 = Gtk.VBox(expand=False, margin=sizes.get('margin'))
        sizer1.set_border_width(sizes.get('margin'))
        frame1.add(sizer1)

        self.controllers, self.store, columns = ui.new_listview([
                ('Name', str),
                ('Channel', str),
                ('Controller', str),
        ])

        self.controllers.get_selection().set_mode(Gtk.SelectionMode.MULTIPLE)
        sizer1.add(ui.add_scrollbars(self.controllers))

        self.btnadd = Gtk.Button.new_from_stock(Gtk.STOCK_ADD)
        self.btnremove = Gtk.Button.new_from_stock(Gtk.STOCK_REMOVE)
        hsizer = Gtk.HButtonBox(layout_style=Gtk.ButtonBoxStyle.START, spacing=sizes.get('margin'))
        hsizer.pack_start(self.btnadd, False, True, 0)
        hsizer.pack_start(self.btnremove, False, True, 0)
        sizer1.pack_start(hsizer, False, True, 0)
        
        self.add(frame1)
        self.btnadd.connect('clicked', self.on_add_controller)
        self.btnremove.connect('clicked', self.on_remove_controller)
        self.update_controllers()


    def update_controllers(self):
        """
        Updates the controller list.
        """
        self.store.clear()
        for name,channel,ctrlid in get_config().get_midi_controllers():
            self.store.append([name, str(channel), str(ctrlid)])


    def on_add_controller(self, widget):
        """
        Handles 'Add' button click. Opens a popup that records controller events.
        """
        res = learn_controller(self)
        if res:
            name, channel, ctrlid = res
            self.store.append([name, str(channel), str(ctrlid)])


    def on_remove_controller(self, widget):
        """
        Handles 'Remove' button click. Removes the selected controller from list.
        """
        store, sel = self.controllers.get_selection().get_selected_rows()
        refs = [Gtk.TreeRowReference(store, row) for row in sel]

        for ref in refs:
            path = ref.get_path()

            if path is None:
                continue

            self.store.remove(store.get_iter(path))


    def apply(self):
        """
        Validates user input and reinitializes the driver with current
        settings. If the reinitialization fails, the user is being
        informed and asked to change the settings.
        """
        ctrllist = []
        for row in self.store:
            name = row[0]
            channel = int(row[1])
            ctrlid = int(row[2])
            ctrllist.append((name,channel,ctrlid))
        get_config().set_midi_controllers(ctrllist)
