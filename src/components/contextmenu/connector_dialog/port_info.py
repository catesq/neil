import zzub
from typing import List


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


