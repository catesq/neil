from .layer import Layer
from . import containers




class OverlayGfx:
    def draw_overlay(self, layer:Layer, container):
        pass



class LedOverlay(OverlayGfx):
    def draw_overlay(self, layer:Layer, container:'containers.PluginContainer'):
        pass



class PanOverlay(OverlayGfx):
    def draw_overlay(self, layer:Layer, container:'containers.PluginContainer'):
        pass



class CpuOverlay(OverlayGfx):
    def draw_overlay(self, layer:Layer, container:'containers.PluginContainer'):
        pass

__all__ = [
    'OverlayGfx',
    'LedOverlay',
    'PanOverlay',
    'CpuOverlay',
]