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
Provides an info view which allows to enter text.
"""
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Pango

from neil import components, sizes

class InfoPanel(Gtk.VBox):
    """
    Contains the info view.
    """
    __neil__ = dict(
        id = 'neil.core.infopanel',
        singleton = True,
        categories = [
            'neil.viewpanel',
            'view',
        ]
    )

    __view__ = dict(
        label = "Info",
        stockid = "neil_info",
        shortcut = 'F11',
        order = 11,
    )

    def __init__(self, *args, **kwds):
        """
        Initializer.
        """
        Gtk.VBox.__init__(self, False, sizes.get('margin'))
        # scrollbars = ui.add_scrollbars(self.view)
        self.set_border_width(sizes.get('margin'))
        self.info_view = InfoView()
        self.pack_start(self.info_view, True, True, 0)
        eventbus = components.get('neil.core.eventbus')
        eventbus.document_loaded += self.update_all

    def handle_focus(self):
        self.info_view.grab_focus()

    def reset(self):
        """
        Resets the router view. Used when
        a new song is being loaded.
        """
        self.info_view.reset()

    def update_all(self):
        self.info_view.update()

class InfoView(Gtk.TextView):
    """
    Allows to enter and view text saved with the module.
    """

    def __init__(self):
        """
        Initializer.
        """
        Gtk.TextView.__init__(self)
        self.set_wrap_mode(Gtk.WrapMode.WORD)
        self.get_buffer().connect('changed', self.on_edit)
        self.modify_font(Pango.FontDescription('monospace 8'))

    def on_edit(self, buffer_):
        """
        Handler for text changes.

        @param event: Event
        @type event: wx.Event
        """
        player = components.get('neil.core.player')
        text = self.get_buffer().get_property('text')
        player.set_infotext(text)

    def reset(self):
        """
        Resets the view.
        """
        self.get_buffer().set_property('text', '')

    def update(self):
        """
        Updates the view.
        """
        player = components.get('neil.core.player')
        text = player.get_infotext()
        self.get_buffer().set_property('text', text)


_all__ = [
    'InfoPanel',
    'InfoView',
]

__neil__ = dict(
    classes = [
        InfoPanel,
        # InfoView,
    ],
)


if __name__ == '__main__':
    import sys
    from neil.main import run
    #sys.argv.append(filepath('demosongs/test.bmx'))
    run(sys.argv)
