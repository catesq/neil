import zzub

from neil.com import com
from neil.utils import Menu, is_generator, is_root, is_effect
from neil.preset import Preset

from .actions import on_popup_unmute_all, on_popup_clone_chains

from .submenus import machine_tree_submenu, restore_selection_submenu, store_selection_submenu


class RouterMenu(Menu):
    __neil__ = dict(
        id = 'neil.core.contextmenu.router',
        singleton = False,
        categories = [],
    )

    def __init__(self, x, y):
        Menu.__init__(self)

        router = com.get("neil.core.router.view")
        player = com.get("neil.core.player")

        self.add_submenu("_Add machine", machine_tree_submenu(connection=False))


        if player.active_plugins or router.selection_count() > 0:
            self.add_separator()
            if player.active_plugins:
                self.add_item("_Clone selected", on_popup_clone_chains, player.active_plugins, router.pixel_to_float((x, y)))
                self.add_submenu("_Keep selection", store_selection_submenu(player.active_plugins))

            if router.selection_count() > 0:
                self.add_submenu("_Recall selection", restore_selection_submenu())

        self.add_separator()
        self.add_item("Unmute all", on_popup_unmute_all)

