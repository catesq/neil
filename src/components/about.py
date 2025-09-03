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
Contains the information displayed in the about box.
"""

import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

from neil.utils import imagepath

NAME = "Neil"
VERSION = "0.9"
COPYRIGHT = "Copyright (C) 2009 Vytautas Jančauskas"
COMMENTS = '"The highest activity a human being can attain is learning for understanding, because to understand is to be free" -- Baruch Spinoza'

LICENSE = """This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA."""

WEBSITE = "http://sites.google.com/site/neilsequencer/"

AUTHORS = [
    'Vytautas Jančauskas <unaudio@gmail.com>',
    'Leonard Ritter <contact@leonard-ritter.com>',
    'Pieter Holtzhausen <13682857@sun.ac.za>',
    'Anders Ervik <calvin@countzero.no>',
    'James Stone <jamesmstone@gmail.com>',
    'James McDermott <jamesmichaelmcdermott@gmail.com>',
    'Carsten Sørensen',
    'Joachim Michaelis <jm@binarywerks.dk>',
    'Aaron Oxford <aaron@hardwarehookups.com.au>',
    'Laurent De Soras <laurent.de.soras@club-internet.fr>',
    'Greg Raue <graue@oceanbase.org>',
]

ARTISTS = [
    'syntax_the_nerd (the logo)',
    'famfamfam http://www.famfamfam.com/lab/icons/silk/',
]

DOCUMENTERS = [
    'Vytautas Jančauskas <unaudio@gmail.com>',
    'Leonard Ritter <contact@leonard-ritter.com>',
    'Pieter Holtzhausen <13682857@sun.ac.za>',
    'Phed',
]


class AboutDialog(Gtk.AboutDialog):
    """
    A simple about dialog with a text control and an OK button.
    """

    __neil__ = dict(
        id = "neil.core.dialog.about",
        categories = ["viewdialog"]
    )

    def __init__(self):
        """
        Initialization.
        """
        Gtk.AboutDialog.__init__(self)
        self.set_name(NAME)
        self.set_version(VERSION)
        self.set_copyright(COPYRIGHT)
        self.set_comments(COMMENTS)
        self.set_license(LICENSE)
        self.set_wrap_license(True)
        self.set_website(WEBSITE)
        self.set_authors(AUTHORS)
        self.set_artists(ARTISTS)
        self.set_documenters(DOCUMENTERS)
        self.set_logo(self.get_pixbuf(imagepath("alien.png")))

    def get_pixbuf(self, filename):
        image = Gtk.Image()
        image.set_from_file(filename)
        return image.get_pixbuf()

    def show(self):
        self.run()
        self.destroy()

__neil__ = dict(
    classes = [
        AboutDialog,
    ]
)

__all__ = [
]

if __name__ == '__main__':
    AboutDialog().run()
