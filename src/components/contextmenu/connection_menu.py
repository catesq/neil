import zzub
from neil.com import com
from neil.utils import Menu, is_generator, is_root, is_effect
from neil.preset import Preset

from .actions import on_popup_disconnect
from .submenus import machine_tree_submenu


class ConnectionMenu(Menu):
    __neil__ = dict(
        id = 'neil.core.contextmenu.connection',
        singleton = False,
        categories = [],
    )

    def __init__(self, plugin, index):
        Menu.__init__(self)
        player = com.get('neil.core.player')
        router = com.get('neil.core.router.view')

        conntype = plugin.get_input_connection_type(index)
        if conntype == zzub.zzub_connection_type_audio:
            self.add_submenu("Add machine", machine_tree_submenu(connection=True))
            self.add_separator()

        self.add_item("Disconnect plugins", on_popup_disconnect, plugin, index)

        if conntype == zzub.zzub_connection_type_event:
            # Connection connects a control plug-in to it's destination.
            # menu.add_separator()
            loader = plugin.get_input_connection_plugin(index).get_pluginloader()
            for i in range(loader.get_parameter_count(3)):
                param = loader.get_parameter(3, i)
