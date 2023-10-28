
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

class PatternStatus():
    def __init__(self):
        self.parameter_raw = Gtk.Label()
        self.parameter_info = Gtk.Label()
        
        self.parameter_info.set_halign(Gtk.Align.START)
        self.parameter_raw.set_halign(Gtk.Align.START)

        self.parameter_box = Gtk.HBox()
        self.parameter_box.pack_start(self.parameter_info, True, True, 0)
        self.parameter_box.pack_end(self.parameter_raw, False, False, 0)
        self.parameter_box.set_hexpand(True)

        self.pattern_position = Gtk.Label()
        self.parameter_description = Gtk.Label()
        self.selection_status = Gtk.Label()

    def update_pattern_position(self, label):
        self.pattern_position.set_label(label)

    def update_parameter_values(self, detail, raw):
        self.parameter_info.set_label(detail)
        self.parameter_raw.set_label(raw)

    def update_parameter_description(self, label):
        self.parameter_description.set_label(label)

    def update_selection_status(self, label):
        self.selection_status.set_label(label)

    def get_position_widget(self) -> Gtk.Widget:
        return self.pattern_position

    def get_values_widget(self) -> Gtk.Widget:
        return self.parameter_box

    def get_description_widget(self) -> Gtk.Widget:
        return self.parameter_description

    def get_selection_widget(self) -> Gtk.Widget:
        return self.selection_status
    

