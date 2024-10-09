
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

import zzub


class OptionsGroup():
    def __init__(self, names):
        self.names = names


class Options():
    def __init__(self):
        self.amp = 0
        self.panning = 0
        self.scaling_type = 0

    def get_panning(self):
        pass

    def set_panning(self):
        pass

    def get_amp(self):
        pass

    def set_amp(self, amp):
        pass

        

class OptionsBox(Gtk.Box):
    def __init__(self):
        Gtk.Box.__init__(self, orientation=Gtk.Orientation.HORIZONTAL)
        self.options = Options()
        self.add(Gtk.Label("Options"))

    def populate(self, opts_group):
        pass

    def export_connector_data(self, connectordata: zzub.CvConnectorData):
        return zzub.CvConnectorData.create()

    def import_connector_data(self, connectordata: zzub.CvConnectorData):
        pass