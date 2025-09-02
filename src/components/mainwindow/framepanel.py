from .helpers import cmp_view
from functools import cmp_to_key

import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

from neil import components, views
from neil.utils import ui


class FramePanel(Gtk.Notebook):
    __neil__ = dict(
        id = 'neil.core.framepanel',
        singleton = True,
        categories = [
        ],
    )


    def __init__(self):
        Gtk.Notebook.__init__(self)
        self.set_tab_pos(Gtk.PositionType.LEFT)
        self.set_show_border(True)
        self.set_border_width(1)
        self.set_show_tabs(True)
        components.get("neil.core.icons") # make sure theme icons are loaded
        defaultpanel = None
        self.statusbar = views.get_statusbar()
        self.pages = sorted(components.get_from_category('neil.viewpanel'), key=cmp_to_key(cmp_view))
        self.set_size_request(100, 100)
        

        for index, panel in enumerate(self.pages):
            if not panel or not hasattr(panel, '__view__'):
                print(("panel", panel, "misses attribute __view__"))
                continue

            options = panel.__view__
            stockid = options['stockid']
            label = options['label']
            key = options.get('shortcut', '')

            if options.get('default'):
                defaultpanel = panel
                
            panel.show_all()

            theme_img = ui.new_theme_image(stockid, Gtk.IconSize.MENU)
            header = Gtk.VBox()
            labelwidget = Gtk.Label(label=label)
            labelwidget.set_angle(90)
            header.pack_start(labelwidget, True, True, 0)
            header.pack_start(theme_img, True, True, 0)
            header.show_all()

            if key:
                header.set_tooltip_text("%s (%s)" % (label, key))
            else:
                header.set_tooltip_text(label)

            self.append_page(panel, header)

        if defaultpanel:
            self.select_viewpanel(defaultpanel)
        else:
            self.select_viewpanel(self.pages[0])

        self.show_all()

    def select_viewpanel(self, panel):
        prev_index = self.get_current_page()

        if prev_index < 0 or prev_index > len(self.pages):
            return

        if self.pages[prev_index] == panel:
            return
        
        try:
            next_index = self.pages.index(panel)
        except ValueError:
            return
        
        self.statusbar.clear_both_sides()

        if hasattr(self.pages[prev_index], 'remove_focus'):
            self.pages[prev_index].remove_focus()

        self.set_current_page(next_index)

        if hasattr(panel, 'handle_focus'):
            panel.handle_focus()