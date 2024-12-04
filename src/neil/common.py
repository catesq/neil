
# Neil
# Modular Sequencer
# Copyright (C) 2006,2007,2008 The Neil Development Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

"""
Provides information used by all ui sections.
"""

from neil import components
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk
import zzub, cairo
from neil.utils import get_plugin_type, Vec2

PLUGIN_DRAG_TARGETS = [
    Gtk.TargetEntry.new('application/x-neil-plugin-uri', Gtk.TargetFlags.SAME_APP, 1)
]

class GfxCache:
    def __init__(self):
        self.surface:cairo.Surface = None
        self.context:cairo.Context = None

    def clear(self):
        self.__init__()



class PluginInfo(object):
    """
    Encapsulates data associated with a plugin.
    """
    def __init__(self, plugin):
        print("new PluginInfo for %s %s" % (plugin.get_id(), plugin.get_name()))
        self.plugin = plugin
        self.solo_plugin = False
        self.muted = False
        self.selected = False
        self.hovered = False
        self.bypassed = False
        self.cpu = -9999.0
        self.pattern_position = (0, 0, 0, 0, 0)
        self.selection = None
        self.songplugin = True
        self.plugingfx = GfxCache()
        self.patterngfx = {}
        self.gfx_cache = {}
        self.amp = -9999.0
        self.octave = 3
        self.type = get_plugin_type(self.plugin)

        # the normalized position from -1 to +1, maintained in router.drag_update
        # by offsetting from the current mouse dragpos then scaling/normalizing 
        self.dragpos = Vec2(0)
        # screen offset of the plugin from the click that started the drag 
        self.dragoffset = Vec2(0)

    def prepare_info(self):
        pass

    def reset_patterngfx(self):
        self.patterngfx = {}
    
    def reset_plugingfx(self):
        self.plugingfx.clear()
        self.amp = -9999.0
        self.cpu = -9999.0
        self.dragpos.clear()
        # screen offset of the plugin from the click that started the drag 
        self.dragoffset.clear()

    def __call__(self, name, *args):
        # if name begins with 'set_' and property with that suffix exists and at least one arg then assign property
        if name.startswith('set_') and hasattr(self, name[4:]) and len(args) > 0:
            setattr(self, name[4:], args[0])

        


class PluginInfoCollection:
    """
    Manages plugin infos.
    """
    def __init__(self):
        self.plugin_info = {}
        self.update()

    def reset(self):
        self.plugin_info = {}

    def __getitem__(self, k) -> PluginInfo:
        return self.plugin_info.__getitem__(k)

    def __delitem__(self, k):
        return self.plugin_info.__delitem__(k)

    def keys(self) -> list[zzub.Plugin]:
        return list(self.plugin_info.keys())

    def get(self, k) -> PluginInfo:
        if not k in self.plugin_info:
            self.add_plugin(k)
        return self.plugin_info.__getitem__(k)

    def items(self) ->list[PluginInfo]:
        return iter(self.plugin_info.items())

    def reset_plugingfx(self):
        for k,v in self.plugin_info.items():
            v.reset_plugingfx()

    def reset_plugin(self, mp):
        if mp in self.plugin_info:
            self.plugin_info[mp].reset_plugingfx()

    def add_plugin(self, mp):
        self.plugin_info[mp] = PluginInfo(mp)

    def update(self):
        previous = dict(self.plugin_info)
        self.plugin_info.clear()
        for mp in components.get('neil.core.player').get_plugin_list():
            if mp in previous:
                self.plugin_info[mp] = previous[mp]
            else:
                self.plugin_info[mp] = PluginInfo(mp)

collection = None
screen_size=Vec2(0,0)

def set_screen_size(x, y):
    global screen_size
    screen_size.set(x, y)

def get_screen_size():
    return Vec2(screen_size)

def get_plugin_infos() -> PluginInfoCollection:
    global collection
    if not collection:
        collection = PluginInfoCollection()
    return collection


if __name__ == '__main__':
    components.load_packages()
    col = PluginInfoCollection()
    del col[5]
