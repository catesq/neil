
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
import zzub


PLUGIN_DRAG_TARGETS = [
    Gtk.TargetEntry.new('application/x-neil-plugin-uri', Gtk.TargetFlags.SAME_APP, 1)
]

class PluginInfo(object):
    """
    Encapsulates data associated with a plugin.
    """
    def __init__(self, plugin):
        self.plugin = plugin
        self.muted = False
        self.bypassed = False
        self.cpu = -9999.0
        self.pattern_position = (0, 0, 0, 0, 0)
        self.selection = None
        self.songplugin = True
        self.plugingfx = None
        self.patterngfx = {}
        self.gfx_cache = {}
        self.amp = -9999.0
        self.octave = 3

    def reset_patterngfx(self):
        self.patterngfx = {}

    def get_cached_gfx(self, key):
        return self.gfx_cache.get(key, None)

    def add_cached_gfx(self, key, item):
        self.gfx_cache[key] = item
    
    def reset_plugingfx(self):
        self.plugingfx = None
        self.amp = -9999.0
        self.cpu = -9999.0



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



def get_plugin_infos() -> PluginInfoCollection:
    global collection
    if not collection:
        collection = PluginInfoCollection()
    return collection


if __name__ == '__main__':
    components.load_packages()
    col = PluginInfoCollection()
    del col[5]
