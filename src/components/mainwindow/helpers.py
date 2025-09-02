
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

# used to compare the neil objects which contains a __view__ dict, sorts them for the view menu
def cmp_view(a, b):
    a_order = (hasattr(a, '__view__') and a.__view__.get('order',0)) or 0
    b_order = (hasattr(b, '__view__') and b.__view__.get('order',0)) or 0
    return a_order <= b_order




class Accelerators(Gtk.AccelGroup):
    __neil__ = dict(
        id = 'neil.core.accelerators',
        singleton = True,
        categories = [],
    )

    def __init__(self):
        Gtk.AccelGroup.__init__(self)

    def add_accelerator(self, shortcut, widget, signal="activate"):
        key, modifier = Gtk.accelerator_parse(shortcut)
        return widget.add_accelerator(signal, self,  key,  modifier, Gtk.AccelFlags.VISIBLE)