import gi

gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Pango
from typing import Generator
from neil.utils import Vec2


from .ui import Layer, ClickArea, Container, AreaType




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
        self.clickareas = ClickArea()
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







# overlay for the plugin and connection layers


