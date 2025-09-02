
from .mainwindow import NeilFrame
from .helpers import Accelerators
from .framepanel import FramePanel
from .view_menu import ViewMenu
from .statusbar import StatusBar


__all__ = [
    'FramePanel',
    'ViewMenu',
    'Accelerators',
    'NeilFrame',
]

__neil__ = dict(
    classes = [
        FramePanel,
        ViewMenu,
        Accelerators,
        NeilFrame,
        StatusBar,
    ],
)

if __name__ == '__main__':
    pass
