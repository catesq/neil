#encoding: latin-1

# Aldrin
# Modular Sequencer
# Copyright (C) 2006,2007,2008 The Aldrin Development Team
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

from gtkimport import gtk
import common
from utils import add_scrollbars
from common import MARGIN, MARGIN0, MARGIN2, MARGIN3
from aldrincom import com

LICENSES = [
	dict(
			group = 'Creative Commons',
			title = 'Attribution',
			url = 'http://creativecommons.org/licenses/by/2.5/',
		),
	dict(
			group = 'Creative Commons',
			title = 'Attribution-NoDerivs',
			url = 'http://creativecommons.org/licenses/by-nd/2.5/',
		),
	dict(
			group = 'Creative Commons',
			title = 'Attribution-NonCommercial-NoDerivs',
			url = 'http://creativecommons.org/licenses/by-nc-nd/2.5/',
		),
	dict(
			group = 'Creative Commons',
			title = 'Attribution-NonCommercial',
			url = 'http://creativecommons.org/licenses/by-nc/2.5/',
		),
	dict(
			group = 'Creative Commons',
			title = 'Attribution-NonCommercial-ShareAlike',
			url = 'http://creativecommons.org/licenses/by-nc-sa/2.5/',
		),
	dict(
			group = 'Creative Commons',
			title = 'Attribution-ShareAlike',
			url = 'http://creativecommons.org/licenses/by-sa/2.5/',
		),
]

class InfoPanel(gtk.VBox):
	"""
	Contains the info view.
	"""
	__aldrin__ = dict(
		id = 'aldrin.core.infopanel',
		singleton = True,
		categories = [
			'aldrin.viewpanel',
		]
	)		

	__view__ = dict(
			label = "Info",
			stockid = "aldrin_info",
			shortcut = 'F10',
			order = 10,
	)

	def __init__(self, rootwindow, *args, **kwds):
		"""
		Initializer.
		
		@param rootwindow: Main window.
		@type rootwindow: wx.Frame
		"""
		self.rootwindow = rootwindow
		gtk.VBox.__init__(self, False, MARGIN)
		self.set_border_width(MARGIN)
		hbox = gtk.HBox(False, MARGIN)
		hbox.pack_start(gtk.Label('License'), expand=False)
		self.cblicense = gtk.combo_box_new_text()
		for license in LICENSES:
			if license.get('group',None):
				text = "%s: %s" % (license['group'],license['title'])
			else:
				text = "%s" % (license['title'])
			self.cblicense.append_text(text)
		hbox.pack_start(self.cblicense, expand=False)
		self.view = InfoView(rootwindow)
		self.pack_start(hbox, expand=False)
		self.pack_start(add_scrollbars(self.view))
		
	def handle_focus(self):
		self.view.grab_focus()
		
	def reset(self):
		"""
		Resets the router view. Used when
		a new song is being loaded.
		"""
		self.view.reset()
		
	def update_all(self):		
		self.view.update()

class InfoView(gtk.TextView):
	"""
	Allows to enter and view text saved with the module.
	"""	
	
	def __init__(self, rootwindow):
		"""
		Initializer.
		
		@param rootwindow: Main window.
		@type rootwindow: wx.Frame
		"""
		gtk.TextView.__init__(self)
		self.set_wrap_mode(gtk.WRAP_WORD)
		self.connect('insert-at-cursor', self.on_edit)
		self.connect('delete-from-cursor', self.on_edit)
		
	def on_edit(self, event):
		"""
		Handler for text changes.
		
		@param event: Event
		@type event: wx.Event
		"""
		player = com.get('aldrin.core.player')
		player.set_infotext(self.get_buffer().get_property('text'))
		
	def reset(self):
		"""
		Resets the view.
		"""
		self.get_buffer().set_property('text', '')
		
	def update(self):
		"""
		Updates the view.
		"""
		player = com.get('aldrin.core.player')
		text = player.get_infotext()
		#~ if not text:
			#~ text = "Composed with Aldrin.\n\nThe revolution will not be televised."
		self.get_buffer().set_property('text', text)


_all__ = [
	'InfoPanel',
	'InfoView',
]

__aldrin__ = dict(
	classes = [
		InfoPanel,
		InfoView,
	],
)


if __name__ == '__main__':
	import sys
	from main import run
	#sys.argv.append(filepath('demosongs/test.bmx'))
	run(sys.argv)
