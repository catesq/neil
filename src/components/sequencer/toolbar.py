import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

from neil import components, sizes
import config

class SequencerToolBar(Gtk.HBox):
    """
    Sequencer Toolbar

    Allows to set the step size for the sequencer view.
    """
    def __init__(self, seqview):
        """
        Initialization.
        """
        Gtk.HBox.__init__(self, False, sizes.get('margin'))
        self.seqview = seqview
        self.set_border_width(sizes.get('margin'))
        self.steplabel = Gtk.Label()
        self.steplabel.set_text_with_mnemonic("_Step")
        self.steps = [1, 2, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64]
        self.stepselect = Gtk.ComboBoxText.new()
        for step in self.steps:
            self.stepselect.append_text(str(step))

        self.stepselect.connect('changed', self.on_stepselect)
        self.stepselect.connect('key_release_event', self.on_stepselect)
        self.stepselect.set_size_request(60, -1)
        self.steplabel.set_mnemonic_widget(self.stepselect)
        # Follow song checkbox.
        self.followsong = Gtk.CheckButton("Follow Song Position")
        self.followsong.set_active(False)
        # Display all the components.
        self.pack_start(self.steplabel, False, True, 0)
        self.pack_start(self.stepselect, False, True, 0)
        self.pack_start(self.followsong, False, True, 0)

    def increase_step(self):
        if self.seqview.step < 64:
            self.seqview.step *= 2
        self.update_stepselect()
        self.seqview.update()

    def decrease_step(self):
        if self.seqview.step > 1:
            self.seqview.step >>= 1
        self.update_stepselect()
        self.seqview.update()

    def update_all(self):
        """
        Updates the toolbar to reflect sequencer changes.
        """
        self.update_stepselect()

    def update_stepselect(self):
        """
        Updates the step selection choice box.
        """
        player = components.get('neil.core.player')
        try:
            self.stepselect.set_active(self.steps.index(self.seqview.step))
            config.get_config().set_default_int('SequencerStep', self.seqview.step)
            player.sequence_step = self.seqview.step
        except ValueError:
            pass
        self.seqview.adjust_scrollbars()

    def on_stepselect(self, widget, event=False):
        """
        Handles events sent from the choice box when a step size is being selected.
        """
        try:
            step = int(widget.get_active_text())
        except:
            self.seqview.step = 1
            return
        if widget.get_active() == -1 and event == False:
            return
        if self.seqview.step == step:
            return
        if (step > 128):
            self.seqview.step = 128
        if (step < 1):
            self.seqview.step = 1
        else:
            self.seqview.step = step
        self.seqview.update()
        player = components.get('neil.core.player')
        player.set_seqstep(step)
