import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

from neil.utils import prepstr, ui

from .utils import router_sizes

class AttributesDialog(Gtk.Dialog):
    """
    Displays plugin atttributes and allows to edit them.
    """
    __neil__ = dict(
        id = 'neil.core.attributesdialog',
        singleton = False,
        categories = [
        ]
    )

    def __init__(self, plugin, parent):
        """
        Initializer.

        @param plugin: Plugin object.
        @type plugin: wx.Plugin
        """
        Gtk.Dialog.__init__(self,
                "Attributes",
                parent.get_toplevel(),
                Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT,
                None
        )
        vbox = Gtk.VBox(False, router_sizes.get('margin'))
        vbox.set_border_width(router_sizes.get('margin'))
        self.plugin = plugin
        self.pluginloader = plugin.get_pluginloader()
        self.resize(300, 200)
        self.attriblist, self.attribstore, columns = ui.new_listview([
                ('Attribute', str),
                ('Value', str),
                ('Min', str),
                ('Max', str),
                ('Default', str),
        ])
        vbox.add(ui.add_scrollbars(self.attriblist))
        hsizer = Gtk.HButtonBox()
        hsizer.set_spacing(router_sizes.get('margin'))
        hsizer.set_layout(Gtk.ButtonBoxStyle.START)
        self.edvalue = Gtk.Entry()
        self.edvalue.set_size_request(50, -1)
        self.btnset = Gtk.Button("_Set")
        self.btnok = self.add_button(Gtk.STOCK_OK, Gtk.ResponseType.OK)
        self.btncancel = self.add_button(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL)
        hsizer.pack_start(self.edvalue, False, True, 0)
        hsizer.pack_start(self.btnset, False, True, 0)
        vbox.pack_start(hsizer, False, True, 0)
        self.attribs = []
        for i in range(self.pluginloader.get_attribute_count()):
            attrib = self.pluginloader.get_attribute(i)
            self.attribs.append(self.plugin.get_attribute_value(i))
            self.attribstore.append([
                    prepstr(attrib.get_name()),
                    "%i" % self.plugin.get_attribute_value(i),
                    "%i" % attrib.get_value_min(),
                    "%i" % attrib.get_value_max(),
                    "%i" % attrib.get_value_default(),
            ])
        self.btnset.connect('clicked', self.on_set)
        self.connect('response', self.on_ok)
        self.attriblist.get_selection().connect('changed', self.on_attrib_item_focused)
        if self.attribs:
            self.attriblist.grab_focus()
            self.attriblist.get_selection().select_path((0,))
        self.get_content_area().add(vbox)
        self.show_all()

    def get_focused_item(self):
        """
        Returns the currently focused attribute index.

        @return: Index of the attribute currently selected.
        @rtype: int
        """
        store, rows = self.attriblist.get_selection().get_selected_rows()
        return rows[0][0] if rows and rows[0] else None

    def on_attrib_item_focused(self, selection):
        """
        Called when an attribute item is being focused.

        @param event: Event.
        @type event: wx.Event
        """
        v = self.attribs[self.get_focused_item()]
        self.edvalue.set_text("%i" % v)

    def on_set(self, widget):
        """
        Called when the "set" button is being pressed.
        """
        i = self.get_focused_item()
        attrib = self.pluginloader.get_attribute(i)
        try:
            v = int(self.edvalue.get_text())
            assert v >= attrib.get_value_min()
            assert v <= attrib.get_value_max()
        except:
            ui.error(self, "<b><big>The number you entered is invalid.</big></b>\n\nThe number must be in the proper range.")
            return
        self.attribs[i] = v
        iter = self.attribstore.get_iter((i,))
        self.attribstore.set_value(iter, 1, "%i" % v)

    def on_ok(self, widget, response):
        """
        Called when the "ok" or "cancel" button is being pressed.
        """
        if response == Gtk.ResponseType.OK:
            for i in range(len(self.attribs)):
                self.plugin.set_attribute_value(i, self.attribs[i])