from gi.repository import Gtk
from neil.com import com
from functools import reduce
from neil.utils import gettext, prepstr, filenameify, show_machine_manual, is_root, \
                       clone_plugin, clone_plugin_patterns, clone_preset
from .connector_dialog import ConnectorDialog


def on_popup_mute_selected(widget, plugins):
    com.get('neil.core.player').mute_group(plugins)


def on_popup_mute(widget, plugin):
    com.get('neil.core.player').toggle_mute(plugin)


def on_popup_unmute_all(widget, plugin):
    player = com.get('neil.core.player')
    common = com.get('neil.core.common')

    for plugin in reversed(list(player.get_plugin_list())):
        info = common.get_plugin_infos().get(plugin)
        info.muted = False
        plugin.set_mute(info.muted)
        info.reset_plugingfx()


def on_popup_solo(widget, plugin):
    player = com.get('neil.core.player')

    if player.solo_plugin != plugin:
        player.solo(plugin)
    else:
        player.solo(None)


def on_popup_bypass(widget, plugin):
    com.get('neil.core.player').toggle_bypass(plugin)


def on_popup_show_params(widget, plugin):
    com.get('neil.core.parameterdialog.manager').show(plugin, widget)


def on_popup_show_attribs(widget, plugin):
    dlg = com.get('neil.core.attributesdialog', plugin, widget.get_toplevel())
    dlg.run()
    dlg.destroy()


def on_popup_show_presets(widget, plugin):
    com.get('neil.core.presetdialog.manager').show(plugin, widget.get_toplevel())


def on_popup_rename(widget, plugin):
    text = gettext(widget, "Enter new plugin name:", prepstr(plugin.get_name()))
    if text:
        player = com.get('neil.core.player')
        plugin.set_name(text)
        player.history_commit("rename plugin")


def on_popup_delete(widget, plugin):
    com.get('neil.core.player').delete_plugin(plugin)


def on_popup_delete_group(widget, plugins):
    player = com.get('neil.core.player')
    for plugin in plugins:
        player.delete_plugin(plugin)


def on_popup_set_target(widget, plugin):
    player = com.get('neil.core.player')
    if player.autoconnect_target == plugin:
        player.autoconnect_target = None
    else:
        player.autoconnect_target = plugin


def on_popup_command(widget, plugin, subindex, index):
    plugin.command((subindex << 8) | index)


def on_machine_help(widget, plugin):
    name = filenameify(plugin.get_pluginloader().get_name())
    if not show_machine_manual(name):
        info = Gtk.MessageDialog(widget.get_toplevel(), flags=0, type=Gtk.MessageType.INFO, buttons=Gtk.ButtonsType.OK, message_format="Sorry, there's no help for this plugin yet")
        info.run()
        info.destroy()


def disconnect_plugin(plugin, conn_index):
    conn_plugin = plugin.get_input_connection_plugin(conn_index)
    conn_type = plugin.get_input_connection_type(conn_index)
    plugin.delete_input(conn_plugin, conn_type)





# called from contextmenu.ConnectionMenu 
def on_popup_edit_cv_connector(widget, to_plugin, connection_id, connector_id):
    """
    show a to edit dialog the cv connection between two plugins
    """
    from_plugin = to_plugin.get_input_connection_plugin(connection_id)
    connection = to_plugin.get_input_connection(connection_id).as_cv_connection()


    connector = connection.get_connector(connector_id)
    source, target, data = (connector.get_source(), connector.get_target(), connector.get_data())

    dialog = ConnectorDialog(widget, from_plugin, to_plugin, source, target, data)
    
    res = dialog.run()

    if res == Gtk.ResponseType.OK:
        [source, target] = dialog.get_connectors()
        to_plugin.update_cv_connector(from_plugin, source, target, dialog.get_cv_data(), connector_id)
        com.get_player().history_commit("edit cv connection")

    dialog.destroy()






def on_popup_remove_cv_connector(widget, to_plugin, from_plugin, connector_id):
    to_plugin.remove_cv_connector(to_plugin, from_plugin, connector_id)
    com.get_player().history_commit("remove cv connection")


def on_popup_disconnect(widget, plugin, index):
    disconnect_plugin(plugin, index)
    com.get_player().history_commit("disconnect")




def on_popup_disconnect_all(widget, connections):
    for plugin, index, conntype in connections:
        disconnect_plugin(plugin, index)
        
    com.get_player().history_commit("disconnect")



# all the hard work for this is done in ccmwriter in ccm.cpp and is basically a c++ copy of clone_chain() hooked into the file save
def on_popup_save_chain(widget, plugins):
    chained_plugins_list = get_plugin_chain(plugins)

    for pl in chained_plugins_list:
        pass



# get the output connections for this plugin then call get_plugin_chains recursively with each of plugin in the output connections
def get_plugin_chain(plugin, chain = {}, player = None):
    if not player:
        player = com.get('neil.core.player')

    # if plugin is really a list of plugins then call get_plugin_chain on each plugin and merge the result
    if type(plugin) == list:
        for item in plugin:
            get_plugin_chain(item, chain, player)

        return chain.keys()

    if is_root(plugin):
        return chain.keys()

    if plugin.get_id() not in chain.keys():
        chain[plugin.get_id()] = None

    for index in range(plugin.get_output_connection_count()):
        output_id = plugin.get_output_connection_plugin(index).get_id()
        output_plugin = player.get_plugin_by_id(output_id)
        if output_id not in chain and not is_root(output_plugin):
            chain[output_id] = None
            output_plugin = player.get_plugin_by_id(output_id)
            get_plugin_chain(output_plugin, chain, player)

    return chain.keys()



def clone_chain(src_plugins, offset = (0,0)):

    player = com.get('neil.core.player')

    plugins = {plugin_id: None for plugin_id in get_plugin_chain(src_plugins)}
    for src_id in plugins.keys():
        src_plugin = player.get_plugin_by_id(src_id)
        if not is_root(src_plugin):
            new_plugin = clone_plugin(player, src_plugin)
            plugins[src_id] = new_plugin.get_id()

    for (src_id, new_id) in plugins.items():
        src_plugin = player.get_plugin_by_id(src_id)

        new_plugin = player.get_plugin_by_id(new_id)
        output_ids = [src_plugin.get_output_connection_plugin(index).get_id() for index in range(src_plugin.get_output_connection_count())]

        for index, out_id in enumerate(output_ids):
            out_id = plugins[out_id] if (out_id in plugins and plugins[out_id] != -1) else out_id
            out_plugin = player.get_plugin_by_id(out_id)
            conn_type = src_plugin.get_output_connection_type(index)
            out_plugin.add_input(new_plugin, conn_type)

    sum_items = lambda x, y: x+y
    # patterns are copied after the making connections as the number of tracks in group 0 of the plugin patterns are the audio and event connections.
    for src_id, new_id in plugins.items():
        src_plugin = player.get_plugin_by_id(src_id)
        new_plugin = player.get_plugin_by_id(new_id)

        clone_plugin_patterns(src_plugin, new_plugin)
        clone_preset(player, src_plugin, new_plugin)

        x, y = map(sum_items, src_plugin.get_position(), offset)
        new_plugin.set_position(x, y)

    return plugins


def on_popup_clone_chain(widget, src_plugin):
    clone_chain(src_plugin, (0.1, 0.1))
    com.get('neil.core.player').history_commit("Cloned plugin chain")


def on_popup_clone_chains(widget, plugins, point = None):
    def average_position(plugin_list):
        add_positions = lambda posA, posB: (posA[0] + posB[0], posA[1] + posB[1])
        plugin_positions = [plugin.get_position() for plugin in plugin_list]

        pos_sum = reduce(add_positions, plugin_positions)

        return pos_sum / len(plugins)

    if point is None:
        offset = average_position(plugins) - point
    else:
        offset = (0.1, 0.1)

    clone_chain(plugins, offset)

    com.get('neil.core.player').history_commit("Cloned plugin chains")


def on_popup_clone_plugin(widget, plugin):
    player = com.get('neil.core.player')
    new_plugin = clone_plugin(player, plugin)
    clone_preset(player, plugin, new_plugin)
    player.history_commit("Clone plugin %s" % new_plugin.get_pluginloader().get_short_name())


def on_store_selection(widget, index, plugins):
    com.get('neil.core.router.view').store_selection(index, plugins)


def on_restore_selection(widget, index):
    com.get('neil.core.router.view').restore_selection(index)


# file chooser dialog extensions.
# ext_str is separated with semicolons and an optional description
# ext_str is ".ext", ".ext1:ext2", "description1=.ext1 : .ext2", "description1=ext1 : description2=ext2"
def get_filters(exts):
    def split_ext(str):
        return str.split("=", 2) if str.find("=") > 0 else (str, str)

    def file_filter(name, pattern):
        filter = Gtk.FileFilter()
        filter.set_name(name)
        filter.add_pattern("*" + pattern.lstrip("*"))
        return filter

    if exts.strip() == "":
        yield file_filter("*")

    for name, ext in [split_ext(ext_str) for ext_str in exts.split(":")]:
        yield file_filter(name,ext)


def build_preset_dialog(default_action, action_title, extensions):
    dialog = Gtk.FileChooserDialog(
        title="Select file",
        parent=None,
        action=default_action
    )

    dialog.add_buttons("Close", Gtk.ResponseType.CANCEL, action_title, Gtk.ResponseType.OK)

    for filter in get_filters(extensions):
        dialog.add_filter(filter)

    dialog.set_default_response(Gtk.ResponseType.OK)

    return dialog


def on_load_preset(widget, plugin):
    dialog = build_preset_dialog(Gtk.FileChooserAction.OPEN, "Open", plugin.get_preset_file_extensions())
    response = dialog.run()

    while response == Gtk.ResponseType.OK:
        print("load response")
        plugin.load_preset_file(dialog.get_filename())
        response = dialog.run()
    else:
        dialog.destroy()


def on_save_preset(widget, plugin):
    dialog = build_preset_dialog(Gtk.FileChooserAction.SAVE, "Save", plugin.get_preset_file_extensions())
    response = dialog.run()

    while response == Gtk.ResponseType.OK:
        print("save response")
        filename = dialog.get_filename()
        plugin.save_preset(dialog.get_filename())
        dialog.destroy()

        dialog = build_preset_dialog(Gtk.FileChooserAction.SAVE, "Save", plugin.get_preset_file_extensions())
        dialog.select_filename(filename)
        response = dialog.run()

