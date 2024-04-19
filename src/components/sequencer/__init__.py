
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


from .panel import SequencerPanel
from .view import SequencerView, PatternNotFoundException

if __name__ == '__main__':
    import os
    os.system('../../bin/neil-combrowser neil.core.sequencerpanel')
    raise SystemExit


__all__ = [
    'PatternNotFoundException',
    'SequencerPanel',
    'SequencerView',
]

__neil__ = dict(
    classes = [
        SequencerPanel,
        # SequencerView,
    ],
)
