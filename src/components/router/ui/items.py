from typing import Sequence, Tuple

import cairo
from .area_type import AreaType
from .click_area import ClickArea

import zzub
import neil.common as common
from neil.utils import Vec2, Area, Colors
# import plugin_gfx 


from . import layers, plugin_gfx, overlay_gfx



def get_rect_edge_midpoints(area: Area) -> Sequence[tuple[str, Vec2]]:
    # the mid points of every edge of a rectangle
    return (
        ('top',    Vec2(area.x + area.width / 2, area.y)),
        ('bottom', Vec2(area.x + area.width / 2, area.y + area.height)),
        ('left',   Vec2(area.x,                  area.y + area.height / 2)),
        ('right',  Vec2(area.x + area.width,     area.y + area.height / 2))
    )


def get_nearest_rect_edges(source_rect, target_rect):
    # Calculate nearest midpoints of any edge of source and target plugins
    # return: ( 
    #   (source_name:str, source_pos:Vec2), 
    #   (target_name:str, source_pos:Vec2), 
    # ) 
    # examples of target name are 'top', 'left' or 'drag'
    source_midpoints = get_rect_edge_midpoints(source_rect)
    target_midpoints = get_rect_edge_midpoints(target_rect)

    return get_nearest_points(source_midpoints, target_midpoints)



# Every plugin and connection has a container. 
# to store gui positions and colors and have some helper methods used
#  by router_layer.ContainersOverlayer to draw the plugin/connection

# router_layer.RouterLayer manages the lists of Containers and updates the position etc
class Item:
    init_canvas: Vec2
    size: Vec2
    pos: Vec2
    zoom: Vec2

    def __init__(self, id, colors: Colors):
        
        # self.type = type
        self.id = id
        # self.init_canvas:Vec2 = None
        # self.canvas_size:Vec2 = None 
        # self.size:Vec2 = None,
        # self.pos = None
        self.zoom = Vec2(1,1)

        self.colors: Colors = colors          

        # self.display: plugin_gfx.DisplayGfx = self.create_display()
        # self.overlays: dict[AreaType, overlay_gfx.OverlayGfx] = self.create_overlays()

    def draw_item(self, context: cairo.Context):
        """draw the main graphics"""
        pass


    def set_zoom(self, zoom):
        self.zoom.set(zoom)
        self.update_dimensions()


    # geometry is screen size 
    # sizes is a usable neil.utils.Sizes subclass  components.router.ui.utils.router_sizes

    def init_dimensions(self, sizes):
        """  """
        pass

    
    def update_dimensions(self):
        """ usually called by set_canvas_size """
        pass

    # self.norm_mid_point * screensize / 2 + screensize / 2  = middle of screen
    def init_canvas_size(self, size: Vec2):
        """
        Only called after canvas size is known and before anything is drawn.
        gtk tries a few times before settles on a final size.
        """
        self.init_canvas = Vec2(size)
        self.set_canvas_size(size)

    def set_canvas_size(self, canvas_size:Vec2):
        """
        This is called after screen was drawn.
        """
            # There's a few resize events before the final canvas size is determined
        self.canvas_size = Vec2(canvas_size)
        self.update_dimensions()


    def scroll_to(self, scroll):
        """ Screen scrolled, adjust item locations"""
        pass

    
    def uses_plugin(self, plugin):
        """ Returns true if the item is/connects to the plugin"""
        return False


    def plugin_moved(self, plugin, norm_pos:Vec2):
        # updates the display position of the plugin 
        # does not update the actual plugin as a history checkpoint is added when
        #  the plugin location is changed and this spams the history log in mouse motion events
        pass


    def add_click_areas(self, clickareas: ClickArea):
        """ add mouse sensitive spots  """
        pass

    def remove_click_areas(self, clickareas: ClickArea):
        """ remove mouse click areas """
        pass




# each plugin in the router has one of these
class PluginItem(Item):
    def __init__(self, mp:zzub.Plugin, info:common.PluginInfo, colors: Colors):
        Item.__init__(self, mp.get_id(), colors)

        self.metaplugin:zzub.Plugin = mp
        self.info = info

        self.norm_plugin_pos = Vec2(*mp.get_position())

        self.scroll = Vec2(0,0)
        self.plugingfx = plugin_gfx.PluginGfx(self)
        self.overlays = {
            AreaType.PLUGIN_LED: overlay_gfx.LedOverlay(),
            AreaType.PLUGIN_PAN: overlay_gfx.PanOverlay(),
            AreaType.PLUGIN_CPU: overlay_gfx.CpuOverlay(),
        }
        


    def init_dimensions(self, sizes):
        self.plugin_pos = self.norm_plugin_pos
        self.plugin_size = sizes['plugin']
        self.half_plugin = (sizes['plugin'] / 2).ints()

        # internal pixel sizes - scaled in self.update_areas using self.zoom
        self.init_led = Area(sizes['led'])
        self.init_pan = Area(sizes['pan'])
        self.init_cpu = Area(sizes['cpu'])

        # final screen coordinates relative to top left corner of layer/surface/gtk.DrawingArea
        self.plugin_area = Area()
        self.led_area = Area()
        self.cpu_area = Area()
        self.pan_area = Area()

        self.pos = self.plugin_area.pos
        self.size = self.plugin_area.size


    # use current plugin pos, scroll position and zoom level to recalculate screen coordinates
    def update_dimensions(self):
        self.plugin_pos = (self.norm_plugin_pos * self.init_canvas / 2).ints()

        top_left = (self.plugin_pos * self.zoom + self.shift_pos - self.half_plugin)
        self.plugin_area.pos.set(top_left).ints()
        self.plugin_area.size.set(self.plugin_size * self.zoom).ints()

        self.led_area.set(self.init_led * self.zoom).ints()
        self.pan_area.set(self.init_pan * self.zoom).ints()
        self.cpu_area.set(self.init_cpu * self.zoom).ints() 


    def draw_item(self, context: cairo.Context):
        """draw the main graphics"""
        self.plugingfx.draw_gfx(context)


    def draw_overlays(self, context: cairo.Context):
        """ draw all the overlays """
        for overlay in self.overlays.values():
            overlay.draw_overlay(context)


    def draw_overlay(self, context: cairo.Context, area_type:AreaType):
        """ draw an individual overlay """
        if area_type in self.overlays:
            self.overlays[area_type].draw_overlay(context)

    def uses_plugin(self, plugin):
        return self.metaplugin == plugin


    def plugin_moved(self, plugin:zzub.Plugin, norm_pos:Vec2):
        self.norm_plugin_pos.set(norm_pos)
        self.update_dimensions()


    def init_canvas_size(self, size):
        self.shift_pos = (size / 2 + self.scroll).ints()
        super().init_canvas_size(size)


    def add_click_areas(self, clickareas: ClickArea):
        clickareas.add_object(self.id, self.metaplugin, self.led_area, AreaType.PLUGIN_LED)
        clickareas.add_object(self.id, self.metaplugin, self.pan_area, AreaType.PLUGIN_PAN)
        clickareas.add_object(self.id, self.metaplugin, self.plugin_area, AreaType.PLUGIN)


    def remove_click_areas(self, clickareas: ClickArea):
        clickareas.remove_object(self.metaplugin)
    

    def __repr__(self):
        return "PluginItem({})".format(self.metaplugin.get_name())


# the connection index is on the target id
class ConnID:
    def __init__(self, source_id, target_id, target_conn_index):
        self.source_id = source_id
        self.target_id = target_id
        self.conn_index = target_conn_index

    def __repr__(self):
        return "ConnID({}, {}, {})".format(self.source_id, self.target_id, self.conn_index)



    # Often between two plugins and the from_points are the edges of the plugin's outline 
    # get the name and middle of the closest edges on each rectangle 
    # The name of the edge doesn't matter and is used as a return value so the edge is known
    # eg
    # sources = ( ('top', Vec(0.5, 0)), ('left', Vec(0, 0.5)), ('right', Vec(1,0.5), ('bottom', Vec(0.5,1)))
    # targets = ( ('left', Vec(5,0.5)), ...  )
    # returns (nearest_source, nearest_target) -> ( ('right', Vec(1,0.5)), ('left', Vec(5,0.5)) )
    # 
def get_nearest_points(sources: Sequence[Tuple[str, Vec2]], targets: Sequence[Tuple[str, Vec2]]):
        # Determine the nearest edges
    min_distance = float('inf')
    nearest_source_edge = None
    nearest_target_edge = None
    nearest_source_mid = None
    nearest_target_mid = None

    for source_edge, source_midpoint in sources:
        for target_edge, target_midpoint in targets:
            distance = ((source_midpoint.x - target_midpoint.x) ** 2 + (source_midpoint.y - target_midpoint.y) ** 2) ** 0.5
            if distance < min_distance:
                min_distance = distance
                nearest_source_edge, nearest_source_mid = source_edge, source_midpoint
                nearest_target_edge, nearest_target_mid = target_edge, target_midpoint

    
    return (nearest_source_edge, nearest_source_mid), (nearest_target_edge, nearest_target_mid)


class ConnectionItem(Item):
    source_pos: Vec2
    target_pos: Vec2

    edge_offsets = {
        'top':    Vec2(1,0),
        'bottom': Vec2(1,0),
        'left':   Vec2(0,1),
        'right':  Vec2(0,1)
    }

    def __init__(self, index: int, connection:zzub.Connection, source_item:PluginItem, target_item:PluginItem, colors: Colors):
        conn_id = ConnID(source_item.id, target_item.id, index)

        Item.__init__(self, conn_id, colors)
    
        self.source_item = source_item
        self.target_item = target_item
        self.display = plugin_gfx.ConnectionGfx(self)

        self.source_pos = Vec2(source_item.pos)
        self.target_pos = Vec2(target_item.pos)

        # self.gfxcache = common.GfxCache()  # for volume arrow

        self.connection:zzub.Connection = connection
        self.is_audio = connection.get_type() == zzub.zzub_connection_type_audio


    def init_dimensions(self, sizes):
        self.line_width = sizes['line_width']
        self.line_border = sizes['line_border']
        self.volume_size = sizes['vol_knob']
        self.volume_area = Area()
        self.line_gap = sizes['gap']

    def draw_item(self, context: cairo.Context):
        self.display.draw_gfx(context)

    def uses_plugin(self, plugin):
        return self.source_item.metaplugin == plugin or self.target_item.metaplugin == plugin     

    def plugin_moved(self, plugin, norm_pos):
        self.update_dimensions()

    def update_dimensions(self):
        source, target = get_nearest_rect_edges(self.source_item.plugin_area, self.target_item.plugin_area)

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

    
    # used to move midpoints vertical or horizontal along whichever edge it's on
    def get_edge_offset(self, edge_name):
        return ConnectionItem.edge_offsets[edge_name]

    # def setup_views(self):
    #     self.connection_display = plugin_gfx.ConnectionGfx()
    #     self.overlays = overlay_gfx.VolumeGfx()

    def add_click_areas(self, clickareas: ClickArea):
        clickareas.add_object(self.id, self.connection, self.volume_area, AreaType.CONNECTION_ARROW)
    
    def remove_click_areas(self, clickareas: ClickArea):
        clickareas.remove_by_id(self.id)



class DragId:
    def __init__(self, plugin_id):
        self.id = plugin_id


# can't drag plugin and connection at same time so don't override
# plugin_moved or uses_plugin
class DragConnectionItem(Item):
    source_pos: Vec2
    target_pos: Vec2

    def __init__(self, source_item:PluginItem, target_pos: Vec2, colors: Colors):
        Item.__init__(self, DragId(source_item.id), colors)

        self.source_id = source_item.id
        self.source_item = source_item
        self.target_pos = target_pos
        self.display = plugin_gfx.DragConnectionGfx(self)
        self.source_pos = Vec2(0, 0)
        self.hidden = True                  # plugin's can't connect to themself so initally hidden
        self.target_item:PluginItem = None  # this is only set when drag point is over a plugin


    def draw_item(self, context: cairo.Context):
        if not self.hidden:
            self.display.draw_gfx(context)

        if self.target_item:
            pass # todo outline target with green or red depending if valid target


    def set_target_invalid(self):
        self.hidden = True


    def set_target_pos(self, x, y):
        self.hidden = False
        self.target_pos.set(x, y)
        self.target_item = None
        self.update_dimensions()


    def set_target_item(self, target_item):
        self.hidden = False
        self.target_item = target_item
        self.update_dimensions()


    # use current plugin pos, scroll position and zoom level to recalculate screen coordinates
    def update_dimensions(self):
        if self.target_item:
            source, target = get_nearest_rect_edges(self.source_item.plugin_area, self.target_item.plugin_area)
            print(self.source_pos, self.target_pos)
            self.source_pos = source[1]
            self.target_pos = target[1]
        else:
            source_midpoints = get_rect_edge_midpoints(self.source_item.plugin_area)
            source, _ = get_nearest_points(source_midpoints, [('drag', self.target_pos)])
            self.source_pos = source[1]
        # print("drag from", self.source_pos, "to", self.drag_pos)
 

    def __repr__(self):
        return "DragConnectionItem({})".format(self.metaplugin.get_name())