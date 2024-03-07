"""
the context menu in the fx router for plugins, patterns
"""
from .disconnect_dialog import ConnectDialog
from .single_plugin_menu import SinglePluginMenu
from .multiple_plugin_menu import MultiplePluginMenu
from .router_menu import RouterMenu
from .connection_menu import ConnectionMenu


__neil__ = dict(
    classes = [
        SinglePluginMenu,
        MultiplePluginMenu,
        RouterMenu,
        ConnectionMenu,
        ConnectDialog,
    ],
)
