from enum import IntEnum

class AreaType(IntEnum):
    PLUGIN = 128
    PLUGIN_OVERLAYS = 128 + 1
    
    PLUGIN_LED = 128 + 20
    PLUGIN_PAN = 128 + 40
    PLUGIN_CPU = 128 + 60

    CONNECTION = 256
    CONNECTION_OVERLAYS = 256 + 1
    CONNECTION_ARROW = 256 + 20

    HIGHLIGHT = 512
    HIGHLIGHT_DRAG = 512 + 20
