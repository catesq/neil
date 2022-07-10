from gi.repository import Gtk

import zzub
from neil.com import com
from neil.utils import (Menu, is_generator, is_root, is_effect, iconpath, prepstr)
from neil.preset import Preset
from .actions import on_popup_bypass, on_store_selection, on_restore_selection
import os.path
import json

#class Connection:
#    def __init__(self, metaplugin, connection_id):
#        self.metaplugin = metaplugin
#        self.id = connection_id

#used by on_store_selection in actions.py
def store_selection_submenu(metaplugins):
    router = com.get('neil.core.router.view')
    store_submenu = Menu()

    for index in range(10):
        store_submenu.add_check_item(f"_{index}", router.has_selection(index), on_store_selection, index, metaplugins)

    return store_submenu

#used by on_restore_selection in actions.py
def restore_selection_submenu():
    router = com.get('neil.core.router.view')
    restore_submenu = Menu()

    for index in range(10):
        restore_submenu.add_check_item(f"_{index}", router.has_selection(index), on_restore_selection, index)

    return restore_submenu

#used by machine_tree_submenu below
def load_plugin_list(filename):
    with open(filename, "r") as jsonfile:
        return json.load(jsonfile)

    with open(os.path.join(os.path.dirname(__file__), filename)) as jsonfile:
        return json.load(jsonfile)

    return {}


def machine_tree_submenu(connection=False):
    def get_icon_name(pluginloader):
        # uri = pluginloader.get_uri()
        #if uri.startswith('@zzub.org/dssidapter/'):
        #    return iconpath("scalable/dssi.svg")
        #if uri.startswith('@zzub.org/ladspadapter/'):
        #    return iconpath("scalable/ladspa.svg")
        #if uri.startswith('@psycle.sourceforge.net/'):
        #    return iconpath("scalable/psycle.svg")
        filename = pluginloader.get_name()
        filename = filename.strip().lower()
        for c in '():[]/,.!"\'$%&\\=?*#~+-<>`@ ':
            filename = filename.replace(c, '_')
        while '__' in filename:
            filename = filename.replace('__', '_')
        filename = filename.strip('_')
        return "%s.svg" % iconpath("scalable/" + filename)

    def add_path(tree, path, loader):
        if len(path) == 1:
            tree[path[0]] = loader
            return tree
        elif path[0] not in tree:
            tree[path[0]] = add_path({}, path[1:], loader)
            return tree
        else:
            tree[path[0]] = add_path(tree[path[0]], path[1:], loader)
            return tree

    def populate_from_tree(menu, tree):
        for key, value in tree.items():
            if not isinstance(value, dict):
                icon = Gtk.Image()
                filename = get_icon_name(value)
                if os.path.isfile(filename):
                    icon.set_from_file(get_icon_name(value))
                item = Gtk.ImageMenuItem(prepstr(key, fix_underscore=True))
                item.set_image(icon)
                item.connect('activate', create_plugin, value)
                menu.add(item)
            else:
                item, submenu = menu.add_submenu(key)
                populate_from_tree(submenu, value)

    def create_plugin(item, loader):
        player = com.get('neil.core.player')
        if connection:
            player.create_plugin(loader, connection=connection)
        else:
            player.plugin_origin = menu.context
            player.create_plugin(loader)

    player = com.get('neil.core.player')
    plugin_list = load_plugin_list("plugin_tree.json")
    plugins = {}
    tree = {}
    submenu = Menu()

    for pluginloader in player.get_pluginloader_list():
        plugins[pluginloader.get_uri()] = pluginloader
    for uri, loader in plugins.items():
        try:
            path = plugin_list[uri]
            if connection and (path[0] not in ["Effects", "Analyzers"]):
                continue
            path = path + [loader.get_name()]
            tree = add_path(tree, path, loader)
        except KeyError:
            pass

    populate_from_tree(submenu, tree)

    return submenu




