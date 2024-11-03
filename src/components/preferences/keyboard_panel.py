import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

from neil.utils import sizes
from config import get_config

class KeyboardPanel(Gtk.VBox):
    """
    Panel which allows to see and change the current keyboard configuration.
    """

    __neil__ = dict(
        id = 'neil.core.pref.keyboard',
        categories = [
                'neil.prefpanel',
        ]
    )

    __prefpanel__ = dict(
        label = "Keyboard",
    )

    KEYMAPS = [
        ('en', 'English (QWERTY)'),
        ('de', 'Deutsch (QWERTZ)'),
        ('dv', 'Dvorak (\',.PYF)'),
        ('fr', 'French (AZERTY)'),
        ('neo','Neo (XVLCWK)')
    ]

    def __init__(self):
        Gtk.VBox.__init__(self, False, sizes.get('margin'))
        self.set_border_width(sizes.get('margin'))
        hsizer = Gtk.HBox(False, sizes.get('margin'))
        hsizer.pack_start(Gtk.Label("Keyboard Map"), True, True, 0)
        self.cblanguage = Gtk.ComboBoxText()
        sel = 0
        lang = get_config().get_keymap_language()
        index = 0
        for kmid, name in self.KEYMAPS:
            self.cblanguage.append_text(name)
            if lang == kmid:
                sel = index
            index += 1
        hsizer.add(self.cblanguage)
        self.pack_start(hsizer, False, True, 0)
        self.cblanguage.set_active(sel)

    def apply(self):
        """
        applies the keymap.
        """
        get_config().set_keymap_language(self.KEYMAPS[self.cblanguage.get_active()][0])
