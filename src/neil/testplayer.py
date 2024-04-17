#encoding: latin-1

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
Provides a test player for testcases.
"""

from gi.repository import Gtk
from gi.repository import GLib

from config import get_plugin_aliases, get_plugin_blacklist
from . import common
from neil import components
import zzub

_player = None

event_handlers = []

def player_callback(player, plugin, data, tag):
    """
    Default callback for ui events sent by zzub.
    """
    result = False
    for handler in event_handlers:
        result = handler(player,plugin,data) or result
    return result


class TestWindow(Gtk.Window):
    def __init__(self):
        Gtk.Window.__init__(self, title="Test player")
        self.event_handlers = event_handlers
        self.resize(640,480)
        self.connect('destroy', lambda widget: Gtk.main_quit())
        self.show_all()
        get_player()

def get_player():
    components.init()
    global _player
    if _player is not None:
        return _player

    _player = components.get('neil.core.player')

    _player.set_callback(zzub.zzub_callback_t(player_callback), 2)

    def handle_events(player):
        _player.handle_events()
        return True

    GLib.timeout_add(1000/25, handle_events, _player)

    return _player

__all__ = [
    'get_player',
    'TestWindow'
]

if __name__ == '__main__':
    player = get_player()
