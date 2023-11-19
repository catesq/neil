import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

from .toolbar import PatternToolBar
from .page import NeilNotebookPage
from .views import PatternView
from .patternstatus import PatternStatus

import neil.common as common
import neil.com as com

class PatternPanel(NeilNotebookPage):
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
        eventbus = com.get('neil.core.eventbus')
        eventbus.edit_pattern_request += self.on_edit_pattern_request

    def on_edit_pattern_request(self, plugin, index):
        player = com.get('neil.core.player')
        player.active_plugins = [plugin]
        player.active_patterns = [(plugin, index)]
        framepanel = com.get('neil.core.framepanel')
        framepanel.select_viewpanel(self)

    def handle_focus(self):
        player = com.get('neil.core.player')
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
        self.view.needfocus = True
        self.view.focused()
        self.view.redraw()

    def update_all(self):
        
        if self.is_current_page():
            self.view.update_font()
            self.view.redraw()
