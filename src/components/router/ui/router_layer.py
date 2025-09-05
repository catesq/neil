import cairo
import gi

gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Pango
from typing import Generator, Tuple
from neil.utils import Vec2, Area, Colors
import neil.common as common
from typing import cast

from . import click_area, items, layers, area_type

import zzub



# used to typecast from ClickedArea 
class RouterPlugin:
    def __init__(self, id, plugin: zzub.Plugin, rect: Area, area_type):
        self.id = id
        self.object = plugin
        self.rect = rect
        self.type = area_type

    def __repr__(self):
        return f"SelectedPlugin({self.id} {self.object}, {self.rect}, {self.type})"


# used to typecast from ClickedArea 
class RouterConnection:
    def __init__(self, id, connection: zzub.Connection, rect: Area, area_type):
        self.id = id
        self.object = connection
        self.rect = rect
        self.type = area_type

    def __repr__(self):
        return f"SelectedConnection({self.id} {self.object}, {self.rect}, {self.type})"
    




class RouterItems(layers.Layer):
    pass


# class RouterLayer(layers.Layer):

# This class does a lot of work for the RouteView
# It maintains a list of Plugin and Connection items and
# informs them all of changes of canvas size to all items
# and has an offscreen buffer with all the plugins/connections predrawn
# so most of the time only the led/pan overlays (in PluginItem) need to be redrawn

# The plugin and connection items are precalculated
# screen position/sizes with draw_itm and draw_overlay functions



class RouterLayer():
    pango: Pango.Context

    def __init__(self, sizes, colors: Colors):
        # self.pango:Pango.Context = None

        self.sizes = sizes
        self.colors = colors

        self.zoom_level = Vec2(1, 1)

        # precalculated zoom_level * size
        self.size:Vec2 = Vec2(0)
        self.precalc_scale  = Vec2(1, 1)
        self.half_size = Vec2(1,1)

        self.center_pos = Vec2(0,0)

        self.items: dict[area_type.AreaType, list[items.Item]] = {
            area_type.AreaType.PLUGIN: [],
            area_type.AreaType.CONNECTION: [],
        }

        # temporary highlights are added and removed here
        self.extra_items = []

        # temporary highight for dragging connection
        self.drag_connection = None

        # first draw to this layer
        self.cache_layer = layers.Layer(bg = (0.5,0.5,0.5,0))

        self.clickareas = click_area.ClickArea()
        # self.set_zoom(self.zoom_level)
        self.full_refresh = False


    def set_parent(self, parent:Gtk.Widget):
        self.pango = parent.get_pango_context()


    

    def to_normal_pos(self, pos: Vec2) -> Vec2:
        """
        scale a pixel position to normal range -1.0 to +1.0

        @param pos: Vec2 coordinate in pixels
        @return: Vec2

        """
        return Vec2(
            (pos.x - self.half_size.x) / self.precalc_scale.x,
            (pos.y - self.half_size.y) / self.precalc_scale.y
        )


    def to_screen_pos(self, pos: Vec2) -> Vec2:
        """
        expand from normal range(-1.0 to +1.0) to pixel position

        @param pos: Vec2 coordinate normal range -1.0 to +1.0
        @return: Vec2
        """
        return Vec2(
            int(pos.x * self.precalc_scale.x +  self.half_size.x),
            int(pos.y * self.precalc_scale.y +  self.half_size.y)
        )


    def to_normal_pos_pair(self, x, y) -> Tuple[float,float]:
        """
        scale a pixel position to normal range -1.0 to +1.0 

        @param x: x coordinate in pixels
        @param y: y coordinate in pixels
        @return: (float, float) from -1 to +1
        """
        return (
            (x - self.half_size.x) / self.precalc_scale.x,
            (y - self.half_size.y) / self.precalc_scale.y
        )


    def to_screen_pos_pair(self, x, y) -> Tuple[int,int]:
        """
        expand from notmal range to pixel position

        @param x: normalized x coordinate from -1 to +1
        @param y: normalized y coordinate from -1 to +1
        @return: (int, int) in pixels
        """
        return (
            int(x * self.precalc_scale.x +  self.half_size.x),
            int(y * self.precalc_scale.y +  self.half_size.y)
        )


    def move_plugin(self, plugin, norm_pos:Vec2):
        for item in self.get_items():
            if item.uses_plugin(plugin):
                item.plugin_moved(plugin, norm_pos)


    def add_drag_connection(self, x, y, plugin):
        plugin_item = self.find_plugin(plugin.get_id())

        if plugin_item is None:
            return
        
        item = items.DragConnectionItem(plugin_item, Vec2(x, y), self.colors)
        self.drag_connection = item
        self.extra_items.append(item)


    def update_drag_connection(self, x, y, ):
        if not self.drag_connection:
            return

        hover_plugins = [found for found in self.clickareas.get_all_type_at(x,y, area_type.AreaType.PLUGIN) 
                   if found.type == area_type.AreaType.PLUGIN]
        
        if len(hover_plugins) == 0:
            self.drag_connection.set_target_pos(x, y)
        elif len(hover_plugins) == 1 and hover_plugins[0].object.get_id() == self.drag_connection.source_id:
            self.drag_connection.set_target_invalid()
        else:
            #remove plugins matching source_id
            conn_src_id = self.drag_connection.source_id
            hover_plugins = [found for found in hover_plugins if found.object.get_id() != conn_src_id]
            target_item = self.get_plugin_by_id(hover_plugins[-1].object.get_id())
            self.drag_connection.set_target_item(target_item)


    def remove_drag_connection(self):
        if self.drag_connection:
            self.extra_items.remove(self.drag_connection)
            self.drag_connection = None


    def get_zoom(self):
        return self.zoom_level


    def set_size(self, size):
        self.size = size
        self.half_size = size / 2
        self.precalc_scale = self.half_size * self.zoom_level
        self.cache_layer.set_size(size)
        # self.main_layer.set_size(size)
        
        # has the main layer ever been drawn
        if self.cache_layer.copied:
            for item in self.get_items():
                item.set_canvas_size(size)
        else:
            for item in self.get_items():
                item.init_canvas_size(size)


    def set_scroll_offset(self, offset: Vec2):
        self.center_pos += offset

        for item in self.get_items():
            item.scroll_to(self.center_pos)

        self.set_refresh()

    # 
    def set_zoom(self, zoom):
        self.zoom_level.set(zoom.x, zoom.y)

        for item in self.get_items():
            item.set_zoom(self.zoom_level)

        self.set_refresh()

    # redraw connections + plugins
    def set_refresh(self):
        self.full_refresh=True

    # clear screen, draw connections and plugins gfx to buffer (if redraw) then copy buffer to screen
    def draw_router(self, context: cairo.Context):
        """
        draw internally
        """
        context.save()
        context.set_operator(cairo.Operator.SOURCE)
        context.set_source_rgba(*self.colors.router.background())
        context.rectangle(0, 0, self.size.x, self.size.y)
        context.fill()
        context.set_operator(cairo.Operator.OVER)

        if self.full_refresh:
            self.cache_layer.clear_surface()

            for item in self.items[area_type.AreaType.CONNECTION]:
                item.draw_item(self.cache_layer.context)

            for item in self.items[area_type.AreaType.PLUGIN]:
                item.draw_item(self.cache_layer.context)
        
        self.cache_layer.copy_to_context(context)

        for item in self.extra_items:
            item.draw_item(context)
            
        # context.set_operator()
        # self.cache_layer.copy_to_context(context)
        context.restore()


    # in theory get_plugin_item_class and get_connection_item_class will be configurable
    # with more advanced plugin/connection display gfx
    def get_plugin_item_class(self, metaplugin):
        return items.PluginItem


    def get_connection_item_class(self, connection, target_plugin, source_plugin):
        return items.ConnectionItem





    # add plugin then connection items
    def populate(self, player: zzub.Player):
        self.clear()

        for plugin in player.get_plugin_list():
            self.add_plugin_item(plugin)

        # both source and target plugin items need to be added before connection items
        for plugin_item in self.get_plugins():
            self.add_connection_items(plugin_item)


    # build a PluginItem and store it in self.items
    def add_plugin_item(self, metaplugin: zzub.Plugin):
        info = common.get_plugin_infos().get(metaplugin)
        plugin_item_cls = self.get_plugin_item_class(metaplugin)
        plugin_item = plugin_item_cls(metaplugin, info, self.colors)
            #, pos, router_sizes, self.colors
        self.add_item(plugin_item, area_type.AreaType.PLUGIN)
        

    # build all ConnectionItem's for a plugin
    # both source and target plugins need a PluginItem before making connections
    def add_connection_items(self, plugin_item):
        metaplugin = plugin_item.metaplugin

        for index in range(metaplugin.get_input_connection_count()):
            connection = metaplugin.get_input_connection(index)
            source_plugin = metaplugin.get_input_connection_plugin(index)
            source_item = self.find_plugin(source_plugin.get_id())

            if not source_item:
                continue

            conn_item_cls = self.get_connection_item_class(connection, metaplugin, source_plugin)
            conn_item = conn_item_cls(index, connection, source_item, plugin_item, self.colors)

            self.add_item(conn_item, area_type.AreaType.CONNECTION)


    # add plugin and connection items then intialise dimensions and clickable sections
    def add_item(self, item: 'items.Item', item_type):
        """
        """
        if not self.find_by_id(item.id, item_type):
            item.init_dimensions(self.sizes)
            self.items[item_type].append(item)
            item.add_click_areas(self.clickareas)
            if not self.size.is_zero():
                item.init_canvas_size(self.size)


    def remove_item(self, id, area_type):
        """
        Remove a PluginItem or ConnectionItem from self.items

        @param id         int | items.ConnID
        @param area_type  AreaType.PLUGIN | AreaType.CONNECTION
        @return           items.Item or None
        """
        item = self.find_by_id(id, area_type)
        if item:
            self.items[area_type].remove(item)
            item.remove_click_areas(self.clickareas)


    def clear(self):
        self.clickareas.clear()
        for items in self.items.values():
            items.clear()

    # the items being found here are the view classes, items.PluginItems and items.ConnectionItems
    # all the coordinates and colours and draw functions used for rendering connections and plugins on the router 

    def get_items(self) -> Generator['items.Item', None, None]:
        for item in self.items[area_type.AreaType.PLUGIN]:
            yield item

        for item in self.items[area_type.AreaType.CONNECTION]:
            yield item

        for item in self.extra_items:
            yield item


    def get_plugins(self) -> Generator['items.PluginItem', None, None]:
        for item in self.items[area_type.AreaType.PLUGIN]:
            yield cast(RouterPlugin, item) # pyright: ignore[reportReturnType]

    def get_connections(self) -> Generator['items.ConnectionItem', None, None]:
        for item in self.items[area_type.AreaType.CONNECTION]:
            yield cast(RouterConnection, item) # pyright: ignore[reportReturnType]

    def get_plugin_by_id(self, id):
        for item in self.items[area_type.AreaType.PLUGIN]:
            if item.id == id:
                return cast(RouterPlugin, item)


    def find_by_id(self, id, area_type):
        """
        Find a PluginItem or ConnectionItem in self.items

        @param id         int | items.ConnID
        @param area_type  AreaType.PLUGIN | AreaType.CONNECTION
        @return           items.Item or None
        """
        return next((item for item in self.items[area_type] if item.id == id), None)

    def find_plugin(self, id) -> items.PluginItem | None:
        """
        Find a PluginItem in self.items by its id.
 
        @param id: int
        @return: items.PluginItem or None
        """
        return self.find_by_id(id, area_type.AreaType.PLUGIN) # pyright: ignore[reportReturnType]

    def find_connection(self, id) -> items.ConnectionItem | None:
        """
        Find a ConnectionItem in self.items by its id.
 
        @param id: items.ConnID
        @return: items.ConnectionItem or None
        """
        return self.find_by_id(id, area_type.AreaType.CONNECTION) # pyright: ignore[reportReturnType]


    def find_by_attr(self, match_val, attr, area_type):
        """
        Find a PluginItem or ConnectionItem in one of self.items

        @param match_val:Any  Probably a zzub.Plugin or a zzub.Connection 
        @param attr:str       Probaly 'metaplugin' or 'connection'
        @param area_type      AreaType.PLUGIN | AreaType.CONNECTION
        @return:              items.Item or None
        """
        return next((item for item in self.items[area_type] if getattr(item, attr) == match_val), None)
    

    # the items found below are the outline/area of the rendered objects, used to determine what has been clicked
    # the connection can only be clicked on the small volume adjust button in the center point of the connection
    # the plugins can be clicked on the plugin_led, the plugin_pan bar and anywhere else


    def get_item_locator(self) -> click_area.ClickArea:
        return self.clickareas


    def get_plugins_at(self, x, y) -> Generator['RouterPlugin', None, None]:
        for item in self.clickareas.get_all_type_at(x, y, area_type.AreaType.PLUGIN):
            yield cast(RouterPlugin, item) # pyright: ignore[reportReturnType]


    def get_connections_at(self, x, y) -> Generator['RouterConnection', None, None]:
        for item in self.clickareas.get_all_type_at(x, y, area_type.AreaType.CONNECTION):
            if item.type == area_type.AreaType.CONNECTION_ARROW:
                yield cast(RouterConnection, item) # pyright: ignore[reportReturnType]


    

