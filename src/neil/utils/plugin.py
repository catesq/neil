from enum import IntEnum
import zzub

class PluginType(IntEnum):
    Root = 0
    Instrument = 1
    Effect = 2
    CV = 3
    Controller = 4
    Streamer = 5
    Other = 6



# used by PluginGfx and ConnectionGfx in router view to cache small graphics drawn onto a large canvas 



plugin_type_names = {
    PluginType.Root: "Root",
    PluginType.Instrument: "Instrument",
    PluginType.Effect: "Effect",
    PluginType.CV: "CV generator",
    PluginType.Controller: "Controller",
    PluginType.Streamer: "Streamer",
    PluginType.Other: "Other",
}




# AUDIO_IO_FLAGS = zzub.zzub_plugin_flag_has_audio_input | zzub.zzub_plugin_flag_has_audio_output
# EVENT_IO_FLAGS = zzub.zzub_plugin_flag_has_event_output


adapters = {
    "lv2adapter": "lv2", 
    "ladspadapter": "ladspa", 
    "dssidapter": "dssi", 
    "vstadapter": "vst2",
    "vst3adapter": "vst3",
}


def get_adapter_name(pluginloader: zzub.Pluginloader):
    # get the adapter name: lv2, vst, vst3, ladspa or zzub
    # from the internal machine url: '@zzub.org/adapter_name/external_plugin_name'
    name = pluginloader.get_loader_name()
    typename = name[10:name.find("/", 10)]
    if typename in adapters.keys():
        return adapters[typename]

    return "zzub"



def get_plugin_type(plugin: zzub.Pluginloader | zzub.Plugin):
    flags = plugin.get_flags()

    if flags & zzub.zzub_plugin_flag_is_instrument:
        return PluginType.Instrument
    elif flags & zzub.zzub_plugin_flag_is_effect:
        return PluginType.Effect
    elif flags & zzub.zzub_plugin_flag_is_cv_generator:
        return PluginType.CV
    elif flags & zzub.zzub_plugin_flag_is_root:
        return PluginType.Root
    elif flags & zzub.zzub_plugin_flag_control_plugin:
        return PluginType.Controller
    elif flags & zzub.zzub_plugin_flag_stream:
        return PluginType.Streamer

    return PluginType.Other




def is_other(plugin: zzub.Pluginloader | zzub.Plugin):
    return not (is_effect(plugin) or is_a_generator(plugin) or is_root(plugin))

def has_audio_input(plugin: zzub.Pluginloader | zzub.Plugin):
    return plugin.get_flags() & zzub.zzub_plugin_flag_has_audio_input 


def has_audio_output(plugin: zzub.Pluginloader | zzub.Plugin):
    return plugin.get_flags() & zzub.zzub_plugin_flag_has_audio_output


def has_cv_output(plugin: zzub.Pluginloader | zzub.Plugin):
    return plugin.get_flags() & zzub.zzub_plugin_flag_has_cv_output | plugin.get_flags() & zzub.zzub_plugin_flag_is_cv_generator


def has_output(plugin: zzub.Pluginloader | zzub.Plugin):
    return plugin.get_flags() & (
        zzub.zzub_plugin_flag_has_cv_output | 
        zzub.zzub_plugin_flag_has_audio_output | 
        zzub.zzub_plugin_flag_has_midi_output 
    )

def is_effect(plugin: zzub.Pluginloader | zzub.Plugin):
    return plugin.get_flags() & zzub.zzub_plugin_flag_is_effect 



def is_instrument(plugin: zzub.Pluginloader | zzub.Plugin):
    return plugin.get_flags() & zzub.zzub_plugin_flag_is_instrument



def is_cv_generator(plugin: zzub.Pluginloader | zzub.Plugin):
    return plugin.get_flags() & zzub.zzub_plugin_flag_is_cv_generator



def is_a_generator(plugin: zzub.Pluginloader | zzub.Plugin):
    return plugin.get_flags() & zzub.zzub_plugin_flag_is_instrument or (plugin.get_flags() & zzub.zzub_plugin_flag_is_cv_generator)



# def is_controller(plugin: zzub.Pluginloader | zzub.Plugin):
#     return plugin.get_flags() & zzub.zzub_plugin_flag_control_plugin or ((plugin.get_flags() & EVENT_IO_FLAGS) and not (plugin.get_flags() & AUDIO_IO_FLAGS))



def is_root(plugin: zzub.Pluginloader | zzub.Plugin):
    return plugin.get_flags() & zzub.zzub_plugin_flag_is_root



def is_streamer(plugin: zzub.Pluginloader | zzub.Plugin):
    return plugin.get_flags() & zzub.zzub_plugin_flag_stream



# used in the router view
def rename_plugin(player, plugin):
    num = 1
    name = plugin.get_name() + f"_{num}"

    while name in [plugin.get_name() for plugin in player.get_plugin_list()]:
        name = plugin.get_name() + f"_{num}"
        num += 1

    return name



def clone_plugin(player, src_plugin):
    new_plugin = player.create_plugin(src_plugin.get_pluginloader())
    new_plugin.set_name(rename_plugin(player, src_plugin))
    return new_plugin



def clone_plugin_and_patterns(player, src_plugin, new_plugin):
    new_plugin = clone_plugin(player, src_plugin)
    clone_plugin_patterns(player, src_plugin, new_plugin)



def clone_preset(player, src_plugin, new_plugin):
    # this import has to be here to avoid circular import
    from neil.preset import Preset
     
    preset = Preset()
    preset.pickup(src_plugin)
    preset.apply(new_plugin)
    player.history_commit("Clone plugin %s" % src_plugin.get_pluginloader().get_short_name())



def clone_plugin_patterns(plugin, new_plugin):
    new_plugin.set_track_count(plugin.get_track_count())

    for index, pattern in [(index, plugin.get_pattern(index)) for index in range(plugin.get_pattern_count())]:
        new_pattern = new_plugin.create_pattern(pattern.get_row_count())
        new_pattern.set_name(pattern.get_name())

        for group in range(pattern.get_group_count()):
            for track in range(pattern.get_track_count(group)):
                for row in range(pattern.get_row_count()):
                    for column in range(pattern.get_column_count(group, track)):
                        val = pattern.get_value(row, group, track, column)
                        new_pattern.set_value(row, group, track, column, val)

        new_plugin.add_pattern(new_pattern)



__all__ = [
    'PluginType',
    'get_adapter_name',
    'get_plugin_type',
    'plugin_type_names',
    'is_other',
    'is_effect',
    'is_instrument',
    'is_a_generator',
    'is_cv_generator',
    'is_root',
    'is_streamer',
    'has_audio_input',
    'has_audio_output',
    'has_cv_output',
    'has_output',
    'rename_plugin',
    'clone_plugin',
    'clone_plugin_and_patterns',
    'clone_preset',
    'clone_plugin_patterns',
]
