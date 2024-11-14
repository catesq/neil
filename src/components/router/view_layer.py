import weakref
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Gdk, GObject, Pango, PangoCairo

import cairo
from neil.utils import ( 
    Sizes, Vec2, Area, OffsetArea
)
from enum import IntEnum


class AreaType(IntEnum):
    BACKGROUND = 0
    PLUGIN = 1
    PANNING = 2
    LED = 3
    ARROW = 4





# the layer is given a context to draws to in draw
# each layer has an internal srface which is painted to the parent context
# in the copy_to_parent()
class Layer:
    def __init__(self, bg):
        self.pos = Vec2(0,0)
        self.size: Vec2 = None
        self.widget: Gtk.Widget = None

        self.surface: cairo.Surface = None
        self.context: cairo.Context = None

        self.refresh_surface = True
        self.is_transparent_bg = False
        self.set_bg(bg)

        
    def set_bg(self, color):
        """
        Sets the background color of the layer group.

        @param color: bg color, tuple or list with 3 or 4 floats (r,g,b) or (r,g,b,a) 
        """
        self.bg = color
        self.is_transparent_bg = (len(color) == 4)

    def set_refresh(self, refresh=True):
        self.refresh_surface = refresh
            
    def get_new_size(self):
        return Vec2(
            self.widget.get_allocated_width(), 
            self.widget.get_allocated_height()
        )
    
    #  draw to surface
    def draw(self, widget, parent_context):
        """
        full draw
        """
        self.set_parent(widget, parent_context)

        if self.refresh_surface:
            self.prepare_surface()

        if self.redraw_layer:
            self.clear_surface()
            self.draw_layer()
            self.redraw_layer = False



    def redraw(self, parent_context):
        self.copy_layer(parent_context)
    
    def set_parent(self, widget, parent_context):
        self.widget = weakref.proxy(widget)
        # self.parent_context = weakref.proxy(parent_context)

    def prepare_surface(self):
        new_size = self.get_new_size()
        if not self.surface or not self.size or self.size != new_size:
            self.create_surface(new_size)
        self.redraw_layer = True
        self.refresh_surface = False

    def clear_surface(self):
        if self.is_transparent_bg:
            self.context.set_operator(cairo.Operator.SOURCE)
            
        self.context.set_source_rgba(*self.bg)
        self.context.rectangle(0, 0, self.size.x, self.size.y)
        self.context.fill()

        if self.is_transparent_bg:
            self.context.set_operator(cairo.Operator.OVER)
    
    def create_surface(self, size):
        self.size = size
        self.surface = cairo.ImageSurface(cairo.Format.ARGB32, self.size.x, self.size.y)
        self.context = cairo.Context(self.surface)

    def draw_layer(self):
        """
        draw to layer surface
        """
        pass

    def copy_layer(self, parent_context):
        """
        copy the layer surface to the parent surface 
        """
        parent_context.set_source_surface(self.surface, 0, 0)
        parent_context.paint()








# draw plugin box 
class MainPluginLayer(Layer):
    def __init__(self):
        Layer.__init__(self)



class PluginOverlay():
    def __init__(self,  plugin_views:list['PluginView']):
        Layer.__init__(self)
        self.plugin_views = weakref.proxy(plugin_views)

    def draw_overlay(self, layer:Layer):
        pass


class LedLayer(PluginOverlay):
    def draw_overlay(self, layer:Layer):
        pass



class PanLayer(PluginOverlay):
    def draw_overlay(self, layer:Layer):
        pass


## in draw_layer all plugin outlines, colored boxes and inner text are drawn to cache_surface 
## in render_surface cache drawn to main surface then leds and pan drawn on top of main
class PluginLayer(Layer):
    def __init__(self, plugin_views):
        #list layer init
        Layer.__init__(self)

        self.main_layer = MainPluginLayer(self, plugin_views)
        self.pan_layer = PanLayer(plugin_views)
        self.led_layer = LedLayer(plugin_views)

        self.set_bg(0.5, 0.5, 0.5, 1)

    def create_surface(self, size):
        self.size = size
        self.surface = cairo.ImageSurface(cairo.Format.ARGB32, self.size.x, self.size.y)
        self.context = cairo.Context(self.surface)

    def draw_layer(self):
        self.main_layer.draw()

    def copy_layer(self, parent_context):
        self.pan_layer.draw_overlay(self.main_layer)
        self.led_layer.draw_overlay(self.main_layer)
        # copy main layer to surface
        self.main_layer.copy_layer(self.context)

        parent_context.set_source_surface(self.surface, 0, 0)
        parent_context.paint()
        

    def update_leds(self):
        pass

    def update_pan_item(self, item):
        pass



class ConnectionLayer(Layer):
    def __init__(self, conn_views):
        Layer.__init__(self)
        self.set_bg(0.5, 0.5, 0.5, 1)
        self.conn_views = conn_views



class RouterLayers(Layer):
    def __init__(self):
        Layer.__init__(self)
        self.plugin_views = []
        self.connection_views = []
        self.plugin_layer = PluginLayer(self.plugin_views)
        self.conn_layer = ConnectionLayer(self.connection_views)

    def draw_layer(self):
        """
        draw internally
        """
        self.conn_layer.draw(self.widget, self.context)
        self.plugin_layer.draw(self.widget, self.context)

    def redraw(self):
        self.plugin_layer.redraw(self.context)
        self.conn_layer.redraw(self.context)

    def add_plugin(self, plugin_view: 'PluginView'):
        self.plugin_views.append(plugin_view)

    def add_connection(self, conn_view:'ConnView'):
        self.connection_views.append(conn_view)

    def update_leds(self):
        pass

    def copy_plugins_layer(self):
        pass


class PluginView:
    def __init__(self, x, y, sizes:Sizes):
        self.screen_pos = Vec2(x,y)
        self.size = sizes['plugin']
        self.led_size = sizes['led']
        self.margins = Vec2(sizes['gap'], sizes['gap'])
        self.pan_size = Vec2(sizes['panwidth'], sizes['panheight'])
        self.plugin_area = Area(self.screen_pos, self.size)
        self.led_area = OffsetArea(self.screen_pos, self.size - self.led_size - self.margins, self.led_size)
        self.pan_area = OffsetArea(self.screen_pos, Vec2(5, 20), Vec2(90, 5))
        

    # def register(self, clickboxes: 'ClickBoxes'):
    #     clickboxes.add_object(self, self.plugin_area, AreaType.PLUGIN)
    #     clickboxes.add_object(self, self.led_area, AreaType.LED)
    #     clickboxes.add_object(self, self.pan_area, AreaType.PANNING)
        

    # def unregister(self, clickboxes: 'ClickBoxes'):
    #     clickboxes.remove_object(self)



class ConnView:
    def __init__(self):
        pass



class ClickBoxes:
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

