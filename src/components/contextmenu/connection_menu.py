import zzub

from neil.com import com
from neil.utils import Menu

from .actions import on_popup_disconnect, on_popup_disconnect_dialog, on_popup_disconnect_all, on_popup_edit_cv_connection
from .submenus import machine_tree_submenu


# in the plugin router view, when right mouse button clicked on one of the connections
class ConnectionMenu(Menu):
    __neil__ = dict(
        id = 'neil.core.contextmenu.connection',
        singleton = False,
        categories = [],
    )

    # connections is a list of tuples (metaplugin, index)
    # usually it's a list with one item, 
    # if it's multiple connections it's always(so far) all the inputs of the same metaplugin
    def __init__(self, connections):
        Menu.__init__(self)

        if len(connections) == 1:
            mp, index = connections[0]
            conntype = mp.get_input_connection_type(index)

            if conntype == zzub.zzub_connection_type_audio:
                self.add_submenu("Add machine", machine_tree_submenu(connection=True))
                self.add_separator()
            elif conntype == zzub.zzub_connection_type_cv:
                self.add_item("Edit cv connection", on_popup_edit_cv_connection, mp, 0)
                self.add_separator()

            self.add_item("Disconnect plugin", on_popup_disconnect, mp, index)
        else:
            self.add_item("Choose connector to delete", on_popup_disconnect_dialog, connections)
            self.add_item("Disconnect plugin", on_popup_disconnect_all, connections)
