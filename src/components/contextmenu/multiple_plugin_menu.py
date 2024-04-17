
from neil import components
from neil.utils import ui


from .submenus import store_selection_submenu, restore_selection_submenu


from .actions import (
    on_popup_mute_selected,
#    on_popup_solo,
#    on_popup_bypass,`
#    on_popup_clone_chain,
    on_popup_clone_chains,
    on_popup_save_chain,
    on_popup_delete_group
)


class MultiplePluginMenu(ui.EasyMenu):
    __neil__ = dict(
        id = 'neil.core.contextmenu.multipleplugins',
        singleton = False,
        categories = [],
    )

    def __init__(self, plugins):
        ui.EasyMenu.__init__(self)
        player = components.get('neil.core.player')
        router = components.get('neil.core.router.view')

        self.add_check_item("_Mute", player.group_is_muted(plugins), on_popup_mute_selected, plugins)
        self.add_item("_Delete", on_popup_delete_group, plugins)

        self.add_separator()
        self.add_submenu("Keep selection", store_selection_submenu(plugins))

        if router.has_selection():
            self.add_submenu("_Restore selection", restore_selection_submenu())

        self.add_separator()

        self.add_item("_Clone selected", on_popup_clone_chains, plugins)
        self.add_item("_Save selected", on_popup_save_chain, plugins)




