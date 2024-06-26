import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

from .toolbar import PatternToolBar
from .views import PatternView
from .patternstatus import PatternStatus

from neil import components

class PatternPanel(Gtk.VBox):
    """
    Panel containing the pattern toolbar and pattern view.
    """
    __neil__ = dict(
        id = 'neil.core.patternpanel',
        singleton = True,
        categories = [
            'neil.viewpanel',
            'view',
        ]
    )

    __view__ = dict(
        label = "Patterns",
        stockid = "neil_pattern",
        shortcut = 'F2',
        order = 2,
    )

    def __init__(self):
        """
        Initialization.
        """
        Gtk.VBox.__init__(self)
        self.is_focused = False

        vscroll = Gtk.VScrollbar()
        hscroll = Gtk.HScrollbar()
        self.view = PatternView(self, hscroll, vscroll)
        self.viewport = Gtk.Viewport()
        self.viewport.add(self.view)
        self.toolbar = PatternToolBar(self.view)
        self.pack_start(self.toolbar, False, True, 0)
        scrollwin = Gtk.Table(2, 2)
        scrollwin.attach(self.viewport, 0, 1, 0, 1, Gtk.AttachOptions.FILL | Gtk.AttachOptions.EXPAND, Gtk.AttachOptions.FILL | Gtk.AttachOptions.EXPAND)
        scrollwin.attach(vscroll, 1, 2, 0, 1, 0, Gtk.AttachOptions.FILL)
        scrollwin.attach(hscroll, 0, 1, 1, 2, Gtk.AttachOptions.FILL, 0)
        self.pack_start(scrollwin, True, True, 0)

        self.view.grab_focus()
        eventbus = components.get('neil.core.eventbus')
        eventbus.edit_pattern_request += self.on_edit_pattern_request

    def on_edit_pattern_request(self, plugin, index):
        player = components.get('neil.core.player')
        player.active_plugins = [plugin]
        player.active_patterns = [(plugin, index)]
        framepanel = components.get('neil.core.framepanel')
        framepanel.select_viewpanel(self)

    # def has_focus(self):
    #     return self.is_focused
    
    def handle_focus(self):
        player = components.get('neil.core.player')
        # check if active patterns match the pattern view settings
        if not (self.view.plugin, self.view.pattern) in player.active_patterns:
            # if player.active_plugins:
            #     plugin = player.active_plugins[0]
            #     # make sure there is a pattern of selected machine to edit
            #     if plugin.get_pattern_count() > 0:
            #         player.active_patterns = [(plugin, 0)]
            #     else:
            #         player.active_patterns = []
            #     self.view.plugin, self.view.pattern  = plugin, 0
            self.view.init_values()
        try:
            self.view.show_cursor_right()
        except AttributeError:  # no pattern in current machine
            pass

        self.view.handle_focus()
        self.toolbar.handle_focus()
        self.view.needfocus = True
        self.view.redraw()

    def remove_focus(self):
        self.view.remove_focus()
        self.toolbar.remove_focus()

    def update_all(self):
        if self.self.is_focused:
            self.view.update_font()
            self.view.redraw()
