
from .frame import NeilFrame, Accelerators, FramePanel
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
