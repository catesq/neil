
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

class PatternStatus():
    def __init__(self):
        self.parameter_value = Gtk.Label()
        self.pattern_position = Gtk.Label()
        self.parameter_description = Gtk.Label()
        self.selection_status = Gtk.Label()

    def update_pattern_position(self, label):
        self.pattern_position.set_label(label)

    def update_parameter_value(self, label):
        self.parameter_value.set_label(label)

    def update_parameter_description(self, label):
        self.parameter_description.set_label(label)

    def update_selection_status(self, label):
        self.selection_status.set_label(label)

    def get_pattern_position(self) -> Gtk.Label:
        return self.pattern_position

    def get_parameter_value(self) -> Gtk.Label:
        return self.parameter_value

    def get_parameter_description(self) -> Gtk.Label:
        return self.parameter_description

    def get_selection_status(self) -> Gtk.Label:
        return self.selection_status
    

