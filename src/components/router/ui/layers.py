from typing import Union
import weakref
from neil.utils import Vec2, Area
import cairo
# each layer draws to it's internal surface which is later painted 
# a parent's layer Cairo.Context in copy_to_context
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk


"""
A basic hold for a surface and context used as a buffer
"""
class Layer:
    def __init__(self, pos:Vec2 = None, size:Vec2=None, bg=(0,0,0)):
        self.pos = pos if pos is not None else Vec2(0,0)
        self.size: Vec2 = size
        self.surface: cairo.Surface = None
        self.context: cairo.Context = None
        self.copied = False               # 
        # self.do_refresh = False
        self.is_transparent_bg = False

        self.set_bg(*bg)
        
        
    def set_bg(self, *args):
        """
        Sets the background color of the layer group.

        @param color: bg color, tuple or list with 3 or 4 floats (r,g,b) or (r,g,b,a) 
        """
        self.bg = [max(0, min(1, float(arg))) for arg in args]
        self.is_transparent_bg = (len(args) == 4)


    def set_size(self, size):
        if not self.size or self.size != size:
            self.size = Vec2(size)
            self.create_surface()
            self.clear_surface()


    def clear_surface(self):
        if not self.size:
            return
        
        if not self.surface:
            self.create_surface()
            
        self.context.save()

        if self.is_transparent_bg:
            self.context.set_operator(cairo.Operator.SOURCE)

        self.context.set_source_rgba(*self.bg)
        self.context.rectangle(0, 0, self.size.x, self.size.y)
        self.context.fill()

        self.context.restore()


    def create_surface(self):
        self.surface = cairo.ImageSurface(cairo.Format.ARGB32, self.size.x, self.size.y)
        self.context = cairo.Context(self.surface)
        # self.do_refresh = True


    def copy_to_context(self, parent_context:cairo.Context):
        """
        copy surface to the parent context 
        """
        if self.surface:
            parent_context.set_source_surface(self.surface, self.pos.x, self.pos.y)
            parent_context.paint()
            self.copied = True

