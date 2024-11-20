from .area_type import AreaType
from .colors import RouterColors
from .click_area import ClickArea
from .container import Container
from .layer import Layer

import zzub
import neil.common as common
from neil.utils import Vec2, Sizes, Area, OffsetArea
from .plugin_gfx import PluginGfx

from . import overlay_gfx 


# each plugin in the router has one of these
class PluginContainer(Container):
    def __init__(self, mp:zzub.Plugin, info:common.PluginInfo, 
                 pos: Vec2, sizes:Sizes, colors: RouterColors):
        
        self.metaplugin:zzub.Plugin = mp
        self.info = info
        Container.__init__(self, mp.get_id(), AreaType.PLUGIN, pos, sizes, colors)

    def setup_sizes(self, sizes):
        self.size = sizes['plugin']
        self.margins = Vec2(sizes['gap'], sizes['gap'])

        self.plugin_area = Area(self.pos, self.size)
        self.led_area = OffsetArea(self.pos, sizes['led_offs'], sizes['led_size'])
        self.pan_area = OffsetArea(self.pos, sizes['pan_offs'], sizes['pan_size'])
        self.cpu_area = OffsetArea(self.pos, sizes['cpu_offs'], sizes['cpu_size'])

    def setup_views(self):
        self.plugin_display = self.setup_plugin_display()
        self.overlays = self.get_plugin_overlays()

    def setup_plugin_display(self):
        return PluginGfx()

    def get_plugin_overlays(self) -> dict[AreaType, 'overlay_gfx.OverlayGfx']:
        return {
            AreaType.PLUGIN_LED: overlay_gfx.LedOverlay(),
            AreaType.PLUGIN_PAN: overlay_gfx.PanOverlay(),
            AreaType.PLUGIN_CPU: overlay_gfx.CpuOverlay(),
        }
    
    def draw_overlays(self, layer: Layer):
        for overlay in self.overlays.values():
            overlay.draw_overlay(layer, self)

    def draw_overlay(self, layer: Layer, area_type:AreaType):
        if area_type in self.overlays:
            self.overlays[area_type].draw_overlay(layer, self)

    def draw_item(self, layer: Layer):
        self.plugin_display.draw_layer(layer, self)

    def add_click_areas(self, clickareas: ClickArea):
        clickareas.add_object(self, self.led_area, AreaType.PLUGIN_CPU)
        clickareas.add_object(self, self.led_area, AreaType.PLUGIN_LED)
        clickareas.add_object(self, self.pan_area, AreaType.PLUGIN_PAN)
        clickareas.add_object(self, self.plugin_area, AreaType.PLUGIN)

    def remove_click_areas(self, clickareas: ClickArea):
        clickareas.remove_object(self)

    def __repr__(self):
        return "PluginContainer({})".format(self.metaplugin.get_name())

