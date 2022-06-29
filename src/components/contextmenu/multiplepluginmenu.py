
import os.path

import zzub
from neil import common
from neil.com import com
from neil.utils import Menu, is_generator

from .actions import (
    on_popup_mute_selected,
    on_popup_solo,
    on_popup_bypass,
    on_popup_copy_chain,
    on_popup_save_chain
)

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
        router = com.get('neil.core.router.view')

        self.add_check_item("_Mute", player.group_is_muted(metaplugins), on_popup_mute_selected, metaplugins)


        self.add_separator()

        store_submenu   = Menu()
        restore_submenu = Menu()

        for i in range(10):
            store_submenu.add_item("Group _%d".format(i), router.on_store_selection, 1, metaplugins)
            restore_submenu.add_item("Group _%d".format(i), router.on_restore_selection, 1, metaplugins)

        self.make_submenu_item(store_submenu, "_Remember selection")
        self.make_submenu_item(restore_submenu, "_Restore selection")
        self.add_separator()

        self.add_item("_Clone", on_popup_copy_chain, metaplugins)
        self.add_item("_Save", on_popup_save_chain, metaplugins)




