import gi

gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Pango
from typing import Generator
from neil.utils import Vec2

from .area_type import AreaType

from .click_area import ClickArea
from .layer import Layer
# from .items import Item

from . import items
import zzub


#used in router_layer to process the list of PluginItem
class Overlayer:
    def __init__(self, layer, items):
        """
        layer is parent layer drawn onto
        """
        self.layer: Layer = layer
        self.items: list['Item'] = items

    def draw(self):
        self.draw_items()
        self.draw_overlays()

    def redraw(self):
        self.draw_overlays()

    def draw_items(self):
        for item in self.items:
            item.draw_item(self.layer)

    def draw_overlays(self):
        for item in self.items:
            item.draw_overlays(self.layer)



class RouterLayer(Layer):
    def __init__(self, sizes):
        Layer.__init__(self)
        self.pango:Pango.Context = None

        self.sizes = sizes

        self.zoom_level = Vec2(1, 1)
        
        self.center_pos = (0,0)

        self.items: dict[AreaType, items.Item] = {
            AreaType.PLUGIN: [],
            AreaType.CONNECTION: []
        }
        
        self.plugins_gfx = Overlayer(self, self.items[AreaType.PLUGIN])
        self.connects_gfx = Overlayer(self, self.items[AreaType.CONNECTION])

        self.clickareas = ClickArea()
        # self.set_zoom(self.zoom_level)

    def prepare(self, parent:Gtk.Widget):
        self.pango = parent.get_pango_context()

    def normalise_screen_pos(self, screen_pos) -> Vec2:
        norm_screen_pos = (screen_pos - self.size / 2) / self.size
        return norm_screen_pos / self.zoom_level 
        

    def resized(self, size):
        if not self.painted:
            for item in self.get_items():
                item.set_canvas_size(size)


    def scroll(self, offset: Vec2):
        self.center_pos += offset

        for item in self.get_items():
            item.scroll_to(self.center_pos)

        self.set_refresh()


    def get_items(self) -> Generator['items.Item', None, None]:
        for item in self.items[AreaType.PLUGIN]:
            yield item

        for item in self.items[AreaType.CONNECTION]:
            yield item


    def get_plugins(self) -> list['items.PluginItem']:
        return self.items[AreaType.PLUGIN]


    def get_connections(self) -> list['items.ConnectionItem']:
        return self.items[AreaType.CONNECTION]
        

    def set_zoom(self, zoom):
        self.zoom_level.set(zoom.x, zoom.y)

        for item in self.get_items():
            item.set_zoom(self.zoom_level)

        self.set_refresh()


    def draw_layer(self):
        """
        draw internally
        """
        self.connects_gfx.draw()
        self.plugins_gfx.draw()


    def redraw_layer(self):
        self.connects_gfx.redraw()
        self.plugins_gfx.redraw()


    def find_by_id(self, id, area_type):
        return next((item for item in self.items[area_type] if item.id == id), None)


    def find_by_attr(self, match_itm, attr, area_type):
        return next((item for item in self.items[area_type] if getattr(item, attr) == match_itm), None)
    

    def add_item(self, item: 'items.Item'):
        if item.type in self.items and not self.find_by_id(item.id, item.type):
            item.init_dimensions(self.sizes)
            self.items[item.type].append(item)
            item.add_click_areas(self.clickareas)
            if self.size is not None:
                item.set_canvas_size(self.size)


    def remove_item(self, id, area_type):
        item = self.find_item(id, area_type)
        if item:
            self.items[area_type].remove(item)
            item.remove_click_areas(self.clickareas)


    def clear(self):
        self.clickareas.clear()
        for items in self.items.values():
            items.clear()


    def get_object_at(self, x, y):
        return self.clickareas.get_object_at(x,y)
    

    # 
    def get_plugin_at(self, x, y):
        clicked = self.clickareas.get_object_group_at(x,y, AreaType.PLUGIN)
        return clicked.object if clicked else False
    

    def get_plugin_item(self, plugin_or_id: int|zzub.Plugin):
        if isinstance(plugin_or_id, int):
            return self.find_by_id(plugin_or_id, AreaType.PLUGIN)
        else:
            return self.find_by_attr(plugin_or_id, 'metaplugin', AreaType.PLUGIN)
    

    def get_connection_item(self, conn_or_conn_id: items.ConnId|zzub.Connection):
        if isinstance(conn_or_conn_id, int):
            return self.find_by_id(conn_or_conn_id, AreaType.CONNECTION)
        else:
            return self.find_by_attr(conn_or_conn_id, 'connection', AreaType.CONNECTION)
    

    # only look for audio and cv_connections
    def get_connections_at(self, x, y):
        pass


    # can check
    def get_connection_type_at(self, x, y, conn_type):
        pass





# overlay for the plugin and connection layers


