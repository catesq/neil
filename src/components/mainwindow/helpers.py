
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk



def is_panel(view):
    return hasattr(view, '__is_panel__') and view.__is_panel__
    # return 'neil.viewpanel' in view.__neil__.get('categories',[])



def set_is_panel(view, is_panel=True):
    view.__is_panel__ = bool(is_panel)



# used to compare the neil objects which contains a __view__ dict, sorts them for the view menu
def cmp_view(a, b):
    a_order = (hasattr(a, '__view__') and a.__view__.get('order',0)) or 0
    b_order = (hasattr(b, '__view__') and b.__view__.get('order',0)) or 0
    return a_order <= b_order



class Accelerators(Gtk.AccelGroup):
    """
    Shortcuts for neil. Several components retreive this and add their own shortcuts. Bit of a free for all, not ideal.
    """
    __neil__ = dict(
        id = 'neil.core.accelerators',
        singleton = True
    )

    def __init__(self):
        Gtk.AccelGroup.__init__(self)

    def add_accelerator(self, shortcut, widget, signal="activate"):
        key, modifier = Gtk.accelerator_parse(shortcut)
        return widget.add_accelerator(signal, self,  key,  modifier, Gtk.AccelFlags.VISIBLE)

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

class NeilException(Exception):
    """
    All purpose exception
    """
    __neil__ = dict(
        id = 'neil.core.error',
        exception = True
    )
