import string
from typing import List

import gi
gi.require_version('Gtk', '3.0')

from gi.repository import Gtk

import zzub



# used to store port information, matches api of ports on the lv2 plugin
class PortWrapper:
    def __init__(self, name, flow, index, type):
        self.name = name
        self.flow = flow
        self.index = index
        self.type = type


    def get_index(self):
        return self.index


    def get_flow(self):
        return self.flow


    def get_type(self):
        return self.type


    def get_name(self):
        return self.name



# The connect dialog builds a PortInfo structure for the source and target plugins
# It builds a list of PortWrapper objects
# only the lv2 plugin wrapper directly supports port's
# vst and zzub plugins use the make_ports() method to mimic the lv2 ports
class PortInfo:
    def __init__(self, plugin):
        if plugin.get_flags() & zzub.zzub_plugin_flag_has_ports:
            ports = list(plugin.get_ports())
        else:
            ports = self.make_ports(plugin)

        def filter_ports(type, flow):
            return [port for port in ports if port.get_type() == type and port.get_flow() == flow]

        self.audio_in_ports = filter_ports(zzub.zzub_port_type_audio, zzub.zzub_port_flow_input)
        self.audio_out_ports = filter_ports(zzub.zzub_port_type_audio, zzub.zzub_port_flow_output)

        self.midi_in_ports = filter_ports(zzub.zzub_port_type_midi, zzub.zzub_port_flow_input)
        self.midi_out_ports = filter_ports(zzub.zzub_port_type_midi, zzub.zzub_port_flow_output)

        self.parameter_in_ports = filter_ports(zzub.zzub_port_type_parameter, zzub.zzub_port_flow_input)
        self.parameter_out_ports = filter_ports(zzub.zzub_port_type_parameter, zzub.zzub_port_flow_output)

        self.cv_in_ports = filter_ports(zzub.zzub_port_type_cv, zzub.zzub_port_flow_input)
        self.cv_out_ports = filter_ports(zzub.zzub_port_type_cv, zzub.zzub_port_flow_output)

        self.track_in_ports = filter_ports(zzub.zzub_port_type_track, zzub.zzub_port_flow_input)
        self.track_out_ports = []


    def make_ports(self, mp) -> List[PortWrapper]:
        ports = []

        if mp.get_flags() & zzub.zzub_plugin_flag_has_audio_input:
            ports.append(PortWrapper("Audio In Left", zzub.zzub_port_flow_input, 0, zzub.zzub_port_type_audio))
            ports.append(PortWrapper("Audio In Right", zzub.zzub_port_flow_input, 1, zzub.zzub_port_type_audio))

        if mp.get_flags() & zzub.zzub_plugin_flag_has_audio_output:
            ports.append(PortWrapper("Audio Out Left", zzub.zzub_port_flow_output, 0, zzub.zzub_port_type_audio))
            ports.append(PortWrapper("Audio Out Right", zzub.zzub_port_flow_output, 1, zzub.zzub_port_type_audio))

        if mp.get_flags() & zzub.zzub_plugin_flag_has_midi_input:
            ports.append(PortWrapper("Midi In", zzub.zzub_port_flow_input, 0, zzub.zzub_port_type_midi))

        if mp.get_flags() & zzub.zzub_plugin_flag_has_midi_output:
            ports.append(PortWrapper("Midi Out", zzub.zzub_port_flow_output, 0, zzub.zzub_port_type_midi))

        for index, param in enumerate(mp.get_global_parameters()):
            ports.append(PortWrapper(param.get_name(), zzub.zzub_port_flow_input, index, zzub.zzub_port_type_parameter))

        return ports



# a toggle switch which display port name and changes colour when clicked
# would prefer to use a gtk.label but they inherit background color from enclosing widget which makes changing colour messy
class BoxyLabel(Gtk.EventBox):
    def __init__(self, text):
        Gtk.EventBox.__init__(self, expand=True)
        self.label = Gtk.Label(text)
        self.add(self.label)


    def set_selected_value(self, value):
        if value:
            self.get_style_context().add_class("selected")
        else:
            self.get_style_context().remove_class("selected")


# used to populate a grid with two columns and very variable number of rows
# it uses/updates a row_counts property on the grid to
# the inital row_offset is read from the grid.row_counts property
# is_target is used as the column index
# the build_rows function adds rows - starting from row_offset - and returns the number of new rows added
class AbstractPortGroup:
    def __init__(self, grid, is_target, port_type, name, *args):
        self.row_offset = grid.row_counts[is_target]
        self.name = name                                     # eg 'audio in', 'midi out'
        self.full_css_name = name.replace(" ", "_")          # eg 'audio_in', 'midi_out'
        self.type_css_name = name[0:name.find(" ")]          # eg 'audio', 'midi'

        self.port_type = port_type
        self.is_target = is_target
        self.selected = 0
        self.handler = None

        self.row_count = self.build_rows(grid, self.is_target, self.row_offset, *args)

        grid.row_counts[is_target] = self.row_offset + self.row_count


    def build_rows(self, grid, column_id, first_row, *args):
        return 0


    def set_handler(self, handler):
        self.handler = handler


    def add_base_css(self, widget):
        context = widget.get_style_context()
        context.add_class(self.full_css_name)
        context.add_class(self.type_css_name)


    def set_selected_value(self, value):
        self.selected = value
        self.add_highlight(value)

        if self.handler:
            self.handler(self.port_type, self.is_target, value)


    def remove_highlight(self, value):
        pass


    def add_highlight(self, value):
        pass



class DummyGroup():
    def __init__(self, is_target, port_type):
        self.port_type = port_type
        self.is_target = is_target


    def set_handler(self, handler):
        pass


    def remove_highlight(self, value):
        pass


    def add_highlight(self, value):
        pass


    def set_selected_value(self, value):
        pass



class TypedPorts(AbstractPortGroup):
    def __init__(self, grid, is_target, name, ports, port_type):
        AbstractPortGroup.__init__(self, grid, is_target, port_type, name, ports)


    def build_rows(self, grid, column_id, first_row, ports):
        self.labels = []

        for index, port in enumerate(ports):
            label = BoxyLabel(port.get_name())
            label.connect("button-release-event", self.toggle_port, index)
            grid.attach(label, column_id, first_row + index, 1, 1)
            self.add_base_css(label)
            self.labels.append(label)

        return len(ports)


    def toggle_port(self, widget, event, index):
        self.set_selected_value(index)


    def remove_highlight(self, value):
        if value < len(self.labels):
            self.labels[value].get_style_context().remove_class("selected")


    def add_highlight(self, value):
        if value < len(self.labels):
            self.labels[value].get_style_context().add_class("selected")




# there are two audiogrids in the connect dialog, the left side is ports on source source, right side is ports for target plugin
# int used as a bitflag of which channels are selected
class AudioPorts(AbstractPortGroup):
    def __init__(self, grid, is_target, name, port_count):
        AbstractPortGroup.__init__(self, grid, is_target, zzub.zzub_port_type_audio, name, port_count)


    def build_rows(self, grid, column_id, first_row, port_count):
        if port_count == 1:
            channels = self.build_channel_list(["mono"])
        elif port_count == 2:
            channels = self.build_channel_list(["L", "R"])
        else:
            channels = self.build_channel_list(range(max(port_count, 4)))

        self.channels = channels
        grid.attach(channels, column_id, first_row, 1, 1)

        return 1


    # return number of columns created
    def build_channel_list(self, titles) -> Gtk.Box:
        hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)

        for index, title in enumerate(titles):
            label = BoxyLabel(title)
            label.connect("button-release-event", self.toggle_channel, index)
            hbox.pack_start(label, True, True, 0)
            self.add_base_css(label)

        return hbox


    # the "L" or "R" button was clicked. toggle: channel on <-> channel off
    def toggle_channel(self, widget, event, channel):
        self.set_selected_value(self.selected ^ (1 << channel))


    def remove_highlight(self, value):
        for label in self.channels.get_children():
            label.get_style_context().remove_class("selected")


    def add_highlight(self, value):
        for index, label in enumerate(self.channels.get_children()):
            if value & (1 << index):
                label.get_style_context().add_class("selected")




# store selected port info to create a cvnode
# the connectdialog has two of these and they are update when user clicks a label in the dialog
class Connector:
    def __init__(self, plugin_id, is_target):
        self.type = None           # audio/parameter/cv
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


class ConnectDialog(Gtk.Dialog):
    def __init__(self,
                 parent,
                 from_plugin: zzub.Plugin,
                 to_plugin: zzub.Plugin,
                 source_node: zzub.CvNode = None,
                 target_node: zzub.CvNode = None,
                 cvdata: zzub.CvConnectorData = None
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
        self.cvdata = zzub.CvConnectorData.create() if cvdata is None else cvdata

        self.target_ports = PortInfo(to_plugin)
        self.source_ports = PortInfo(from_plugin)

        self.grid = Gtk.Grid()
        self.grid.set_column_spacing(1)
        self.grid.set_column_homogeneous(True)

        # self.groups is list of TypedGroup, AudioGroup and DummyGroup
        # all connectors for one plugin are added to a single column of the grid,
        # the group objects keep track of which cells of the grid hold audio/midi/param/cv connectors
        # self.get_group() uses the port_type and is_target property of the group to locate them
        self.groups = self.build_groups(self.grid, from_plugin, to_plugin)

        content = self.build_layout(self.grid)
        self.get_content_area().add(content)
        self.set_size_request(400, 500)

        self.show_all()

        if source_node and target_node:
            self.get_group(source_node.get_type(), 0).set_selected_value(source_node.get_value())
            self.get_group(target_node.get_type(), 1).set_selected_value(target_node.get_value())


    def get_group(self, port_type, is_target):
        for group in self.groups:
            if group.port_type == port_type and group.is_target == is_target:
                return group

        return DummyGroup(is_target, port_type)


    def build_layout(self, grid):
        # add hbox to bottom of vbox
        btn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)

        scroll = Gtk.ScrolledWindow()
        scroll.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        scroll.add(grid)

        cancel = Gtk.Button("Cancel")
        ok = Gtk.Button("OK")

        btn_box.pack_end(cancel, False, False, 0)
        btn_box.pack_end(ok, False, False, 0)

        cancel.connect("clicked", lambda *args: self.response(Gtk.ResponseType.CANCEL))
        ok.connect("clicked", lambda *args: self.response(Gtk.ResponseType.OK))

        #add scroll grid and button to main content
        main_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        main_box.pack_start(scroll, True, True, 0)
        main_box.pack_end(btn_box, False, False, 0)

        return main_box


    def get_connectors(self):
        """
        get the selected source and target connectors
        only valid if dialog returns OK
        """
        return [self.source.as_node(), self.target.as_node()]


    # the zzub.CvConnectorData object
    def get_cv_data(self):
        return self.cvdata


    def item_selected(self, port_type, is_target, index):
        connector = self.target if is_target else self.source

        if connector.type is not None:
            group = self.get_group(connector.type, connector.is_target)
            group.remove_highlight(connector.value)

        connector.update(port_type, index)


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
            [audio_ports, midi_ports, parameter_ports, track_ports] = [ports.audio_in_ports, ports.midi_in_ports, ports.parameter_in_ports, ports.track_in_ports]
        else:
            [audio_ports, midi_ports, parameter_ports, track_ports] = [ports.audio_out_ports, ports.midi_out_ports, ports.parameter_in_ports, ports.track_in_ports]

        if len(audio_ports) > 0:
            sub_grids.append(self.build_audio_group(grid, audio_ports, is_target, "audio %s" % suffix))

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


