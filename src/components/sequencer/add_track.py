# This Python file uses the following encoding: utf-8
import gi
from gi.repository import Gtk

class AddSequencerTrackDialog(Gtk.Dialog):
    """
    Sequencer Dialog Box.

    This dialog is used to create a new track for an existing machine.
    """
    def __init__(self, parent, machines):
        Gtk.Dialog.__init__(self,
                "Add track",
                parent.get_toplevel(),
                Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT,
                None
        )
        self.btnok = self.add_button(Gtk.STOCK_OK, Gtk.ResponseType.OK)
        self.btncancel = self.add_button(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL)
        self.combo = Gtk.ComboBoxText()
        for machine in sorted(machines, key=lambda m: m.lower()):
            self.combo.append_text(machine)
        # Set a default.
        self.combo.set_active(0)
        self.get_content_area().add(self.combo)
        self.show_all()
