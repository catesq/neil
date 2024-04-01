
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

import zzub

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