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
Contains all classes and functions needed to render the pattern
editor and its associated dialogs.
"""

import gi
gi.require_version("Gtk", "3.0")


from .toolbar import PatternToolBar
from .panel import PatternPanel
from .views import PatternView, PatternDialog, show_pattern_dialog


from .utils import get_str_from_param, get_subindexcount_from_param, get_subindexoffsets_from_param


__all__ = [
    'PatternDialog',
    'PatternToolBar',
    'PatternPanel',
    'PatternView',
    'show_pattern_dialog',
    'get_str_from_param',
    'get_length_from_param',
    'get_subindexcount_from_param',
    'get_subindexoffsets_from_param',
]

__neil__ = dict(
    classes = [
        PatternDialog,
        PatternToolBar,
        PatternPanel,
        PatternView,
    ],
)

if __name__ == '__main__':
    import os, sys
    os.system('../../bin/neil-combrowser neil.core.patternpanel')
    sys.exit(0)
