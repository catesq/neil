from typing import Tuple, Union
import weakref
import gi

import zzub
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Pango
from typing import Generator
import cairo
from neil.utils import ( 
    Sizes, Vec2, Area, OffsetArea
)
from enum import IntEnum
import neil.common as common
from neil.common import PluginGfx
from .colors import RouterColors

class AreaType(IntEnum):
    PLUGIN = 128
    PLUGIN_OVERLAYS = 128 + 1
    CONNECTION = 256
    CONNECTION_OVERLAYS = 256 + 1
    CONNECTION_ARROW = 256 + 20
    PLUGIN_LED = 128 + 20
    PLUGIN_PAN = 128 + 40
    PLUGIN_CPU = 128 + 60
    GROUP_MASK = 128 + 32


class ClickAreas:
    def __init__(self):
        self.objects = []

    def add_object(self, obj, area: Area, area_type: AreaType):
        """
        Registers a new object.

        @param obj: The ui object to associate with the click box.
        @param area: The x coordinate of the top-left corner of the click box.
        """
        self.objects.append((obj, area, area_type))

    def remove_object(self, obj):
        """
        Removes an object.

        @param obj: The object whose click box should be removed.
        """
        prev_size = self.objects.size()
        self.objects = [click_box for click_box in self.objects if click_box[0] != obj]
        return self.objects.size() == prev_size

    def get_object_at(self, x, y) -> Tuple[object, Area, AreaType]:
        """
        Finds the object at a specific position.

        @param x: The x coordinate.
        @param y: The y coordinate.
        @return: The object and area at the specified position, or None,None if no object is found.
        """
        for obj, area, area_type in self.objects:
            if area.contains(x, y):
                return obj, area, area_type
        return None, None, None

    def get_object_group_at(self, x, y, obj_group) -> Tuple[object, Area, AreaType]:
        """
        Finds the object at a specific position.

        @param x: The x coordinate.
        @param y: The y coordinate.
        @return: The object and area at the specified position, or None,None if no object is found.
        """
        for obj, area, area_type in self.objects:
            if (area_type & AreaType.GROUP_MASK) == obj_group and area.contains(x, y):
                return obj, area, area_type
        return None, None, None

    def clear(self):
        self.objects.clear()



# each layer draws to it's internal surface which is later painted 
# a parent's layer Cairo.Context in copy_layer
class Layer:
    def __init__(self, pos:Vec2 = None, size:Vec2=None, bg=(0,0,0)):
        self.pos = pos if pos is not None else Vec2(0,0)
        self.size: Vec2 = size
        self.surface: cairo.Surface = None
        self.context: cairo.Context = None
        self.painted = False
        self.do_draw_layer = False
        self.do_create_surface = False
        self.is_transparent_bg = False

        self.set_bg(*bg)
        
    def set_bg(self, *args):
        """
        Sets the background color of the layer group.

        @param color: bg color, tuple or list with 3 or 4 floats (r,g,b) or (r,g,b,a) 
        """
        self.bg = [max(0, min(1, float(arg))) for arg in args[:3]]
        self.is_transparent_bg = (len(args) == 4)

    def set_refresh(self):
        self.do_create_surface = True

    
    # first called in Gtk.Widget::realize
    def resized(self, size: Vec2, prev_size: Vec2):
        pass
        

    def set_size(self, x, y):
        if not self.size or self.size.x != x or self.size.y != y:
            size = Vec2(x, y)
            self.resized(size, self.size)
            self.size = size
            self.do_create_surface = True

    # only called in Gtk.Widget::realize when the top layer has a Gtk.Widget parent
    def set_parent(self, parent: Union[Gtk.Widget, 'Layer']):
        self.prepare(weakref.proxy(parent))


    # called by set_parent
    def prepare(self, parent: Union[Gtk.Widget, 'Layer']):
        pass

    # draw to cached surface or full redraw depending on self.do_draw_layer 
    def draw(self, parent_context:cairo.Context):
        """
        full draw
        """
        if self.do_create_surface:
            self.create_surface()

        if self.do_draw_layer:
            self.clear_surface()
            self.draw_layer()
            self.do_draw_layer = False
            self.painted = True

        if self.painted:
            self.copy_layer(parent_context)

    def redraw(self, parent_context):
        if self.painted:
            self.redraw_layer()
            self.copy_layer(parent_context)

    def clear_surface(self):
        self.context.save()
        if self.is_transparent_bg:
            self.context.set_operator(cairo.Operator.SOURCE)
        
        self.context.set_source_rgba(*self.bg)
        self.context.rectangle(0, 0, self.size.x, self.size.y)
        self.context.fill()

        self.context.restore()

    def create_surface(self):
        self.do_create_surface = False
        self.surface = cairo.ImageSurface(cairo.Format.ARGB32, self.size.x, self.size.y)
        self.context = cairo.Context(self.surface)
        self.do_draw_layer = True

    def draw_layer(self):
        """
        draw to surface then copy_layer()
        """
        pass


    def redraw_layer():
        """
        optionally redraw surface then copy_layer()
        """
        pass

    def copy_layer(self, parent_context:cairo.Context):
        """
        copy surface to the parent context 
        """
        # don't check if self.surface, it's an error
        parent_context.set_source_surface(self.surface, self.pos.x, self.pos.y)
        parent_context.paint()
        

    def copy_region(self, parent_context:cairo.Context, area:Area):
        # copy a rectangle define by area  to parent_context
        parent_context.set_source_surface(self.surface, self.pos.x, self.pos.y)
        parent_context.rectangle(area.x, area.y, area.width, area.height)
        parent_context.fill()




# # draw plugin box 
# class MainPluginLayer(Layer):
#     def __init__(self, size, plugin_containers):
#         Layer.__init__(self, size, bg=(0.2,0.2,0.0))
#         self.plugin_containers:list['PluginContainer'] = plugin_containers

#     def draw_layer(self):
#         print("plugin_containers len: ", len(self.plugin_containers))
#         for id, plugin_container in enumerate(self.plugin_containers):
#             plugin_container.draw_area(self, AreaType.PLUGIN)

    # def draw_plugin(self, plugin_view: 'PluginView',id):
    #     print("draw_plugin", id)
    #     self.context.rectangle(plugin_view.screen_pos.x, plugin_view.screen_pos.y, plugin_view.size.x, plugin_view.size.y)
    #     self.context.set_source_rgb(0.2, 0.5, 0.5)
    #     self.context.fill_preserve()
    #     self.context.set_source_rgb(0, 0, 0)
    #     self.context.stroke()






# ## in the main layer the outlines, background and name are painted
# ## every redraw that layer is copied to this sublayer then overlayed with leds and pan drawn on top of main
# # this 
# class DynPluginLayer(Layer):
#     def __init__(self, size, main_layer, containers):
#         #list layer init
#         Layer.__init__(self, size=size)
#         self.containers = containers
#         self.main_layer = MainPluginLayer(containers)

#         self.set_bg(0,0,0)
#         self.items.append(self.main_layer)

#     def prepare(self, parent:'RouterLayers'):
#         self.pango = parent.pango

#     def draw_layer(self):
#         self.main_layer.draw()
#         self.main_layer.copy_layer(self.context)
#         self.draw_overlays(AreaType.PLUGIN_OVERLAYS)

#     def redraw(self, parent_context):
#         self.main_layer.copy_layer(self.context)
#         self.draw_overlays(AreaType.PLUGIN_OVERLAYS)
#         self.copy_layer(parent_context)

#     # def update_overlay(self, parent_context, container, area_type):
#     #     self.draw_overlay(container, area_type)
#     #     self.main_layer.copy_layer(self.context)
#     #     self.copy_layer(parent_context)

#     def draw_overlays(self, area_type:Area):
#         for id, containers in enumerate(self.containers):
#             containers.draw_area(self, area_type)

#     def draw_overlay(self, container, area_type:Area):
#         container.draw_area(self, area_type)




# class ConnectionLayer(Layer):
#     def __init__(self, conn_views):
#         Layer.__init__(self)
#         self.set_bg(0.5, 0.5, 0.5, 1)
#         self.conn_views = conn_views



# ## in draw_layer the inner surface is cleared then plugin box draw then overlay drawn
# ## in redraw only the overlays are redrawn
# class PluginLayer(Layer):
#     def __init__(self, containers):
#         Layer.__init__(self,)
#         self.containers = containers

#         self.set_bg(0.1,0.2,0)

#     def prepare(self, parent:'RouterLayers'):
#         self.pango = parent.pango

#     def draw_layer(self):
#         self.draw_plugins()
#         self.draw_overlays(AreaType.PLUGIN_OVERLAYS)

#     def redraw(self, parent_context):
#         self.draw_overlays(AreaType.PLUGIN_OVERLAYS)
#         self.copy_layer(parent_context)

#     def draw_plugins(self):
#         for container in self.containers:
#             container.draw_area(self, AreaType.PLUGIN)

#     def draw_overlays(self, area_type:Area):
#         for container in self.containers:
#             container.draw_area(self, area_type)

#     def draw_overlay(self, container, area_type:Area):
#         container.draw_area(self, area_type)




class ContainersOverlayer:
    def __init__(self, layer, containers):
        """
        layer is parent layer drawn onto
        """
        self.layer: Layer = layer
        self.containers: list['Container'] = containers

    def draw(self):
        self.draw_items()
        self.draw_overlays()

    def redraw(self):
        self.draw_overlays()

    def draw_items(self):
        for container in self.containers:
            container.draw_item(self.layer)

    def draw_overlays(self):
        for container in self.containers:
            container.draw_overlays(self.layer)

    # def draw_overlay(self, container, area_type:Area):
    #     container.draw_area(self.layer, area_type)






class RouterLayers(Layer):
    def __init__(self, sizes):
        Layer.__init__(self)
        self.pango:Pango.Context = None

        # fake screen size, scales the plugins internal coords of (-1 to +1) into (-500 to 500)
        self.fake_size = 1000
        
        # (0.5,0.5) is center point but it needs to be shifted by half the plugin size
        # to make the master plugin look centered on startup
        self.nudge_mid = Vec2(0.5, 0.5) 

        # shift master midpoint to the left by half plugin size
        self.init_nudge = sizes['plugin'] / (self.fake_size * 2)

        # zoom scales the position not the size
        # counter nudge master midpoint slightly to the right in the first calls to resized() 
        self.init_nudge_fixzoom = Vec2(1,1)

        self.containers: dict[AreaType,Container] = {
            AreaType.PLUGIN: [],
            AreaType.CONNECTION: []
        }

        self.plugin_layer = ContainersOverlayer(self, self.containers[AreaType.PLUGIN])
        self.conn_layer = ContainersOverlayer(self, self.containers[AreaType.CONNECTION])
        self.clickareas = ClickAreas()
        self.set_zoom(1, 1)

    def prepare(self, parent:Gtk.Widget):
        self.pango = parent.get_pango_context()

    def resized(self, size:Vec2, prev_size: Vec2):
        if not self.painted:
            ratio = size / self.fake_size
            self.init_nudge_fixzoom = self.init_nudge / ratio
            self.nudge_mid = Vec2(0.5, 0.5) - self.init_nudge_fixzoom

            self.set_zoom(ratio.x, ratio.y)

    def norm_pos_to_pixel(self, pos):
        return (
            (pos.x + self.nudge_mid.x) * self.fake_size * self.zoom_level.x,
            (pos.y + self.nudge_mid.y) * self.fake_size * self.zoom_level.y
        )
    
    def pixel_to_norm_pos(self, xy):
        pass
 
    def scroll(self, dx, dy):
        self.nudge_mid += Vec2(dx/self.fake_size, dy/self.fake_size)

    def get_containers(self) -> Generator['Container', None, None]:
        for key in self.containers:
            for container in self.containers[key]:
                yield container


    def set_zoom(self, zoom_x, zoom_y):
        self.zoom_level = Vec2(zoom_x, zoom_y)
        for container in self.get_containers():
            container.set_screen_pos(*self.norm_pos_to_pixel(container.norm_pos))

        self.set_refresh()


    def draw_layer(self):
        """
        draw internally
        """
        self.conn_layer.draw()
        self.plugin_layer.draw()


    def redraw_layer(self):
        self.conn_layer.redraw()
        self.plugin_layer.redraw()

    def find_container(self, id, area_type):
        return next((cont for cont in self.containers[area_type] if cont.id == id), None)
    
    def add_container(self, cont: 'Container'):
        if cont.type in self.containers and not self.find_container(cont.id, cont.type):
            self.containers[cont.type].append(cont)
            cont.add_click_areas(self.clickareas)
            cont.set_screen_pos(*self.norm_pos_to_pixel(cont.norm_pos))

    def remove_container(self, id, area_type):
        container = self.find_container(id, area_type)
        if container:
            self.containers[area_type].remove(container)
            container.remove_click_areas(self.clickareas)

    def clear(self):
        self.clickareas.clear()
        for containers in self.containers.values():
            containers.clear()

    def get_object_at(self, x, y):
        return self.clickareas.get_object_at(x,y)
    
    # group_
    def get_object_group_at(self, x, y, group_type):
        return self.clickareas.get_object_group_at(x,y, group_type)




class SurfaceView:
    def draw(self, layer, container):
        surface = self.get_gfx_surface(container)
        layer.context.set_source_surface(surface, container.pos.x, container.pos.y)
        layer.context.paint()

    def get_gfx(self, container) -> PluginGfx:
        pass

    def get_gfx_surface(self, container):
        gfx = self.get_gfx(container)

        if gfx.surface:
            return gfx.surface
        else:
            gfx.surface = self.create_surface(container)
            gfx.context = cairo.Context(gfx.surface)
            self.draw_surface(container, gfx.context)

            return gfx.surface
    
    def create_surface(self, container):
        return cairo.ImageSurface(cairo.Format.ARGB32, int(container.size.x), int(container.size.y))

    def draw_view(self, layer, container):
        pass



# draws the outline, background and name of the plugin
class PluginGfx(SurfaceView):
    def get_gfx(self, container):
        return container.info.plugingfx

    def draw_view(self, layer, container):
        print("draw plugin  ({}) ({})".format(container.pos, container.size))
        layer.context.set_source_rgb(*container.colors.background())
        layer.context.rectangle(container.pos.x, container.pos.y, container.size.x, container.size.y)
        layer.context.fill()


# overlay for the plugin and connection layers
class Overlay:
    def draw_overlay(self, layer:Layer, container):
        pass



class LedOverlay(Overlay):
    def draw_overlay(self, layer:Layer, container:'PluginContainer'):
        pass



class PanOverlay(Overlay):
    def draw_overlay(self, layer:Layer, container:'PluginContainer'):
        pass



class CpuOverlay(Overlay):
    def draw_overlay(self, layer:Layer, container:'PluginContainer'):
        pass



# class OverlaysView:
#     def __init__(self, overlays:dict):
#         self.overlays = {}
#         self.order = []

#         for area_type, view in overlays.items():
#             self.add_overlay(area_type, view)

#     def add_overlay(self, area_type:AreaType, view:Overlay):
#         self.overlays[area_type] = view
#         self.order = sorted(self.overlays.keys())

#     def draw_overlays(self, layer:Layer, container:'PluginContainer'):
#         for area_type in self.order:
#             self.overlays[area_type].draw_overlay(layer, container)

#     def draw_overlay(self, layer:Layer, container:'PluginContainer', area_type: AreaType):
#         if area_type in self.overlays:
#             self.overlays[area_type].draw_overlay(layer, container)


# every plugin and connection has a container. it stores pixel positions and colors and
# some helper methods used by ContainersOverlayer to draw the plugin/connection 
# the containers and the positions are stored and maintained in RouterLayers
class Container:
    def __init__(self, id: int, type: AreaType, 
                 pos: Vec2, sizes:Sizes, colors: RouterColors):
        
        self.type = type
        self.id = id
        self.pos = Vec2(pos.x, pos.y)      # used as screen_pos
        self.norm_pos = pos   # normalised position initially betwen -1 and 1 but unlimited
        self.colors = colors

        self.setup_sizes(sizes)
        self.setup_views()
        
    def setup_sizes(self, sizes):
        pass

    def setup_views():
        pass

    # called when screen zoomed, zoomed, the widget is moved
    def set_screen_pos(self, x, y):
        self.pos.move_to(x, y)

    # called when widget moved - just before set_screen_pos is called
    def set_norm_pos(self, x, y):
        self.norm_pos.move_to(x, y)

    def add_click_areas(self, clickareas: ClickAreas):
        pass
    
    def remove_click_areas(self, clickareas: ClickAreas):
        pass

    def draw_overlays(self, layer: Layer):
        pass

    def draw_overlay(self, layer: Layer, area_type:AreaType):
        pass

    def draw_item(self, layer: Layer):
        pass



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

    def get_plugin_overlays(self) -> dict[AreaType, Overlay]:
        return {
            AreaType.PLUGIN_LED: LedOverlay(),
            AreaType.PLUGIN_PAN: PanOverlay(),
            AreaType.PLUGIN_CPU: CpuOverlay(),
        }
    
    def draw_overlays(self, layer: Layer):
        for overlay in self.overlays.values():
            overlay.draw_overlay(layer, self)

    def draw_overlay(self, layer: Layer, area_type:AreaType):
        if area_type in self.overlays:
            self.overlays[area_type].draw_overlay(layer, self)

    def draw_item(self, layer: Layer):
        self.plugin_display.draw_view(layer, self)

    def add_click_areas(self, clickareas: ClickAreas):
        clickareas.add_object(self, self.led_area, AreaType.PLUGIN_CPU)
        clickareas.add_object(self, self.led_area, AreaType.PLUGIN_LED)
        clickareas.add_object(self, self.pan_area, AreaType.PLUGIN_PAN)
        clickareas.add_object(self, self.plugin_area, AreaType.PLUGIN)

    def remove_click_areas(self, clickareas: ClickAreas):
        clickareas.remove_object(self)


