from .area_type import AreaType

from .click_area import ClickArea
from .colors import RouterColors
from .layer import Layer

from neil.utils import Vec2, Sizes

# every plugin and connection has a container. it stores pixel positions and colors and
# some helper methods used by ContainersOverlayer to draw the plugin/connection 
# the containers and the positions are stored and maintained in RouterLayers
class Container:
    def __init__(self, id: int, type: int, 
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

    def add_click_areas(self, clickareas: ClickArea):
        pass
    
    def remove_click_areas(self, clickareas: ClickArea):
        pass

    def draw_overlays(self, layer: Layer):
        pass

    def draw_overlay(self, layer: Layer, area_type:AreaType):
        pass

    def draw_item(self, layer: Layer):
        pass



