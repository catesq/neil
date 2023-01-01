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
Contains panels and dialogs related to application preferences.
"""

from gi.repository import Gtk

from .general_panel import GeneralPanel
from .driver_panel import DriverPanel
from .controller_panel import ControllerPanel
from .preferences_dialog import PreferencesDialog
from .midi_panel import MidiPanel
from .keyboard_panel import KeyboardPanel


class CancelException(Exception):
    """
    Is being thrown when the user hits cancel in a sequence of
    modal UI dialogs.
    """
    __neil__ = dict(
        id = 'neil.exception.cancel',
        exception = True,
        categories = [
        ]
    )


def show_preferences(parent, *args):
    """
    Shows the {PreferencesDialog}.

    @param rootwindow: The root window which receives zzub callbacks.
    @type rootwindow: wx.Frame
    @param parent: Parent window.
    @type parent: wx.Window
    """
    return PreferencesDialog(parent, *args)


__neil__ = dict(
    classes = [
        CancelException,
        GeneralPanel,
        DriverPanel,
        ControllerPanel,
        MidiPanel,
        KeyboardPanel,
        PreferencesDialog,
    ],
)


__all__ = [
    'CancelException',
    'GeneralPanel',
    'DriverPanel',
    'ControllerPanel',
    'MidiPanel',
    'KeyboardPanel',
    'PreferencesDialog',
    'show_preferences',
]


if __name__ == '__main__':
    import testplayer
    player = testplayer.get_player()
    window = testplayer.TestWindow()
    window.show_all()
    show_preferences(window)
    Gtk.main()
