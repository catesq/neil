
import zzub
from neil import components, views
from neil.utils import is_instrument, is_root, is_effect, prepstr, ui


from .submenus import restore_selection_submenu


from .actions import ( 
    on_popup_mute,
    on_popup_solo,
    on_popup_bypass,
    on_popup_show_params,
    on_popup_show_attribs,
    on_popup_show_presets,
    on_popup_rename,
    on_popup_delete,
    on_popup_clone_plugin,
    on_popup_clone_chain,
    on_popup_set_target,
    on_popup_command,
    on_machine_help,
    on_load_preset,
    on_save_preset
)


class SinglePluginMenu(ui.EasyMenu):
    __neil__ = dict(
        id = 'neil.core.contextmenu.singleplugin',
        singleton = False,
        categories = [],
    )

    def __init__(self, metaplugin):
        ui.EasyMenu.__init__(self)
        player = components.get('neil.core.player')

        self.add_check_item("_Mute", player.plugin_is_muted(metaplugin), on_popup_mute, metaplugin)

        if is_instrument(metaplugin):
            self.add_check_item("_Solo", player.solo_plugin == metaplugin, on_popup_solo, metaplugin)

        self.add_check_item("_Bypass", metaplugin.get_bypass(), on_popup_bypass, metaplugin)
        self.add_separator()
        self.add_item("_Parameters...", on_popup_show_params, metaplugin)
        self.add_item("_Attributes...", on_popup_show_attribs, metaplugin)
        self.add_item("P_resets...", on_popup_show_presets, metaplugin)
        self.add_separator()
        self.add_item("_Rename...", on_popup_rename, metaplugin)

        if not is_root(metaplugin):
            self.add_item("_Delete plugin", on_popup_delete, metaplugin)
            self.add_item("Clone _instrument", on_popup_clone_plugin, metaplugin)
            self.add_item("_Clone chain", on_popup_clone_chain, metaplugin)

        if metaplugin.get_flags() & zzub.zzub_plugin_flag_load_presets:
            self.add_item("_Load preset", on_load_preset, metaplugin)
            pass

        if metaplugin.get_flags() & zzub.zzub_plugin_flag_save_presets:
            self.add_item("_Save preset", on_save_preset, metaplugin)
            pass

        if is_effect(metaplugin) or is_root(metaplugin):
            self.add_separator()
            self.add_check_item("Default Target", player.autoconnect_target == metaplugin, on_popup_set_target, metaplugin)

        router = views.get_router()
        if router.selection_count() > 0:
            self.add_separator()
            self.add_submenu("_Restore selection", restore_selection_submenu())

        commands = metaplugin.get_commands().split('\n')
        if commands != ['']:
            self.add_separator()
            submenuindex = 0
            for index in range(len(commands)):
                cmd = commands[index]
                if cmd.startswith('/'):
                    item, submenu = self.add_submenu(prepstr(cmd[1:], fix_underscore=True))
                    subcommands = metaplugin.get_sub_commands(index).split('\n')
                    submenuindex += 1
                    for subindex in range(len(subcommands)):
                        subcmd = subcommands[subindex]
                        submenu.add_item(prepstr(subcmd, fix_underscore=True),
                                         on_popup_command, metaplugin,
                                         submenuindex, subindex)
                else:
                    self.add_item(prepstr(cmd), on_popup_command, metaplugin, 0, index)

        self.add_separator()
        self.add_item("_Help", on_machine_help, metaplugin)


