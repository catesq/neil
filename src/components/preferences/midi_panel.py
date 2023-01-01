from gi.repository import Gtk
from neil.common import MARGIN
from config import get_config
from neil import com
from neil.utils import prepstr, add_scrollbars, new_listview
import zzub

class MidiPanel(Gtk.VBox):
    """
    Panel which allows to see and change a list of used MIDI output devices.
    """

    __neil__ = dict(
        id = 'neil.core.pref.midi',
        categories = [
                'neil.prefpanel',
        ]
    )

    __prefpanel__ = dict(
            label = "MIDI",
    )

    def __init__(self):
        Gtk.VBox.__init__(self, False, MARGIN)
        self.set_border_width(MARGIN)
        frame1 = Gtk.Frame.new("MIDI Input Devices")
        sizer1 = Gtk.VBox()
        sizer1.set_border_width(MARGIN)
        frame1.add(sizer1)
        self.idevicelist, self.istore, columns = new_listview([
            ("Use", bool),
            ("Device", str),
        ])
        self.idevicelist.set_property('headers-visible', False)
        inputlist = get_config().get_mididriver_inputs()
        player = com.get('neil.core.player')
        for i in range(zzub.Mididriver.get_count(player)):
            if zzub.Mididriver.is_input(player,i):
                name = prepstr(zzub.Mididriver.get_name(player,i))
                use = name.strip() in inputlist
                self.istore.append([use, name])
        sizer1.add(add_scrollbars(self.idevicelist))
        frame2 = Gtk.Frame.new("MIDI Output Devices")
        sizer2 = Gtk.VBox()
        sizer2.set_border_width(MARGIN)
        frame2.add(sizer2)
        self.odevicelist, self.ostore, columns = new_listview([
            ("Use", bool),
            ("Device", str),
        ])
        self.odevicelist.set_property('headers-visible', False)
        outputlist = get_config().get_mididriver_outputs()
        for i in range(zzub.Mididriver.get_count(player)):
            if zzub.Mididriver.is_output(player,i):
                name = prepstr(zzub.Mididriver.get_name(player,i))
                use = name in outputlist
                self.ostore.append([use,name])
        sizer2.add(add_scrollbars(self.odevicelist))
        self.add(frame1)
        self.add(frame2)
        label = Gtk.Label(label="Checked MIDI devices will be used the next time you start Neil.")
        label.set_alignment(0, 0)
        self.pack_start(label, False, True, 0)

    def apply(self):
        """
        Adds the currently selected drivers to the list.
        """
        inputlist = []
        for row in self.istore:
            if row[0]:
                inputlist.append(row[1])
        get_config().set_mididriver_inputs(inputlist)
        outputlist = []
        for row in self.ostore:
            if row[0]:
                outputlist.append(row[1])
        get_config().set_mididriver_outputs(outputlist)

