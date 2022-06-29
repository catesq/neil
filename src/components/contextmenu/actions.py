from gi.repository import Gtk
from neil.com import com
from neil.preset import Preset
from neil.utils import gettext, prepstr, filenameify,show_machine_manual


def on_popup_mute(widget, metaplugin):
    com.get('neil.core.player').toggle_mute(metaplugin)


def on_popup_solo(widget, metaplugin):
    player = com.get('neil.core.player')
    if player.solo_plugin != metaplugin:
        player.solo(metaplugin)
    else:
        player.solo(None)


def on_popup_bypass(widget, metaplugin):
    com.get('neil.core.player').toggle_bypass(metaplugin)


def on_popup_show_params(widget, metaplugin):
    com.get('neil.core.parameterdialog.manager').show(metaplugin, widget)


def on_popup_show_attribs(widget, metaplugin):
    dlg = com.get('neil.core.attributesdialog', metaplugin, self)
    dlg.run()
    dlg.destroy()


def on_popup_show_presets(widget, metaplugin):
    com.get('neil.core.presetdialog.manager').show(metaplugin, widget)


def on_popup_rename(widget, metaplugin):
    text = gettext(widget, "Enter new plugin name:", prepstr(metaplugin.get_name()))
    if text:
        player = com.get('neil.core.player')
        metaplugin.set_name(text)
        player.history_commit("rename plugin")


def on_popup_delete(widget, metaplugin):
    com.get('neil.core.player').delete_plugin(metaplugin)


def on_popup_clone(widget, metaplugin):
    player = com.get('neil.core.player')
    pluginloader = metaplugin.get_pluginloader()
    new_plugin = player.create_plugin(pluginloader)
    print("on clone", pluginloader, new_plugin, player)
    preset = Preset()
    preset.pickup(metaplugin)
    preset.apply(new_plugin)
    player.history_commit("Clone plugin %s" % pluginloader.get_short_name())


def on_popup_set_target(self, widget, metaplugin):
    player = com.get('neil.core.player')
    if player.autoconnect_target == metaplugin:
        player.autoconnect_target = None
    else:
        player.autoconnect_target = metaplugin


def on_popup_command(self, widget, metaplugin, subindex, index):
    plugin.command((subindex << 8) | index)


def on_popup_unmute_all(widget):
    player = com.get('neil.core.player')
    for metaplugin in reversed(list(player.get_plugin_list())):
        info = common.get_plugin_infos().get(metaplugin)
        info.muted = False
        metaplugin.set_mute(info.muted)
        info.reset_plugingfx()


def on_machine_help(widget, metaplugin):
    name = filenameify(metaplugin.get_pluginloader().get_name())
    if not show_machine_manual(name):
        info = Gtk.MessageDialog(self.get_toplevel(), flags=0, type=Gtk.MessageType.INFO, buttons=Gtk.ButtonsType.OK, message_format="Sorry, there's no help for this plugin yet")
        info.run()
        info.destroy()


def on_popup_disconnect(widget, metaplugin, index):
    plugin = metaplugin.get_input_connection_plugin(index)
    conntype = metaplugin.get_input_connection_type(index)
    metaplugin.delete_input(plugin, conntype)
    player = com.get('neil.core.player')
    player.history_commit("disconnect")


def on_popup_copy_chain(widget, plugins):
    print("copy", plugins)
