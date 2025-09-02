import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk
from . import route_view
from neil import components, views

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
    

    view: route_view.RouteView
    scrollview: Gtk.ScrolledWindow


    def __init__(self):
        """
        Initializer.
        """
        Gtk.VBox.__init__(self)

        self.scrollview = Gtk.ScrolledWindow()
        self.scrollview.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)
        
        self.view = views.get_router(self) 
        self.scrollview.add(self.view)

        self.pack_start(self.scrollview, True, True, 0)


    def handle_focus(self):
        self.view.grab_focus()

    # 
    def reset(self):
    #     """
    #     Resets the router view. Used when
    #     a new song is being loaded.
    #     """
        print("***********CALLED RESET***********")
    #     No sign of this actually being used. 

    #     self.view.reset()


    def update_all(self):
        self.view.update_all()
        # self.view.redraw()