#encoding: latin-1

# Neil
# Modular Sequencer
# Copyright (C) 2006,2007,2008,2009,2010 The Neil Development Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA.

from gi.repository import Gtk, Gdk, GdkPixbuf


from functools import cmp_to_key

#                       is_debug, \
from neil.utils import \
                        is_root, is_generator, is_effect, is_controller, is_other, \
                        prepstr, add_scrollbars, new_liststore, get_adapter_name

from neil import com, common

import zzub

# the plugin search creates checkboxes for:
    # plugin adapter: 'lv2', 'vst2', 'ladspa', 'dssi', 'zzub'
    # machine type  : "generators", "effects", "controllers", "others"

# toggling the checkbox will set/unset a config variable named:
    # config.pluginlistbrowser_show_lv2
    # config.pluginlistbrowser_show_vst2
    # ... other plugin adapters ...
    # config.pluginlistbrowser_show_generators
    # config.pluginlistbrowser_show_effects
    # ... other machine types ...

class SearchPluginsDialog(Gtk.Window):
    __neil__ = dict(
        id = 'neil.core.searchplugins',
        singleton = True,
        categories = [
            'viewdialog',
            'view',
        ]
     )

    __view__ = dict(
        label = "Search Plugins",
        order = 0,
        shortcut = "<Shift>F3",
        toggle = True,
    )

    def __init__(self):
        Gtk.Window.__init__(self)
        self.set_default_size(500, -1)
        self.vbox = Gtk.VBox()
        self.add(self.vbox)
        self.set_title("Search Plugins")
        self.connect('delete-event', self.hide_popup_dialog)

        com.get("neil.core.icons")        # make sure theme icons are loaded
        cfg = com.get('neil.core.config') # get the config object with the 'pluginlistbrowser_show_' config settings

        box_container = Gtk.HBox()
        self.check_containers = [Gtk.VBox() for i in range(2)]

        box_container.pack_start(self.check_containers[0], False, False, 0)
        box_container.pack_end(self.check_containers[1], True, False, 0)
        self.vbox.pack_end(box_container, False, False, 0)

        self.machine_types = ["Generators", "Effects", "Controllers", "Others"]
        self.machine_type_check = dict(zip(self.machine_types, [is_generator, is_effect, is_controller, is_other]))

        labels = ['LV2', 'ladspa', 'dssi', 'zzub', 'VST 2']
        urls = ['@zzub.org/dssidapter/', '@zzub.org/ladspadapter/', '@zzub.org/lv2adapter/', '@zzub.org/vstadapter/', '@zzub.org']

        self.adapter_names = ['lv2', 'vst2', 'zzub', 'ladspa', 'dssi']
        self.adapter_labels = dict(zip(self.adapter_names, labels))
        self.adapter_urls = dict(zip(self.adapter_names, urls))

        # prepare the plugin list
        self.treeview = Gtk.TreeView() # the treeview has the list of matching plugins
        self.searchbox = Gtk.Entry()   # the text entry for the plugin list
        self.searchterms = []          # the search box entry split into words

        # the store is a gtk.liststore and will contain every plugin. it is filtered so only matching plugins are shown in the treeview
        (liststore, column_controls) = new_liststore(self.treeview, [
            ('Icon', GdkPixbuf.Pixbuf),
            ('Name', str, dict(markup=True)),
            (None, object),
            (None, str, dict(markup=True)),
        ])

        self.populate(liststore) # add all the plugins to the liststore

        # build the filter for the plugin list. the list is filtered in self.filter_item()
        self.filter = liststore.filter_new()
        self.treeview.set_model(self.filter)
        self.filter.set_visible_func(self.filter_item, data=None)

        # build checkboxes for all machine types and plugin adapters
        def build_checkbox(container, name):
            checkbox = Gtk.CheckButton(label=name)
            container.add(checkbox)
            checkbox.connect("toggled", self.on_checkbox_changed)
            return checkbox

        self.checkboxes = {}
        for machine_type in self.machine_types:
            self.checkboxes[machine_type] = build_checkbox(self.check_containers[0], machine_type)

        for adapter_name in self.adapter_names:
            self.checkboxes[adapter_name] = build_checkbox(self.check_containers[1], self.adapter_labels[adapter_name])

        # read current config settings to check/uncheck each checkbox
        for plugin_name in self.checkboxes.keys():
            attr_name = 'pluginlistbrowser_show_' + plugin_name.lower()
            config_value = getattr(cfg, attr_name)
            self.checkboxes[plugin_name].set_active(config_value)

        self.treeview.set_headers_visible(False)
        self.treeview.set_rules_hint(True)
        self.treeview.set_tooltip_column(3)
        self.treeview.drag_source_set( Gdk.ModifierType.BUTTON1_MASK | Gdk.ModifierType.BUTTON3_MASK, common.DRAG_TARGETS, Gdk.DragAction.COPY )

        scrollbars = add_scrollbars(self.treeview)
        self.vbox.pack_start(scrollbars, True, True, 0)
        self.vbox.pack_end(self.searchbox, False, False, 0)

        self.searchbox.connect("changed", self.on_entry_changed)
        self.searchbox.set_text(cfg.pluginlistbrowser_search_term)

        self.set_size_request(400, 600)
        self.connect('realize', self.realize)
        self.conn_id = False

    def hide_popup_dialog(self, widget, data):
        self.hide()
        return True

    def realize(self, widget):
        if not self.conn_id:
            self.conn_id = self.treeview.connect('drag_data_get', self.on_treeview_drag_data_get)

    def unrealize(self, widget, data):
        self.hide()
        return True

    def get_icon_name(self, pluginloader):
        uri = pluginloader.get_uri()
        for (adapter_name, url_prefix) in self.adapter_urls.items():
            if uri.startswith(url_prefix):
                return adapter_name

        filename = pluginloader.get_name()
        filename = filename.strip().lower()
        for c in '():[]/,.!"\'$%&\\=?*#~+-<>`@ ':
            filename = filename.replace(c, '_')
        while '__' in filename:
            filename = filename.replace('__','_')
        filename = filename.strip('_')
        return filename

    def on_treeview_drag_data_get(self, widget, context, selection_data, info, time):
        target = context.list_targets()[0]
        if target.name() == 'application/x-neil-plugin-uri':
            store, it = self.treeview.get_selection().get_selected()
            child = store.get(it, 2)[0]
            uri = child.get_uri()
            selection_data.set(target, 8, bytes(uri.encode("utf-8")))

    def on_entry_changed(self, widget):
        cfg = com.get('neil.core.config')

        text = self.searchbox.get_text()
        cfg.pluginlistbrowser_search_term = text
        terms = [word.strip() for word in text.lower().replace(',', '').split(' ')]

        self.searchterms = terms
        self.filter.refilter()

    def on_checkbox_changed(self, checkbox):
        cfg = com.get('neil.core.config')
        label = checkbox.get_label().replace(' ', '').lower()
        setattr(cfg, 'pluginlistbrowser_show_' + label, checkbox.get_active())
        self.filter.refilter()

    def match_machine_type(self, pluginloader):
        for machine_type in self.machine_types:
            # check_type_func is one of:  is_generator  is_controller  is_effect  is_other
            check_type_func = self.machine_type_check[machine_type]
            is_checkbox_active = self.checkboxes[machine_type].get_active()
            if is_checkbox_active and check_type_func(pluginloader):
                return True
        return False

    def is_active_adapter(self, pluginloader):
        # adapter name one of:  lv2  vst2  zzub  ladspa  dssi
        adapter_name = get_adapter_name(pluginloader)
        if adapter_name in self.adapter_names:
            return self.checkboxes[adapter_name].get_active()
        return False

    #
    def filter_item(self, model, it, data):
        pluginloader = model.get(it, 2)[0]

        if not self.match_machine_type(pluginloader) or not self.is_active_adapter(pluginloader):
            return False

        name = pluginloader.get_name().lower()
        if len(self.searchterms) > 0:
            unmatched = False
            for group in self.searchterms:
                for word in group:
                    if word not in name:
                        unmatched = True
            return not unmatched

        return True

    def populate(self, liststore):
        cfg = com.get('neil.core.config')
        theme = Gtk.IconTheme.get_default()

        def get_type_rating(n):
            uri = n.get_uri()
            for (adapter_name, url_prefix) in self.adapter_urls.items():
                if uri.startswith(url_prefix):
                    return list(self.adapter_urls.values()).index(url_prefix) + 1
            return 0

        def get_icon_rating(n):
            icon = self.get_icon_name(n)
            if icon and theme.has_icon(icon):
                return 0
            return 1

        def get_rating(n):
            if is_generator(n):
                return 0
            elif is_controller(n):
                return 1
            elif is_effect(n):
                return 2
            else:
                return 3

        def cmp(a, b):
            return int(a > b) - int(a < b)

        def cmp_child(a,b):
            c = cmp(get_type_rating(a), get_type_rating(b))
            if c != 0:
                return c
            c = cmp(get_icon_rating(a),get_icon_rating(b))
            if c != 0:
                return c
            c = cmp(get_rating(a),get_rating(b))
            if c != 0:
                return c
            return cmp(a.get_name().lower(),b.get_name().lower())

        def get_type_text(pl):
            if is_generator(pl):
                return '<span color="' + cfg.get_color('MV Generator') + '">Generator</span>'
            elif is_effect(pl):
                return '<span color="' + cfg.get_color('MV Effect') + '">Effect</span>'
            elif is_controller(pl):
                return '<span color="' + cfg.get_color('MV Controller') + '">Controller</span>'
            else:
                return '<span color="' + cfg.get_color('MV Other') + '">Other</span>'

        plugins = {}
        player = com.get('neil.core.player')
        for pluginloader in player.get_pluginloader_list():
            plugins[pluginloader.get_uri()] = pluginloader

        for pl in sorted(plugins.values(), key=cmp_to_key(cmp_child)):
            name = prepstr(pl.get_name())
            text = '<b>' + name + '</b>\n<small>' + get_type_text(pl) + '</small>'
            pixbuf = None
            icon = self.get_icon_name(pl)
            tooltip = '<b>' + name + '</b> <i>(' + get_type_text(pl) + ')</i>\n'
            tooltip += '<small>' + prepstr(pl.get_uri()) + '</small>\n\n'
            gpcount = pl.get_parameter_count(zzub.zzub_parameter_group_global)
            tpcount = pl.get_parameter_count(zzub.zzub_parameter_group_track)
            acount = pl.get_attribute_count()
            tooltip += '%i global parameters, %i track parameters, %i attributes\n\n' % (gpcount, tpcount, acount)
            author = pl.get_author().replace('<', '&lt;').replace('>', '&gt;')
            tooltip += 'Written by   ' + prepstr(author)
            if icon and theme.has_icon(icon):
                pixbuf = theme.load_icon(icon, Gtk.IconSize.MENU, 0)
            liststore.append([pixbuf, text, pl, tooltip])


__all__ = [
    'SearchPluginsDialog',
]

__neil__ = dict(
    classes = [
        SearchPluginsDialog,
    ],
)

if __name__ == '__main__':
    pass
