from typing import Union
import weakref
from neil.utils import Vec2, Area
import cairo
# each layer draws to it's internal surface which is later painted 
# a parent's layer Cairo.Context in copy_layer
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

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


