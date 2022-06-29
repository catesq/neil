import zzub
from neil.com import com
from neil.utils import Menu, is_generator, is_root, is_effect
from neil.preset import Preset

from .actions import on_popup_disconnect
from .machinemenu import MachineMenu


class ConnectionMenu(MachineMenu):
    __neil__ = dict(
        id = 'neil.core.contextmenu.connection',
        singleton = False,
        categories = [
        ],
    )

    def __init__(self, metaplugin, index):
        Menu.__init__(self)
        player = com.get('neil.core.player')

        conntype = metaplugin.get_input_connection_type(index)
        if conntype == zzub.zzub_connection_type_audio:
            self.create_add_machine_submenu(connection=True)
            menu.add_separator()

        menu.add_item("Disconnect plugins", on_popup_disconnect, metaplugin, index)
        if conntype == zzub.zzub_connection_type_event:
            # Connection connects a control plug-in to it's destination.
            # menu.add_separator()
            loader = metaplugin.get_input_connection_plugin(index).get_pluginloader()
            for i in range(loader.get_parameter_count(3)):
                param = loader.get_parameter(3, i)
                print((param.get_name()))
