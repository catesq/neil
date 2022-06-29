"""
This module contains the context menu component for different zzub objects
such as plugins, patterns, and so on. based on the context object currently
selected, items can choose to append themselves or not.
"""

from .singleplugin import SinglePluginMenu
from .multipleplugins import MultiplePluginMenu
from .routermenu import RouterMenu
from .connectionmenu import ConnectionMenu


__neil__ = dict(
    classes = [
        SinglePluginMenu,
        MultiplePluginMenu,
        RouterMenu,
        ConnectionMenu,
    ],
)
