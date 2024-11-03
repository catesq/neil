import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

from rack import ParameterView
from neil import components


class ParameterDialog(Gtk.Dialog):
    """
    Displays parameter sliders for a plugin in a new Dialog.
    """
    __neil__ = dict(
        id = 'neil.core.parameterdialog',
        singleton = False,
        categories = [
        ]
    )


    def __init__(self, manager, plugin, parent):
        Gtk.Dialog.__init__(self, parent=parent.get_toplevel())
        self.plugin = plugin
        self.manager = manager
        self.manager.plugin_dialogs[plugin] = self
        self.paramview = ParameterView(plugin)
        self.set_title(self.paramview.get_title())
        self.get_content_area().add(self.paramview)
        self.connect('destroy', self.on_destroy)
        self.connect('realize', self.on_realize)
        eventbus = components.get('neil.core.eventbus')
        eventbus.zzub_delete_plugin += self.on_zzub_delete_plugin


    def on_realize(self, widget):
        self.set_default_size(*self.paramview.get_best_size())


    def on_zzub_delete_plugin(self, plugin):
        if plugin == self.plugin:
            self.destroy()


    def on_destroy(self, event):
        """
        Handles destroy events.
        """
        del self.manager.plugin_dialogs[self.plugin]


class ParameterDialogManager:
    """
    Manages the different parameter dialogs.
    """
    __neil__ = dict(
            id = 'neil.core.parameterdialog.manager',
            singleton = True,
            categories = [
            ]
    )

    def __init__(self):
        self.plugin_dialogs = {}

    def show(self, plugin, parent):
        """
        Shows a parameter dialog for a plugin.

        @param plugin: Plugin instance.
        @type plugin: Plugin
        """
        dlg = self.plugin_dialogs.get(plugin, None)
        if not dlg:
            dlg = ParameterDialog(self, plugin, parent)
        dlg.show_all()