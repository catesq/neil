import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

from neil import components
from neil.presetbrowser import PresetView

class PresetDialog(Gtk.Dialog):
    """
    Displays parameter sliders for a plugin in a new Dialog.
    """
    def __init__(self, manager, plugin, parent):
        Gtk.Dialog.__init__(self, parent=components.get('neil.core.window.root'))
        self.plugin = plugin
        self.manager = manager
        self.manager.preset_dialogs[plugin] = self
        self.view = parent
        self.plugin = plugin
        self.presetview = PresetView(self, plugin, self)
        self.set_title(self.presetview.get_title())
        self.get_content_area().add(self.presetview)
        self.connect('realize', self.on_realize)
        eventbus = components.get('neil.core.eventbus')
        eventbus.zzub_delete_plugin += self.on_zzub_delete_plugin

    def on_zzub_delete_plugin(self, plugin):
        if plugin == self.plugin:
            self.destroy()

    def on_destroy(self, event):
        """
        Handles destroy events.
        """
        del self.manager.preset_dialogs[self.plugin]

    def on_realize(self, widget):
        self.set_default_size(200, 400)


class PresetDialogManager:
    """
    Manages the different preset dialogs.
    """
    __neil__ = dict(
            id = 'neil.core.presetdialog.manager',
            singleton = True,
            categories = [
            ]
    )

    def __init__(self):
        self.preset_dialogs = {}

    def show(self, plugin, parent):
        """
        Shows a preset dialog for a plugin.

        @param plugin: Plugin instance.
        @type plugin: Plugin
        """
        dlg = self.preset_dialogs.get(plugin, None)
        if not dlg:
            dlg = PresetDialog(self, plugin, parent)
        dlg.show_all()