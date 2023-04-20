
import gi
gi.require_version("Gtk", "3.0")
gi.require_version('PangoCairo', '1.0')
from gi.repository import Gtk, Gdk, GLib

from functools import cmp_to_key


from neil import com, errordlg, common

import config


from neil.utils import \
    Menu, new_theme_image, make_submenu_item

from .cmp import cmp_view


class ViewMenu(Menu):
    __neil__ = dict(
        id = 'neil.core.viewmenu',
        singleton = True,
        categories = [],
    )

    def on_check_item(self, menuitem, view):
        if menuitem.get_active():
            view.show_all()
        else:
            view.hide()

    def on_activate_item(self, menuitem, view):
        if 'neil.viewpanel' in view.__neil__.get('categories',[]):
            framepanel = com.get('neil.core.framepanel')             # mainwindow/frame.py
            framepanel.select_viewpanel(view)
        else:
            view.hide()

    def on_activate(self, widget, item, view):
        item.set_active(view.get_property('visible'))
        
        if view.get_property('visible'):
            view.show_all()

    def __init__(self):
        Menu.__init__(self)
        views = sorted(com.get_from_category('view'), key=cmp_to_key(cmp_view))
        com.get("neil.core.icons") # make sure theme icons are loaded
        accel = com.get('neil.core.accelerators')

        for view in views:
            if not hasattr(view, '__view__'):
                print(("view",view,"misses attribute __view__"))
                continue
            options = view.__view__
            label = options['label']
            stockid = options.get('stockid', None)
            shortcut = options.get('shortcut', None)

            if options.get('toggle'):
                item = self.add_check_item(label, False, self.on_check_item, view)
                self.connect('show', self.on_activate, item, view)
            elif stockid:
                theme_image = new_theme_image(stockid, Gtk.IconSize.MENU)
                item = self.add_image_item(label, theme_image, self.on_activate_item, view)
            else:
                item = self.add_item(label, self.on_activate_item)

            if shortcut:
                accel.add_accelerator(shortcut, item)

        if 0:
            # TODO: themes
            neil_frame =  com.get('neil.core.accelerators')
            # main_frame = get_root_window()
            tempsubmenu = Gtk.Menu()
            defaultitem = Gtk.RadioMenuItem(label="Default")
            tempsubmenu.append(defaultitem)
            self.thememenu = tempsubmenu
            cfg = config.get_config()
            if not cfg.get_active_theme():
                defaultitem.set_active(True)
            defaultitem.connect('toggled', neil_frame.on_select_theme, None)
            for name in sorted(cfg.get_theme_names()):
                item = Gtk.RadioMenuItem(label=prepstr(name), group=defaultitem)
                if name == cfg.get_active_theme():
                    item.set_active(True)
                item.connect('toggled', neil_frame.on_select_theme, name)
                tempsubmenu.append(item)
            self.append(make_submenu_item(tempsubmenu, "Themes"))