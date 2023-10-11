
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

import neil.com as com
import neil.common as common

class PatternStatusBar(Gtk.HBox):
    def __init__(self):
        Gtk.HBox.__init__(self, False, common.MARGIN)

        self.statuslabels = []

        label = Gtk.Label()
        label.set_size_request(100, 1)
        self.statuslabels.append(label)
        self.pack_start(label, False, True, 0)

        vsep = Gtk.VSeparator()
        self.pack_start(vsep, False, True, 0)

        label = Gtk.Label()
        label.set_size_request(200, 1)
        self.statuslabels.append(label)
        self.pack_start(label, False, True, 0)

        vsep = Gtk.VSeparator()
        self.pack_start(vsep, False, True, 0)

        label = Gtk.Label()
        label.set_size_request(300, 1)
        self.statuslabels.append(label)
        self.pack_start(label, False, True, 0)

        vsep = Gtk.VSeparator()
        self.pack_start(vsep, False, True, 0)

        label = Gtk.Label()
        self.statuslabels.append(label)
        self.pack_end(label, False, True, 0)

        # Add widgets to the status bar here
    def pattern_position(self, label):
        self.statuslabels[0].set_label(label)

    def parameter_value(self, label):
        self.statuslabels[1].set_label(label)

    def parameter_description(self, label):
        self.statuslabels[2].set_label(label)

    def selection_status(self, label):
        self.statuslabels[3].set_label(label)
    

