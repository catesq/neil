import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

from neil.common import MARGIN
from config import get_config
from neil.utils import prepstr, buffersize_to_latency, ui
from neil.main import components
import traceback




samplerates = [96000, 48000, 44100, 22050]
buffersizes = [32768, 16384, 8192, 4096, 2048, 1024, 512, 256, 128, 64, 32, 16]




class DriverPanel(Gtk.VBox):
    """
    Panel which allows to see and change audio driver settings.
    """

    __neil__ = dict(
        id = 'neil.core.pref.driver',
        categories = [
                'neil.prefpanel',
        ]
    )

    __prefpanel__ = dict(
        label = "Audio",
    )

    def __init__(self):
        """
        Initializing.
        """
        Gtk.VBox.__init__(self)
        self.set_border_width(MARGIN)
        self.cboutput = Gtk.ComboBoxText()
        self.cbsamplerate = Gtk.ComboBoxText()
        self.cblatency = Gtk.ComboBoxText()
        size_group = Gtk.SizeGroup(Gtk.SizeGroupMode.HORIZONTAL)

        def add_row(c1, c2):
            row = Gtk.HBox(False, MARGIN)
            size_group.add_widget(c1)
            c1.set_alignment(1, 0.5)
            row.pack_start(c1, False, True, 0)
            row.pack_start(c2, True, True, 0)
            return row

        sizer1 = Gtk.Frame.new("Audio Output")
        vbox = Gtk.VBox(False, MARGIN)
        driver_row = add_row(Gtk.Label(label="Driver"), self.cboutput)
        samp_rate_row = add_row(Gtk.Label(label="Samplerate"), self.cbsamplerate)
        latency_row = add_row(Gtk.Label(label="Latency"), self.cblatency)

        vbox.pack_start(driver_row, False, True, 0)
        vbox.pack_start(samp_rate_row, False, True, 0)
        vbox.pack_start(latency_row, False, True, 0)
        vbox.set_border_width(MARGIN)
        sizer1.add(vbox)
        inputname, outputname, samplerate, buffersize = get_config().get_audiodriver_config()
        audiodriver = components.get('neil.core.driver.audio')
        if not outputname:
            outputname = audiodriver.get_name(-1)
        for i in range(audiodriver.get_count()):
            name = prepstr(audiodriver.get_name(i))
            self.cboutput.append_text(name)
            if audiodriver.get_name(i) == outputname:
                self.cboutput.set_active(i)
        for sr in samplerates:
            self.cbsamplerate.append_text("%iHz" % sr)
        self.cbsamplerate.set_active(samplerates.index(samplerate))
        for bs in buffersizes:
            self.cblatency.append_text("%.1fms" % buffersize_to_latency(bs, 44100))
        self.cblatency.set_active(buffersizes.index(buffersize))
        self.add(sizer1)

    def apply(self):
        """
        Validates user input and reinitializes the driver with current
        settings. If the reinitialization fails, the user is being
        informed and asked to change the settings.
        """
        sr = self.cbsamplerate.get_active()
        if sr == -1:
            ui.error(self, "You did not pick a valid sample rate.")
            raise components.exception('neil.exception.cancel')
        sr = samplerates[sr]
        bs = self.cblatency.get_active()
        if bs == -1:
            ui.error(self, "You did not pick a valid latency.")
            raise components.exception('neil.exception.cancel')

        bs = buffersizes[bs]
        o = self.cboutput.get_active()
        if o == -1:
            ui.error(self, "You did not select a valid output device.")
            raise components.exception('neil.exception.cancel')

        iname = ""
        audiodriver = components.get('neil.core.driver.audio')
        oname = audiodriver.get_name(o)
        inputname, outputname, samplerate, buffersize = get_config().get_audiodriver_config()
        if (oname != outputname) or (samplerate != sr) or (bs != buffersize):
            get_config().set_audiodriver_config(iname, oname, sr, bs) # write back
            try:
                audiodriver.init()

            except audiodriver.AudioInitException:
                traceback.print_exc()
                ui.error(self, "<b><big>There was an error initializing the audio driver.</big></b>\n\nThis can happen when the specified sampling rate or latency is not supported by a particular audio device. Change settings and try again.")
                raise components.exception('neil.exception.cancel')
