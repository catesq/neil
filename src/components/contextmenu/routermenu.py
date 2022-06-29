import zzub
from neil.com import com
from neil.utils import Menu, is_generator, is_root, is_effect
from neil.preset import Preset

from .actions import on_popup_unmute_all
from .machinemenu import MachineMenu


class RouterMenu(MachineMenu):
    __neil__ = dict(
        id = 'neil.core.contextmenu.router',
        singleton = False,
        categories = [
        ],
    )

    def __init__(self, x, y):
        Menu.__init__(self)

        self.create_add_machine_submenu()
        self.add_separator()
        self.add_item("Unmute All", on_popup_unmute_all)

