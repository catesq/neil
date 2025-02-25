
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

import os

from neil import components

from config import get_config
from neil.utils import sharedpath, sizes

class GeneralPanel(Gtk.VBox):
    """
    Panel which allows changing of general settings.
    """

    __neil__ = dict(
        id = 'neil.core.pref.general',
        categories = [
                'neil.prefpanel',
        ]
    )

    __prefpanel__ = dict(
        label = "General",
    )

    def filter_monospace_only(self, family, face):
        return family.is_monospace() and face.describe().get_style() == Pango.Style.NORMAL

    def __init__(self):
        """
        Initializing.
        """
        Gtk.VBox.__init__(self)
        self.set_border_width(sizes.get('margin'))
        frame1 = Gtk.Frame.new("General Settings")
        fssizer = Gtk.VBox(False, sizes.get('margin'))
        fssizer.set_border_width(sizes.get('margin'))
        frame1.add(fssizer)
        incsave = get_config().get_incremental_saving()
        #rackpanel = get_config().get_experimental('RackPanel')
        leddraw = get_config().get_led_draw()
        curvearrows = get_config().get_curve_arrows()
        patnoteoff = get_config().get_pattern_noteoff()
        self.patternfont = Gtk.FontButton.new_with_font(get_config().get_pattern_font())
        self.patternfont.set_use_font(True)
        self.patternfont.set_use_size(True)
        self.patternfont.set_show_style(True)
        self.patternfont.set_show_size(True)
        self.patternfont.set_filter_func(self.filter_monospace_only)

        self.incsave = Gtk.CheckButton()
        self.leddraw = Gtk.CheckButton()
        self.curvearrows = Gtk.CheckButton()
        self.patnoteoff = Gtk.CheckButton()
        self.rackpanel = Gtk.CheckButton()
        self.incsave.set_active(int(incsave))
        self.leddraw.set_active(int(leddraw))
        self.curvearrows.set_active(int(curvearrows))
        self.patnoteoff.set_active(int(patnoteoff))
        #self.rackpanel.set_active(rackpanel)

        self.theme = Gtk.ComboBoxText()
        themes = os.listdir(sharedpath('themes'))
        self.theme.append_text('Default')

        for i, theme in enumerate(themes):
            name = os.path.splitext(theme)[0]
            self.theme.append_text(name);
            if name == get_config().active_theme:
                self.theme.set_active(i)

        sg1 = Gtk.SizeGroup(Gtk.SizeGroupMode.HORIZONTAL)
        sg2 = Gtk.SizeGroup(Gtk.SizeGroupMode.HORIZONTAL)

        def add_row(c1, c2):
            row = Gtk.HBox(False, sizes.get('margin'))
            c1.set_alignment(1, 0.5)
            sg1.add_widget(c1)
            sg2.add_widget(c2)
            row.pack_start(c1, False, True, 0)
            row.pack_end(c2, True, True, 0)
            fssizer.pack_start(row, False, True, 0)

        add_row(Gtk.Label(label="Incremental Saves"), self.incsave)
        add_row(Gtk.Label(label="Draw Amp LEDs in Router"), self.leddraw)
        add_row(Gtk.Label(label="Auto Note-Off in Pattern Editor"), self.patnoteoff)
        add_row(Gtk.Label(label="Pattern Font"), self.patternfont)
        #add_row(Gtk.Label(label="Rack Panel View (After Restart)"), self.rackpanel)
        add_row(Gtk.Label(label="Theme"), self.theme)
        add_row(Gtk.Label(label="Draw Curves in Router"), self.curvearrows)
        self.add(frame1)

    def apply(self):
        """
        Writes general config settings to file.
        """
        if self.patternfont.get_font_name():
            get_config().set_pattern_font(self.patternfont.get_font_name())

        get_config().set_incremental_saving(self.incsave.get_active())
        get_config().set_led_draw(self.leddraw.get_active())
        get_config().set_pattern_noteoff(self.patnoteoff.get_active())
        get_config().set_curve_arrows(self.curvearrows.get_active())
        #get_config().set_experimental('RackPanel', self.rackpanel.get_active())
        theme_name = self.theme.get_active_text()
        if get_config().active_theme != theme_name:
            if theme_name == 'Default':
                get_config().select_theme(None)
            else:
                get_config().select_theme(self.theme.get_active_text())

        components.get('neil.core.patternpanel').update_all()
        components.get('neil.core.router.view').update_all()
        components.get('neil.core.sequencerpanel').update_all()
