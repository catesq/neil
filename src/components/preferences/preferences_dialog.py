import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

from neil.main import components
from neil.common import MARGIN
from neil.utils import ui

def key_prefpanel(a):
    return (hasattr(a, '__prefpanel__') and a.__prefpanel__.get('label','')) or ''


class PreferencesDialog(Gtk.Dialog):
    """
    This Dialog aggregates the different panels and allows
    the user to switch between them using a tab control.
    """

    __neil__ = dict(
        id = 'neil.core.prefdialog',
        categories = [
        ]
    )

    def __init__(self, parent=None, visible_panel=None):
        Gtk.Dialog.__init__(
            self,
            "Preferences",
            parent,
            Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT
        )

        self.set_default_size(800, 600)
        self.set_size_request(800, 600)
        self.nb = Gtk.Notebook()
        self.nb.set_show_tabs(False)
        self.nb.set_border_width(MARGIN)
        self.nb.set_show_border(False)
        self.panels = sorted(components.get_from_category('neil.prefpanel'), key=key_prefpanel)

        starting_tab_index = 0
        for i, panel in enumerate(self.panels):
            if not hasattr(panel, '__prefpanel__'):
                continue
            cfg = panel.__prefpanel__
            label = cfg.get('label',None)
            if not label:
                continue
            if visible_panel == panel.__neil__['id']:
                starting_tab_index = i
            self.nb.append_page(panel, Gtk.Label(label=label))

        self.tab_list, self.tab_list_store, columns = ui.new_listview([('Name', str),])
        self.tab_list.set_headers_visible(False)
        self.tab_list.set_size_request(200, 600)

        # iterate through all tabs and add to tab list
        for i in range(self.nb.get_n_pages()):
            tab_label = self.nb.get_tab_label(self.nb.get_nth_page(i)).get_label()
            self.tab_list_store.append([tab_label])

        self.tab_list.connect('cursor-changed', self.on_tab_list_change)
        self.splitter = Gtk.HPaned()
        self.splitter.pack1(ui.add_scrollbars(self.tab_list))
        self.splitter.pack2(self.nb)
        self.vbox.add(self.splitter)

        btnok = self.add_button(Gtk.STOCK_OK, Gtk.ResponseType.OK)
        self.add_button(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL)
        self.add_button(Gtk.STOCK_APPLY, Gtk.ResponseType.APPLY)
        btnok.grab_default()

        self.connect('response', self.on_response)
        self.show_all()
#        print(starting_tab_index)
        # select starting tab and adjust the list index
        self.nb.set_current_page(starting_tab_index)

    def on_response(self, widget, response):
        if response == Gtk.ResponseType.OK:
            self.on_ok()
        elif response == Gtk.ResponseType.APPLY:
            self.on_apply()
        else:
            self.destroy()

    def on_tab_list_change(self, treeview):
        self.nb.set_current_page(treeview.get_cursor()[0][0])

    def apply(self):
        """
        Apply changes in settings without closing the dialog.
        """
        for panel in self.panels:
            panel.apply()

    def on_apply(self):
        """
        Event handler for apply button.
        """
        try:
            self.apply()
        except components.exception('neil.exception.cancel'):
            pass

    def on_ok(self):
        """
        Event handler for OK button. Calls apply
        and then closes the dialog.
        """
        try:
            self.apply()
            self.destroy()
        except components.exception('neil.exception.cancel'):
            pass
