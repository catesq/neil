import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

from typing import List
import zzub

from .port_info import PortInfo, PortWrapper
from .options import OptionsBox
from .port_ui import AudioPorts, TypedPorts, DummyGroup


# store the port type and port index selected by the user
# the connectdialog uses two of these - one for input plugin, one for the output 
class Connector:
    def __init__(self, plugin_id, is_target):
        self.type = None           # port_type
        self.value = None          # when parameter port it's the index of parameter. when audio port it's bitflag of audio channels where  1 = mono left, 2 = mono right, 3 = stereo
        self.is_target = is_target # this is whether it's the source or target plugin
        self.plugin_id = plugin_id # this is plugin id


    def update(self, type, value):
        self.type = type
        self.value = value


    def as_node(self):
        return zzub.CvNode.create(
            self.plugin_id,
            self.type if self.type is not None else zzub.zzub_port_type_parameter,
            self.value if self.value is not None else 0
        )



# the main ui - two columns of ports with ok/cancel buttons at the bottom
class ConnectorDialog(Gtk.Dialog):
    def __init__(self,
                 parent,
                 from_plugin: zzub.Plugin,
                 to_plugin: zzub.Plugin,
                 source_node: zzub.CvNode = None,
                 target_node: zzub.CvNode = None,
                 cvdata: zzub.CvConnectorOpts = None
                ):
        """
        Show lists of all ports on the souce and target plugin

        parent: the parent window
        from_plugin: the plugin to connect from
        to_plugin: the plugin to connect to
        source_node: port on from_plugin connecting from (only used when when editing)
        target_node: port on to_plugin connecting to (only used when when editing)
        cvdata: cv data of the connector being edited
        """
        Gtk.Dialog.__init__(
            self,
            transient_for = parent.get_toplevel(),
            title = "Connect",
            name="connector"
        )

        self.source = Connector(from_plugin.get_id(), False)
        self.target = Connector(to_plugin.get_id(), True)
        self.cvdata = zzub.CvConnectorOpts.create() if cvdata is None else cvdata

        self.target_ports = PortInfo(to_plugin)
        self.source_ports = PortInfo(from_plugin)

        connector_grid = Gtk.Grid()
        connector_grid.set_column_spacing(1)
        connector_grid.set_column_homogeneous(True)

        self.options_box = OptionsBox()

        # self.groups is list of TypedGroup, AudioGroup and DummyGroup
        # all connectors for one plugin are added to a single column of the grid,
        # the group objects keep track of which cells of the grid hold audio/midi/param/cv connectors
        # self.get_group() uses the port_type and is_target property of the group to locate them
        self.groups = self.build_groups(connector_grid, from_plugin, to_plugin)

        content = self.build_layout(connector_grid, self.options_box)
        self.get_content_area().add(content)
        self.set_size_request(400, 500)

        self.show_all()

        if source_node and target_node:
            self.get_group(source_node.port_type, False).set_selected_value(source_node.value)
            self.get_group(target_node.port_type, True).set_selected_value(target_node.value)


    def get_group(self, port_type, is_target):
        for group in self.groups:
            if group.port_type == port_type and group.is_target == is_target:
                return group

        return DummyGroup(is_target, port_type)


    # build a tall scroll view, the main scroll paine is the connector grid, below that is
    # the connector config data and below that are the ok/cancel buttons
    def build_layout(self, connector_grid, options_box):
        # scroll box with the connector grid
        scroll_box = Gtk.ScrolledWindow()
        scroll_box.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        scroll_box.add(connector_grid)

        btn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)

        cancel = Gtk.Button("Cancel")
        ok = Gtk.Button("OK")

        cancel.connect("clicked", lambda *args: self.response(Gtk.ResponseType.CANCEL))
        ok.connect("clicked", lambda *args: self.response(Gtk.ResponseType.OK))
        
        btn_box.pack_end(cancel, False, False, 0)
        btn_box.pack_end(ok, False, False, 0)

        #add scroll grid and button to main content
        main_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        main_box.pack_start(scroll_box, True, True, 0)
        main_box.pack_end(options_box, False, False, 0)
        main_box.pack_end(btn_box, False, False, 0)

        return main_box


    def get_connectors(self):
        """
        get the selected source and target connectors
        only valid if dialog returns OK
        """
        return [self.source.as_node(), self.target.as_node()]


    # the zzub.CvConnectorOpts object
    def get_cv_data(self) -> zzub.CvConnectorOpts:
        return self.cvdata


    def item_selected(self, port_type, is_target, value):
        connector = self.target if is_target else self.source

        if connector.type is not None:
            group = self.get_group(connector.type, connector.is_target)
            group.clear(connector.value)
            
        connector.update(port_type, value)

        group = self.get_group(port_type, is_target)
        group.highlight(value)


    # builds two sets of grids and add them to the main grid. one on left for source connectors
    def build_groups(self, grid: Gtk.Grid, from_plugin: zzub.Plugin, to_plugin: zzub.Plugin):
        grid.attach(Gtk.Label(from_plugin.get_name()), 0, 0, 1, 1)
        grid.attach(Gtk.Label(to_plugin.get_name()), 1, 0, 1, 1)

        # the number of rows in left and right columns
        grid.row_counts = [1, 1]

        source_column = self.build_column(grid, self.source_ports, 0, "out")
        target_column = self.build_column(grid, self.target_ports, 1, "in")

        return source_column + target_column


    # build the audio/midi/paramater connectors grids.
    # and returns a list of grids
    def build_column(self, grid, ports: List[PortWrapper], is_target, suffix):
        sub_grids = []

        if is_target:
            [audio_ports, midi_ports, parameter_ports, track_ports, cv_ports] = [ports.audio_in_ports, ports.midi_in_ports, ports.parameter_in_ports, ports.track_in_ports, ports.cv_in_ports]
        else:
            [audio_ports, midi_ports, parameter_ports, track_ports, cv_ports] = [ports.audio_out_ports, ports.midi_out_ports, ports.parameter_in_ports, ports.track_in_ports, ports.cv_out_ports]

        if len(audio_ports) > 0:
            sub_grids.append(self.build_audio_group(grid, audio_ports, is_target, "audio %s" % suffix))

        if len(cv_ports) > 0:
            sub_grids.append(self.build_port_group(grid, cv_ports, is_target, "cv", zzub.zzub_port_type_cv))

        if len(midi_ports) > 0:
            sub_grids.append(self.build_port_group(grid, midi_ports, is_target, "midi %s" % suffix, zzub.zzub_port_type_midi))

        if len(parameter_ports) > 0:
            sub_grids.append(self.build_port_group(grid, parameter_ports, is_target, "param", zzub.zzub_port_type_parameter))

        if len(track_ports) > 0:
            sub_grids.append(self.build_port_group(grid, track_ports, is_target, "track", zzub.zzub_port_type_track))

        return sub_grids


    #
    def build_audio_group(self, grid, ports: List[PortWrapper], is_target, title):
        group = AudioPorts(grid, is_target, title, len(ports))
        group.set_handler(self.item_selected)
        return group



    def build_port_group(self, grid, ports: List[PortWrapper], is_target, title, port_type):
        group = TypedPorts(grid, is_target, title, ports, port_type)
        group.set_handler(self.item_selected)
        return group


