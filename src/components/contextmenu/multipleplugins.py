
import os.path

import zzub
from neil.com import com
from neil.utils import Menu, is_generator

from .actions import ( on_popup_mute,
                       on_popup_solo,
                       on_popup_bypass,
                       on_popup_show_params,
                       on_popup_show_attribs,
                       on_popup_show_presets,
                       on_popup_rename )

class MultiplePluginMenu(Menu):
    __neil__ = dict(
        id = 'neil.core.contextmenu.multipleplugins',
        singleton = False,
        categories = [
        ],
    )

    def __init__(self, metaplugins):
        Menu.__init__(self)
        player = com.get('neil.core.player')

        self.add_check_item("_Mute", common.get_plugin_infos().get(metaplugin).muted, on_popup_mute, metaplugins)

        if is_generator(metaplugin):
            self.add_check_item("_Solo", player.solo_plugin == metaplugin, on_popup_solo, metaplugins)

        self.add_check_item("_Bypass", metaplugin.get_bypass(), on_popup_bypass, metaplugins)
        self.add_separator()
        self.add_item("_Clone...", on_popup_clone, metaplugin)



