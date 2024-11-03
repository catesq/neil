import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

from neil import components

class RoutePanel(Gtk.VBox):
    """
    Contains the view panel and manages parameter dialogs.
    """
    __neil__ = dict(
        id = 'neil.core.routerpanel',
        singleton = True,
        categories = [
            'neil.viewpanel',
            'view',
        ]
    )

    __view__ = dict(
        label = "Router",
        stockid = "neil_router",
        shortcut = 'F3',
        default = True,
        order = 3,
    )


    def __init__(self):
        """
        Initializer.
        """
        Gtk.VBox.__init__(self)
        self.view = components.get('neil.core.router.view', self)
        self.pack_start(self.view, True, True, 0)


    def handle_focus(self):
        self.view.grab_focus()


    def reset(self):
        """
        Resets the router view. Used when
        a new song is being loaded.
        """
        self.view.reset()


    def update_all(self):
        self.view.update_colors()
        self.view.redraw()