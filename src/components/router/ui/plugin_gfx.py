from neil.common import GfxCache
import cairo
from . import containers, layer

class DisplayGfx:
    def draw_layer(self, layer: layer.Layer, container: 'containers.PluginContainer'):
        surface = self.get_gfx_surface(container)
        layer.context.set_source_surface(surface, container.pos.x, container.pos.y)
        layer.context.paint()

    def get_cache(self, container) -> GfxCache:
        pass

    def get_gfx_surface(self, container):
        cache = self.get_cache(container)

        if cache.surface:
            return cache.surface
        else:
            cache.surface = self.create_surface(container)
            cache.context = cairo.Context(cache.surface)
            self.draw_cache(cache, container)

            return cache.surface
    
    def create_surface(self, container):
        return cairo.ImageSurface(cairo.Format.ARGB32, int(container.size.x), int(container.size.y))

    def draw_cache(self, cache: GfxCache, container):
        pass


# draws the outline, background and name of the plugin onto the router
class PluginGfx(DisplayGfx):
    def get_cache(self, container: 'containers.PluginContainer'):
        return container.info.plugingfx

    def draw_cache(self, cache: GfxCache, container: 'containers.PluginContainer'):
        print("draw plugin  ({}) ({})".format(container.pos, container.size))
        cache.context.set_source_rgb(*container.colors.background())
        cache.context.rectangle(0, 0, container.size.x, container.size.y)
        cache.context.fill()
