from neil.common import GfxCache, PluginInfo
from neil.utils import prepstr, is_a_generator, RouterColors
import cairo
from . import layer, items


class DisplayGfx:
    def draw_gfx(self, layer, item):
        pass


class CachedGfx(DisplayGfx):
    def draw_gfx(self, layer: 'layer.Layer', item: 'items.Item'):
        print("draw cache", item.id, " to ", item.pos)
        surface = self.get_cached_surface(layer, item)
        layer.context.set_source_surface(surface, item.pos.x, item.pos.y)
        layer.context.paint()

    def draw_cache(self, context, item):
        pass

    def get_cache(self, item: 'items.PluginItem') -> GfxCache:
        pass

    def get_cached_surface(self, layer, item) -> cairo.Surface:
        cache = self.get_cache(item)

        if cache.surface:
            return cache.surface
        else:
            cache.surface = self.create_surface(item)
            cache.context = cairo.Context(cache.surface)
            self.draw_cache(cache.context, item)

            return cache.surface
    
    def create_surface(self, item):
        return cairo.ImageSurface(cairo.Format.ARGB32, int(item.size.x), int(item.size.y))




# draws the outline, background and name of the plugin onto the router
class PluginGfx(CachedGfx):
    def __init__(self, colors: RouterColors):
        self.colors: RouterColors = colors

    def draw_cache(self, context: cairo.Context, item: 'items.PluginItem'):
        print("draw plugin item: ", item.id, item.pos, item.size)
        self.draw_bg(context, item)
        self.draw_outline(context, item)
        self.draw_name(context, item)

    def get_cache(self, item: 'items.PluginItem'):
        return item.info.plugingfx

    def draw_bg(self, context: cairo.Context, item: 'items.PluginItem'):
        plugin_bg = self.colors.plugin(item.info.type, item.info.selected, item.info.muted)
        context.set_source_rgb(*plugin_bg)
        context.rectangle(0, 0, item.size.x, item.size.y)
        context.fill()

    def draw_outline(self, context: cairo.Context, item: 'items.PluginItem'):
        border = self.colors.plugin_border(item.info.type, item.info.selected)
        context.set_source_rgb(*border)
        context.rectangle(1,1, item.size.x-2, item.size.y -2)
        context.stroke()

    def draw_name(self, context: cairo.Context, item: 'items.PluginItem'):
        context.set_source_rgb(*self.colors.text())
        context.select_font_face('Sans', cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_NORMAL)
        context.set_font_size(12)
        context.move_to(5, 15)
        context.show_text(self.get_name(item.info))
        

    
    def get_name(self, info):
        if (info.solo_plugin and is_a_generator(info.flags)):
            return prepstr('[' + info.plugin.get_name() + ']')
        elif info.muted or info.bypassed:
            return prepstr('(' + info.plugin.get_name() + ')')
        else:
            return prepstr(info.plugin.get_name())



class ConnectionGfx(DisplayGfx):
    def __init__(self, colors: RouterColors):
        self.colors: RouterColors = colors
    
    def draw_gfx(self, layer: 'layer.Layer', item: 'items.ConnectionItem'):
        print("draw line from ", item.source_pos, " to ", item.target_pos)
        #draw a line from source_pos to target_pos on item
        layer.context.set_source_rgb(1,1,1)
        layer.context.set_line_width(1)
        layer.context.move_to(*item.source_pos)
        layer.context.line_to(*item.target_pos)
        layer.context.stroke()

