from . import items, layers



class OverlayGfx:
    def draw_overlay(self, layer:layers.Layer, container: 'items.Item'):
        pass


class LedOverlay(OverlayGfx):
    def draw_overlay(self, layer:layers.Layer, container:'items.PluginItem'):
        pass


class PanOverlay(OverlayGfx):
    def draw_overlay(self, layer:layers.Layer, container:'items.PluginItem'):
        pass


class CpuOverlay(OverlayGfx):
    def draw_overlay(self, layer:layers.Layer, container:'items.PluginItem'):
        pass


class ConnectionArrowGfx(OverlayGfx):
    def draw_overlay(self, layer:layers.Layer, container:'items.ConnectionItem'):
        pass


__all__ = [
    'OverlayGfx',
    'LedOverlay',
    'PanOverlay',
    'CpuOverlay',
]