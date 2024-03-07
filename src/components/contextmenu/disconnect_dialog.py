from typing import List
import gi
import itertools, zzub

from zzub import Plugin

gi.require_version("Gtk", "3.0")

from gi.repository import Gtk


# used to store port information
# the lv2 plugin have ports with a matching api - 
# zzub plugins have no concept of ports (yet) so portwrapper is used to mimic the zzub::port api using the zzub::paramaters and audio in/out channels which zzub::plugins do have 
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


# 
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



# used to display port name and change colour when clicked
# can't use a label as they inherit background color which can't be changed
class BoxyLabel(Gtk.EventBox):
    def __init__(self, text):
        Gtk.EventBox.__init__(self, expand=True)
        self.label = Gtk.Label(text)
        self.add(self.label)


# build two columns. the left side has all the ports on the source plugin, the right side the target plugin
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
        label.connect("button-release-event", self.toggle_all_channels)
        self.attach(label, 0, 0, port_count, 1)

    def make_grid(self, titles):
        for index, title in enumerate(titles):
            label = BoxyLabel(title)
            label.connect("button-release-event", self.toggle_one_channel, index)
            self.attach(label, index, 1, 1, 1)

    # the "L" or "R" button was clicked. toggle: channel on <-> channel off
    def toggle_one_channel(self, widget, event, channel):
        return self.toggle_channels_mask(1 << channel)

    # the tiitle was clicked. toggle: all channels on <-> all channels off
    def toggle_all_channels(self, widget, event): 
        return self.toggle_channels_mask((~self.selected) & self.all_mask)

    def toggle_channels_mask(self, mask):
        self.selected ^= mask

        for index in range(self.port_count):
            label = self.get_child_at(index, 1)

            if self.selected & (1 << index):
                label.get_style_context().add_class("selected")
            else:
                label.get_style_context().remove_class("selected")

        if self.handler:
            self.handler(self.selected, *self.args)

    def clear_all(self):
        self.selected = 0

        for index in range(self.port_count):
            channel_label = self.get_child_at(index, 1)
            channel_label.get_style_context().remove_class("selected")
            channel_label.set_state(Gtk.StateFlags.NORMAL)

    def set_handler(self, handler, *args):
        self.handler = handler
        self.args = args


     
# store current port selection - the ConnectDialog uses two of these for the source and target plugins 
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
        return zzub.CvNode(
            self.plugin_id,
            self.type if self.type is not None else zzub.zzub_port_type_parameter,
            self.value if self.value is not None else 0
        )


class ConnectDialog(Gtk.Dialog):
    # __popup__ = dict(
    #     label = "Connect plugin",
    #     where = "router.connect.dialog",
    #     default = True,
    #     singleton = False,
    # )

    def __init__(self, parent, from_plugin: Plugin, to_plugin: Plugin):
        """
        Show lists of all ports on the souce and target plugin
        parent: the parent window
        from_plugin: the plugin to connect from
        to_plugin: the plugin to connect to
        """
        Gtk.Dialog.__init__(self, transient_for = parent.get_toplevel(), title = "Connect", name="connector")
        
        self.source = Connector(from_plugin.get_id(), True)
        self.target = Connector(to_plugin.get_id(), False)

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

            grid.attach(source, 0, index + 1, 1, 1)
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

    
    def build_port_infos(self, plugin):
        return PortInfo(plugin)


    # get the selected source and target connectors
    # only valid if dialog returns OK
    def get_connectors(self):
        return [self.source.as_node(), self.target.as_node()]
    

    # messages from AudioGrid channel selector received here
    def audio_channel_selected(self, selected_channels, is_source):
        connector = self.source if is_source else self.target

        self.clear_selected(connector)
        connector.update(zzub.zzub_port_type_audio, selected_channels)


    # signals from button-release-event on a parameter 
    def parameter_selected(self, widget, event, is_source, index):
        (connector, column) = (self.source, 0) if is_source else (self.target, 1)

        self.clear_selected(connector, True)
        connector.update(zzub.zzub_port_type_parameter, index)

        self.grid.get_child_at(column, 2 + index).get_style_context().add_class("selected")


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
        box.connect("button-press-event", self.parameter_selected, is_source, param.get_index())
        return box
    







class DisconnectDialog(Gtk.Dialog):
    # __popup__ = dict(
    #     label = "Disconnect plugin",
    #     where = "router.disconnect.dialog",
    #     default = True,
    #     singleton = False,
    # )

    def __init__(self, parent, connections):
        super().__init__(transient_for=parent, title=self.__popup__.label)

        self.connections = connections

        self.store = Gtk.ListStore(str, str, int)
        for target_mp, index in connections:
            from_mp = target_mp.get_input_connection_plugin(index)

            self.from_plugin = from_mp.get_id()
            self.to_plugin = target_mp.get_id()
            self.conn_index = index

            self.store.append([target_mp.get_name(), from_mp.get_Name(), index])

        self.tree = Gtk.TreeView(model=self.store)

        for i, column_title in enumerate(["To", "From", "Index"]):
            renderer = Gtk.CellRendererText()
            column = Gtk.TreeViewColumn(column_title, renderer, text=i)
            self.tree.append_column(column)

        sel = self.tree.get_selection()
        sel.set_mode(Gtk.SelectionMode.MULTIPLE)
        sel.connect('changed', self.on_selection_changed)

        self.add_buttons(
            Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_OK, Gtk.ResponseType.OK
        )


    def on_selection_changed(self, selection):
        for row in selection.gtk_tree_selection_get_selected_rows(self.store):
            print(row)


    # return the connection index of the selected items or an empty
    def get_selected_indexes(self):
        return self.listbox.get_selected_indexes()
