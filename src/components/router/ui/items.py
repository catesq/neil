from .area_type import AreaType
from .click_area import ClickArea

import zzub
import neil.common as common
from neil.utils import Vec2, Sizes, Area, Colors
# import plugin_gfx 


from . import layer, plugin_gfx, overlay_gfx


# Every plugin and connection has a container. 
# to store gui positions and colors and have some helper methods used
#  by router_layer.ContainersOverlayer to draw the plugin/connection

# router_layer.RouterLayer manages the lists of Containers and updates the position etc
class Item:
    def __init__(self, id, type: int, colors: Colors):
        
        self.type = type
        self.id = id
        self.size:Vec2 = None,
        self.pos:Vec2 = None
        self.zoom:Vec2 = Vec2(1,1)
        # # corner points of clip rectangle (pixel screen)
        # self.area: Area = None

        # # mid point (normalised internal coords -1 to +1)
        # self.norm_pos = Vec2(0, 0)

        # top left corner of clip rectangle (pixel screen pos) - initialise in reposition
        
                
        # RouterColors from components/router/ui/colors 
        self.colors: Colors = colors          

        # self.setup_dimensions(sizes)

        self.display: plugin_gfx.DisplayGfx = self.create_display()
        self.overlays: dict[AreaType, overlay_gfx.OverlayGfx] = self.create_overlays()

    def set_zoom(self, zoom):
        self.zoom.set(zoom)
        self.update_dimensions()

    # geometry is screen size (halved for convenience)
    # sizes is a neil.utils.Sizes ie components.router.ui.utils.router_sizes

    # The geometry is halved as the normalised sizes run from -1 to +1 instead of 0 to 1
    # so given normalised mid point = (0,0) 
    # norm_mid_point * screensize / 2 + screensize / 2  -> middle of screen
    def init_dimensions(self, geometry, sizes):
        pass

    # norm_mid_point * screensize / 2 + screensize / 2  -> middle of screen
    def update_dimensions(self):
        pass

    # called a few times before gtk settles on the final size
    def set_canvas_size(self, canvas_size:Vec2):
        pass

    def create_display(self) -> plugin_gfx.DisplayGfx:
        pass

    def create_overlays(self) -> dict[AreaType, overlay_gfx.OverlayGfx]:
        pass

    def scroll_to(self, scroll):
        pass

    def set_zoom(self, zoom):
        pass


    def add_click_areas(self, clickareas: ClickArea):
        pass

    def remove_click_areas(self, clickareas: ClickArea):
        pass

    def draw_item(self, layer: layer.Layer):
        self.display.draw_gfx(layer, self)

    def draw_overlays(self, layer: layer.Layer):
        for overlay in self.overlays.values():
            overlay.draw_overlay(layer, self)

    def draw_overlay(self, layer: layer.Layer, area_type:AreaType):
        if area_type in self.overlays:
            self.overlays[area_type].draw_overlay(layer, self)



# each plugin in the router has one of these
class PluginItem(Item):
    def __init__(self, mp:zzub.Plugin, info:common.PluginInfo, colors: Colors):
        Item.__init__(self, mp.get_id(), AreaType.PLUGIN, colors)

        self.metaplugin:zzub.Plugin = mp
        self.info = info

        self.norm_plugin_pos = Vec2(*mp.get_position())

        self.scroll = Vec2(0,0)

    def init_dimensions(self, sizes):
        self.plugin_pos = self.norm_plugin_pos
        self.plugin_size = sizes['plugin']
        self.half_plugin = (sizes['plugin'] / 2).ints()

        # internal pixel sizes - scaled in self.update_areas using self.zoom
        self.init_led = Area(sizes['led'])
        self.init_pan = Area(sizes['pan'])
        self.init_cpu = Area(sizes['cpu'])
        print("init_led", sizes['led'])
        print("init_pan", sizes['pan'])

        # final screen coordinates relative to top left corner of layer/surface/gtk.DrawingArea
        self.plugin_area = Area()
        self.led_area = Area()
        self.cpu_area = Area()
        self.pan_area = Area()

        self.pos = self.plugin_area.pos
        self.size = self.plugin_area.size


    # There's a few resize events before the final canvas size is determined
    def set_canvas_size(self, size):
        self.init_canvas = size
        self.shift_pos = (self.init_canvas / 2 + self.scroll).ints()
        self.plugin_pos = (self.norm_plugin_pos * size / 2).ints()
        self.update_dimensions()



    # use current plugin pos, scroll position and zoom level to recalculate screen coordinates
    def update_dimensions(self):
        print("update plugin areas zoom", self.zoom)
        top_left = (self.plugin_pos * self.zoom + self.shift_pos - self.half_plugin)

        self.plugin_area.pos.set(top_left).ints()
        self.plugin_area.size.set(self.plugin_size * self.zoom).ints()

        self.led_area.pos.set(top_left + self.init_led.pos * self.zoom).ints()
        self.led_area.size.set(self.init_led.size * self.zoom).ints()
        print("old pan area", self.pan_area)

        self.pan_area.pos.set(top_left + self.init_pan.pos * self.zoom).ints()
        self.pan_area.size.set(self.init_pan.size * self.zoom).ints()
        print("new pan area", self.pan_area)

        self.cpu_area.pos.set(top_left + self.init_cpu.pos * self.zoom).ints()
        self.cpu_area.size.set(self.init_cpu.size * self.zoom).ints()
 
    def create_display(self) -> plugin_gfx.DisplayGfx:
        return plugin_gfx.PluginGfx(self.colors.router)

    def create_overlays(self) -> dict[AreaType, overlay_gfx.OverlayGfx]:
        return {
            AreaType.PLUGIN_LED: overlay_gfx.LedOverlay(),
            AreaType.PLUGIN_PAN: overlay_gfx.PanOverlay(),
            AreaType.PLUGIN_CPU: overlay_gfx.CpuOverlay(),
        }
    
    def add_click_areas(self, clickareas: ClickArea):
        clickareas.add_object(self.metaplugin, self.led_area, AreaType.PLUGIN_LED)
        clickareas.add_object(self.metaplugin, self.pan_area, AreaType.PLUGIN_PAN)
        clickareas.add_object(self.metaplugin, self.plugin_area, AreaType.PLUGIN)

    def remove_click_areas(self, clickareas: ClickArea):
        clickareas.remove_object(self.metaplugin)
    
    def __repr__(self):
        return "PluginItem({})".format(self.metaplugin.get_name())


# the connection index is on the target id
class ConnId:
    def __init__(self, source_id, target_id, target_conn_index):
        self.source_id = source_id
        self.target_id = target_id
        self.conn_index = target_conn_index

    def __repr__(self):
        return "ConnId({}, {}, {})".format(self.source_id, self.target_id, self.conn_index)



class ConnectionItem(Item):
    edge_offsets = {
        'top':    Vec2(1,0),
        'bottom': Vec2(1,0),
        'left':   Vec2(0,1),
        'right':  Vec2(0,1)
    }

    def __init__(self, index: int, source_item:PluginItem, target_item:PluginItem, colors: Colors):
        conn_id = ConnId(source_item.id, target_item.id, index)
        Item.__init__(self, conn_id, AreaType.CONNECTION, colors)
    
        self.source_item = source_item
        self.target_item = target_item

        self.gfxcache = common.GfxCache()

        connection = target_item.metaplugin.get_input_connection(index)
        self.connection:zzub.Connection = connection
        self.is_audio = connection.get_type() == zzub.zzub_connection_type_audio


    def init_dimensions(self, sizes):
        self.line_width = sizes['line_width']
        self.line_border = sizes['line_border']
        self.volume_size = sizes['vol_knob']
        self.volume_area = Area()
        self.line_gap = sizes['gap']

    # this needs to be done after the plugin positions are calculated
    def set_canvas_size(self, size):
        self.update_dimensions()

    def create_display(self) -> plugin_gfx.DisplayGfx:
        return plugin_gfx.ConnectionGfx(self.colors.router)

    def create_overlays(self) -> dict[AreaType, overlay_gfx.OverlayGfx]:
        return {
            AreaType.CONNECTION_ARROW: overlay_gfx.ConnectionArrowGfx(),
        }

    def uses_plugin(self, plugin):
        return self.source_item.metaplugin == plugin or self.target_item.metaplugin == plugin     

    def update_dimensions(self):
        print("update connection positions", self.source_item.plugin_area, self.target_item.plugin_area)
        source, target = self.get_nearest_edges()

        self.source_edge_offset = self.get_edge_offset(source[0])
        self.target_edge_offset = self.get_edge_offset(target[0])

        self.source_midpos = source[1]
        self.target_midpos = target[1]

        if self.is_audio:
            self.source_pos = self.source_midpos
            self.target_pos = self.target_midpos
        else:
            self.source_pos = self.source_midpos + self.source_edge_offset * self.line_gap
            self.target_pos = self.target_midpos + self.target_edge_offset * self.line_gap

        self.pos = (self.source_pos + self.target_pos) / 2

        self.volume_area.pos.set(self.pos - self.volume_size * self.zoom / 2).ints()
        self.volume_area.size.set(self.pos - self.volume_size * self.zoom / 2).ints()



    # From the area of the source and target plugin,  
    # get the name and middle of the closest edges on each rectangle 
    # ( 
    #   ('top',  Vec2(x1, y1)),        # source edge
    #   ('left', Vec2(x2, y2))         # target edge
    # )
    def get_nearest_edges(self):
        # chatgpt answer
        # Calculate midpoints of the edges of source and target items
        source_midpoints = self.get_edge_midpoints(self.source_item.plugin_area)
        target_midpoints = self.get_edge_midpoints(self.target_item.plugin_area)

        # Determine the nearest edges
        min_distance = float('inf')
        nearest_source_edge = None
        nearest_target_edge = None
        nearest_source_mid = None
        nearest_target_mid = None

        for source_edge, source_midpoint in source_midpoints.items():
            for target_edge, target_midpoint in target_midpoints.items():
                distance = ((source_midpoint.x - target_midpoint.x) ** 2 + (source_midpoint.y - target_midpoint.y) ** 2) ** 0.5
                if distance < min_distance:
                    min_distance = distance
                    nearest_source_edge, nearest_source_mid = source_edge, source_midpoint
                    nearest_target_edge, nearest_target_mid = target_edge, target_midpoint

        # Draw the line from the nearest source edge to the nearest target edge

        return (nearest_source_edge, nearest_source_mid), (nearest_target_edge, nearest_target_mid)

    # the mid points of every edge of a rectangle
    def get_edge_midpoints(self, area: Area):
        return {
            'top':    Vec2(area.x + area.width / 2, area.y),
            'bottom': Vec2(area.x + area.width / 2, area.y + area.height),
            'left':   Vec2(area.x,                  area.y + area.height / 2),
            'right':  Vec2(area.x + area.width,     area.y + area.height / 2)
        }
    
    # used to move midpoints vertical or horizontal along whichever edge it's on
    def get_edge_offset(self, edge_name):
        return ConnectionItem.edge_offsets[edge_name]

    def setup_views(self):
        self.connection_display = plugin_gfx.ConnectionGfx()
        self.overlays = overlay_gfx.VolumeGfx()

    def add_click_areas(self, clickareas: ClickArea):
        clickareas.add_object(self.id, self.volume_area, AreaType.CONNECTION_ARROW)
    
    def remove_click_areas(self, clickareas: ClickArea):
        clickareas.remove_object(self.id)
