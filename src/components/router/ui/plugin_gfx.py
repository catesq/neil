from neil.common import GfxCache, PluginInfo
from neil.utils import prepstr, is_a_generator, RouterColors
import cairo
from . import layers, items


class DisplayGfx:
    def draw_gfx(self, context: cairo.Context):
        pass


class CachedGfx(DisplayGfx):
    def draw_gfx(self, context: cairo.Context):
        cached = self.get_cached_gfx()
        context.set_source_surface(cached.surface, self.item.pos.x, self.item.pos.y)
        context.paint()

    def draw_cache(self, context):
        pass

    def get_cache(self) -> GfxCache:
        pass

    def get_cached_gfx(self) -> GfxCache:
        cache = self.get_cache()

        if cache.surface:
            return cache
        else:
            cache.surface = self.create_surface()
            cache.context = cairo.Context(cache.surface)
            self.draw_cache(cache.context)

            return cache

    def create_surface(self):
        return cairo.ImageSurface(cairo.Format.ARGB32, int(self.item.size.x), int(self.item.size.y))




# draws the outline, background and name of the plugin onto the router
# needs: item.info: common.PluginInfo, item.size:Vec2, item.pos


class PluginGfx(CachedGfx):
    def __init__(self, item):
        self.item: 'items.PluginItem' = item
        self.colors: RouterColors = item.colors.router

    def draw_cache(self, context: cairo.Context):
        self.draw_bg(context)
        self.draw_outline(context)
        self.draw_name(context)

    def get_cache(self):
        return self.item.info.plugingfx

    def draw_bg(self, context: cairo.Context):
        item = self.item
        info = item.info
        plugin_bg = self.colors.plugin(info.type, info.selected, info.muted)
        context.set_source_rgb(*plugin_bg)
        context.rectangle(0, 0, item.size.x, item.size.y)
        context.fill()

    def draw_outline(self, context: cairo.Context):
        item = self.item
        info = item.info
        border = self.colors.plugin_border(info.type, info.selected)
        context.set_source_rgb(*border)
        context.rectangle(1,1, item.size.x-2, item.size.y -2)
        context.stroke()

    def draw_name(self, context: cairo.Context):
        context.set_source_rgb(*self.colors.text())
        context.select_font_face('Sans', cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
        context.set_font_size(12)
        context.move_to(5, 15)
        context.show_text(self.get_name(self.item.info))
        
    def get_name(self, info):
        if (info.solo_plugin and is_a_generator(info.flags)):
            return prepstr('[' + info.plugin.get_name() + ']')
        elif info.muted or info.bypassed:
            return prepstr('(' + info.plugin.get_name() + ')')
        else:
            return prepstr(info.plugin.get_name())



#needs: source_pos:Vec2 and target_pos:Vec2 on the item
class ConnectionGfx(DisplayGfx):
    def __init__(self, item: 'items.ConnectionItem'):
        self.item = item
        self.colors = item.colors.router
    
    def draw_gfx(self, context: cairo.Context):
        #draw a line from source_pos to target_pos on item
        context.set_source_rgb(1,1,1)
        context.set_line_width(1)
        context.move_to(*self.item.source_pos)
        context.line_to(*self.item.target_pos)
        context.stroke()



#needs: source_pos:Vec2 and target_pos:Vec2 on the item
class DragConnectionGfx(DisplayGfx):
    def __init__(self, item: 'items.ConnectionItem'):
        self.item = item
        self.colors = item.colors
    
    def draw_gfx(self, context: cairo.Context):
        #draw a line from source_pos to target_pos on item
        # print("drag draw source {} target{}".format(item.source_pos, item.drag_pos))
        context.set_source_rgb(0.5,0.5,0.5)
        context.set_line_width(1)
        context.move_to(*self.item.source_pos)
        context.line_to(*self.item.target_pos)
        context.stroke()

