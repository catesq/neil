from typing import List
import itertools, zzub

import gi
gi.require_version("Gtk", "3.0")

from gi.repository import Gtk

from zzub import Plugin



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
            ports = plugin.get_ports()
        else:
            ports = self.make_ports(plugin)
        # self.ports = ports

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

    
    def make_ports(self, mp) -> List[PortWrapper]:
        ports = []

        if mp.get_flags() & zzub.zzub_plugin_flag_has_audio_input:
            ports.append(PortWrapper("Audio In Left", zzub.zzub_port_flow_input, 0, zzub.zzub_port_type_audio))
            ports.append(PortWrapper("Audio In Right", zzub.zzub_port_flow_input, 1, zzub.zzub_port_type_audio))

        if mp.get_flags() & zzub.zzub_plugin_flag_has_audio_output:
            ports.append(PortWrapper("Audio Out Left", zzub.zzub_port_flow_output, 0, zzub.zzub_port_type_audio))
            ports.append(PortWrapper("Audio Out Right", zzub.zzub_port_flow_output, 1, zzub.zzub_port_type_audio))

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




# there are two audiogrids in the connect dialog, the left side is ports on source source, right side is ports for target plugin
class AudioGrid(Gtk.Grid):
    def __init__(self, title, port_count):
        Gtk.Grid.__init__(self)
        self.port_count = port_count

        # int used as a bitflag of which channels are selected
        self.selected = 0
        self.all_mask = (1 << port_count) - 1
        self.handler = None
        self.args = []

        if port_count == 0:
            return
        
        if port_count == 1:
            title = "mono " + title
            self.attach(BoxyLabel("mono" + title), 0, 0, 1, 1)
        elif port_count == 2:
            title = "stereo " + title
            self.make_grid(["L", "R"])
        elif port_count < 4:
            title = "%s (%d)" % (title, port_count)
            self.make_grid(range(port_count))

        label = BoxyLabel(title)
        # label.connect("button-release-event", self.toggle_all_channels)
        self.attach(label, 0, 0, port_count, 1)

    def make_grid(self, titles):
        for index, title in enumerate(titles):
            label = BoxyLabel(title)
            label.connect("button-release-event", self.toggle_channel, index)
            self.attach(label, index, 1, 1, 1)

    # the "L" or "R" button was clicked. toggle: channel on <-> channel off
    def toggle_channel(self, widget, event, channel):
        self.set_selected_value(self.selected ^ (1 << channel))

        if self.handler:
            self.handler(self.selected, *self.args)

    def set_selected_value(self, value):
        self.selected = value
        self.update_css_classes(value)

    def update_css_classes(self, value):
        for index in range(self.port_count):
            label = self.get_child_at(index, 1)

            if value & (1 << index):
                label.get_style_context().add_class("selected")
            else:
                label.get_style_context().remove_class("selected")

    def clear_all(self):
        self.selected = 0

        for index in range(self.port_count):
            channel_label = self.get_child_at(index, 1)
            channel_label.get_style_context().remove_class("selected")
            channel_label.set_state(Gtk.StateFlags.NORMAL)

    def set_handler(self, handler, *args):
        self.handler = handler
        self.args = args



     
# store selected port info to create a cvnode
# the connectdialog has two of these and they are update when user clicks a label in the dialog
class Connector:
    def __init__(self, plugin_id, is_source):
        self.type = None           # audio/parameter/cv
        self.value = None          # when parameter port it's the index of parameter. when audio port it's bitflag of audio channels where  1 = mono left, 2 = mono right, 3 = stereo
        self.is_source = is_source # this is whether it's the source or target plugin
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
                 from_plugin: Plugin, 
                 to_plugin: Plugin, 
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
        
        self.source = Connector(from_plugin.get_id(), True) 
        self.target = Connector(to_plugin.get_id(), False) 
        self.cvdata = zzub.CvConnectorData.create() if cvdata is None else cvdata

        def has_flag(plugin, flag):
            return plugin.get_flags() & flag

        self.target_ports = self.build_port_infos(to_plugin)
        self.source_ports = self.build_port_infos(from_plugin)

        has_audio = has_flag(to_plugin, zzub.zzub_plugin_flag_has_audio_input) or has_flag(from_plugin, zzub.zzub_plugin_flag_has_audio_output)

        # make gtk boxes for the sources and targets lists
        #   first box will be for audio input/output
        #   subsequent boxes for individual parameter + port
        sources = self.make_connectors(True, self.source_ports, has_audio)
        targets = self.make_connectors(False, self.target_ports, has_audio)

        self.grid = grid = Gtk.Grid()
        
        grid.attach(Gtk.Label("Source"), 0, 0, 1, 1)
        grid.attach(Gtk.Label("Targets"), 1, 0, 1, 1)

        for index, (source, target) in enumerate(itertools.zip_longest(sources, targets, fillvalue=False)):
            if not source:
                source = Gtk.Box()

            if not target:
                target = Gtk.Box()

            grid.attach(source,  0, index + 1, 1, 1)
            grid.attach(target, 1, index + 1, 1, 1)

        cancel = Gtk.Button("Cancel")
        ok = Gtk.Button("OK")
  
        grid.attach(cancel, 0, index + 2, 1, 1)
        grid.attach(ok, 1, index + 2, 1, 1)

        cancel.connect("clicked", lambda *args: self.response(Gtk.ResponseType.CANCEL))
        ok.connect("clicked", lambda *args: self.response(Gtk.ResponseType.OK))

        grid.set_column_spacing(1)

        self.get_content_area().add(grid)
        self.set_size_request(400, 500)
        self.show_all()

        if source_node and target_node:
            self.init_connector(source_node.get_type(), source_node.get_value(), True)
            self.init_connector(target_node.get_type(), target_node.get_value(), False)


    def build_port_infos(self, plugin):
        return PortInfo(plugin)


    def init_connector(self, cv_type, cv_value, is_source):
        connector = self.source if is_source else self.target
        connector.update(cv_type, cv_value)
        
        if cv_type == zzub.zzub_cv_node_type_audio:
            self.audio_channel_selected(cv_value, is_source)
            self.get_audio_connector_gridbox(is_source).set_selected_value(cv_value)
        elif cv_type ==  zzub.zzub_cv_node_type_global:
            self.parameter_selected(None, None, cv_value, is_source)
        else:
            print("unknown connector type", cv_type, "value", cv_value, is_source)


    def get_connectors(self):
        """
        get the selected source and target connectors
        only valid if dialog returns OK
        """
        return [self.source.as_node(), self.target.as_node()]

		# def get_amp(): float
		# def get_modulate_mode(): uint
		# def get_offset_before(): float
        # def get_offset_after(): float

    def get_cv_data(self):
        return self.cvdata
    
    def get_audio_connector_gridbox(self, is_source):
        return self.grid.get_child_at(0 if is_source else 1, 1)
    
    def get_parameter_gridbox(self, index, is_source):
        return self.grid.get_child_at(0 if is_source else 1, 2 + index)
    
    # messages from AudioGrid channel selector received here
    def audio_channel_selected(self, selected_channels, is_source):
        connector = self.source if is_source else self.target
        self.clear_selected(connector)
        connector.update(zzub.zzub_port_type_audio, selected_channels)



    # signals from button-release-event on a parameter 
    def parameter_selected(self, widget, event, index, is_source):
        (connector, column) = (self.source, 0) if is_source else (self.target, 1)

        self.clear_selected(connector, True)
        connector.update(zzub.zzub_port_type_parameter, index)
        self.get_parameter_gridbox(index, is_source).set_selected_value(True)


    # remove "selected" css class from the previously selected port        
    def clear_selected(self, prev_connector, clear_audio_selection=False):
        if prev_connector.value is None:
            return
        
        # column 0 is source connector. column 1 is target connector.
        col = 0 if prev_connector.is_source else 1

        if prev_connector.type == zzub.zzub_port_type_audio and clear_audio_selection:

            audio_grid = self.grid.get_child_at(col, 1)
            audio_grid.clear_all()

        elif prev_connector.type == zzub.zzub_port_type_parameter:

            row = 2 + prev_connector.value
            param = self.grid.get_child_at(col, row)
            param.get_style_context().remove_class("selected")


    # make a list of boxes for the plugin's audio and parameter ports 
    def make_connectors(self, is_source, ports: PortInfo, has_audio: bool) -> List[Gtk.ListBoxRow]:
        items = []

        if has_audio:
            if is_source:
                widget = self.make_audio_connectors("audio out", is_source, ports.audio_out_ports)
            else:
                widget = self.make_audio_connectors("audio in", is_source, ports.audio_in_ports)

            items.append(widget)

        for param in ports.parameter_in_ports:
            items.append(self.make_param_connector(is_source, param))

        return items
    
    # 
    def make_audio_connectors(self, title, is_source, ports: List[PortWrapper]):
        # pad the connectors so there's always the same number of items in the source and target lists
        # and the audio in/out/parameters line up visually
        if len(ports) == 0:
            return Gtk.Box()

        grid = AudioGrid(title, len(ports))
        grid.set_handler(self.audio_channel_selected, is_source)
        grid.get_style_context().add_class(title.replace(" ", "_"))

        return grid


    def make_param_connector(self, is_source, param: PortWrapper):
        box = BoxyLabel(param.get_name())
        box.connect("button-press-event", self.parameter_selected, param.get_index(), is_source)
        return box
    




class ConnectorInfo:
    def __init__(self, to_plugin_id, from_plugin_id, connection_id, connector_id):
        self.from_plugin = from_plugin_id
        self.to_plugin = to_plugin_id
        self.connection = connection_id
        self.connector = connector_id

    def get_from_plugin(self):
        return self.from_plugin

    def get_to_plugin(self):
        return self.to_plugin

    def get_connection(self):
        return self.connection

    def get_connector(self):
        return self.connector




# todo: finish
class ChooseConnectorDialog(Gtk.Dialog):
    def __init__(self, parent, plugin, conn_indexes, title, multiple):
        """"
        @param parent: parent window
        @param plugin: the metaplugin the connections are inputs
        @param connections: list of connection indexes
        @param title: title is basically edit or delete
        @param multiple: if true, multiple connections can be selected (edit is single, delete is multiple)
        """
        super().__init__(transient_for=parent, title=title)

        self.grid = Gtk.Grid()
        self.multiple = multiple
        self.selected = []

        self.build_grid(plugin, conn_indexes)
        
        self.get_content_area().add(self.grid)
        self.add_button(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL)
        self.add_button(Gtk.STOCK_OK, Gtk.ResponseType.OK)
        self.show_all()


    def get_selected(self):
        """
        @return list of ConnectorInfo 
        """
        return self.selected


    def toggle_selected_multiple(self, connector_info):
        if connector_info in self.selected:
            self.selected.remove(connector_info)
        else:
            self.selected.append(connector_info)


    def toggle_selected_single(self, connector_info):
        if len(self.selected) == 1 and self.selected[0] == connector_info:
            self.selected.clear()
        else:
            self.selected.append(connector_info)



    def build_grid(self, to_plugin, conn_indexes):
        row_count = 0
        label = Gtk.Label("Plugin: %s" % to_plugin.get_name())
        self.grid.attach(label, 0, row_count, 1, 1)

        built = 1
        for conn_index in conn_indexes:
            conn_type = to_plugin.get_input_connection_type(conn_index)

            if conn_type == zzub.zzub_connection_type_audio:
                built += self.build_grid_audio_connection(to_plugin, conn_index, row_count)
            elif conn_type == zzub.zzub_connection_type_cv:
                built += self.build_grid_cv_connection(to_plugin, conn_index, row_count)

        return built


    def build_grid_audio_connection(self, to_plugin, conn_index, row_count):
        """
        build a label like: "stereo audio from 'plugin_name'"
        """
        label = Gtk.Label("Plugin: %s" % to_plugin.get_name())
        self.grid.attach(label, 0, row_count, 1, 1)

        built = 1
        for conn_index in conn_index:
            built += self.build_grid_for_connection(to_plugin, conn_index)

        return built


    def build_grid_cv_connection(self, to_plugin, conn_index, row_count):
        """
        build a list of labels
        """
        from_plugin = to_plugin.get_input_connection_plugin(conn_index)
        connection = to_plugin.get_input_connection(conn_index).as_cv_connection()
        connector_count = connection.get_connector_count()

        for i in enumerate(connector_count):
            connector = connection.get_connector(from_plugin, to_plugin, i)
            label = self.build_cv_connector_label(from_plugin, to_plugin, connector)
            self.grid.attach(label, 0, i, 1, 1)

        return connector_count


    def build_cv_connector_label(self, from_plugin, to_plugin, connector):
        """
        build a label like: "cv audio/cv paramater data from 'port name' of 'plugin_name'"
        """
        node_from = self.describe_node(from_plugin, connector.get_source())
        node_to = self.describe_node(to_plugin, connector.get_target())

        return Gtk.Label(f"{node_from} -> {node_to}")


    def describe_connector_node(self, plugin, node):
        if node.type == zzub.zzub_cv_node_type_audio:
            return "cv audio %s of %s" % (
                self.describe_cv_audio_node(node), 
                plugin.get_name()
            )
        elif node.type == zzub.zzub_cv_node_type_track_parameter:
            return "cv parameter %s of %s" % (
                self.describe_cv_parameter_node(plugin, node, zzub.zzub_parameter_group_track), 
                plugin.get_name()
            )
        elif node.type == zzub.zzub_cv_node_type_global_parameter:
            return "cv parameter %s of %s" % (
                self.describe_cv_parameter_node(plugin, node, zzub.zzub_parameter_group_global), 
                plugin.get_name()
            )
        elif node.type == zzub.zzub_cv_node_type_port_node:
            return "cv port %s %d of %s" % node.value
        elif node.type == zzub.zzub_cv_node_type_midi_node:
            return "cv midi %s %d of %s" % node.value
        else:
            return "unknown link"


    def describe_cv_audio_node(self, node):
        if node.value == 1:
            return "left channel"
        elif node.value == 2:
            return "right channel"
        elif node.value == 3:
            return "stereo"
        else:
            return "channels %d" % node.value
        

    def describe_cv_parameter_node(self, plugin, node, parameter_group):
        return plugin.get_pluginloader().get_parameter(parameter_group, node.value).get_short_name()

