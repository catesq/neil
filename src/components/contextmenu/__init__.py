"""
This module contains the context menu component for different zzub objects
such as plugins, patterns, and so on. based on the context object currently
selected, items can choose to append themselves or not.
"""

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
    ],
)
