from .layer import Layer
from . import items



class OverlayGfx:
    def draw_overlay(self, layer:Layer, container):
        pass


class LedOverlay(OverlayGfx):
    def draw_overlay(self, layer:Layer, container:'items.PluginItem'):
        pass


class PanOverlay(OverlayGfx):
    def draw_overlay(self, layer:Layer, container:'items.PluginItem'):
        pass


class CpuOverlay(OverlayGfx):
    def draw_overlay(self, layer:Layer, container:'items.PluginItem'):
        pass


class ConnectionArrowGfx(OverlayGfx):
    def draw_overlay(self, layer:Layer, container:'items.ConnectionItem'):
        pass


__all__ = [
    'OverlayGfx',
    'LedOverlay',
    'PanOverlay',
    'CpuOverlay',
]