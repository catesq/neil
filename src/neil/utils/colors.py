from .plugin import PluginType, get_plugin_type
import zzub


plugin_color_group = {
    PluginType.Root: "Master",
    PluginType.Instrument: "Generator",
    PluginType.Effect: "Effect",
    PluginType.CV: "Controller",
    PluginType.Controller: "Effect",
    PluginType.Streamer: "Generator",
    PluginType.Other: "Effect",
}


plugin_led_group = {
    PluginType.Root: "Master",
    PluginType.Instrument: "Generator",
    PluginType.Effect: "Effect",
    PluginType.CV: "Generator",
    PluginType.Controller: "Effect",
    PluginType.Streamer: "Effect",
    PluginType.Other: "Effect",
}


def get_plugin_color_key(plugin: zzub.Pluginloader | zzub.Plugin, suffix: str | bool=False):
    if suffix:
        return plugin_color_group[get_plugin_type(plugin)] + " " + suffix.strip()
    else:
        return plugin_color_group[get_plugin_type(plugin)]
    


def get_plugin_color_group(plugin: zzub.Pluginloader | zzub.Plugin):
    return plugin_color_group[get_plugin_type(plugin)] 

theme_properties = [
    # machine view/router colors
    "MV Background", 
    "MV Text",

    "MV Line",

    "MV Amp Handle", 
    "MV Amp BG", 

    "MV Border",

    "MV Arrow",             "MV Arrow Border In",            "MV Arrow Border Out",
    "MV Controller Arrow",  "MV Controller Arrow Border In", "MV Controller Arrow Border Out", 

    "MV Generator",        "MV Generator Mute",
    "MV Effect",           "MV Effect Mute",
    "MV Master",           "MV Master Mute",
    "MV Controller",       "MV Controller Mute",

    "MV Generator LED Off", "MV Generator LED On",
    "MV Effect LED Off",    "MV Effect LED On",
    "MV Master LED Off",    "MV Master LED On",

    # Pattern colors
    "PE BG Very Dark", "PE BG Dark", "PE BG", "PE BG Light", "PE BG Very Light",

    "PE Sel BG",
    "PE Text",
    "PE Track Numbers",
    "PE Row Numbers",

    # seqencer colors
    "SE Background",
    "SA Amp BG", "SA Amp Line",
    "SA Freq BG", "SA Freq Line",
    "SE Weak Line","SE Strong Line",
    "SE Text",
    "SE Loop Line",
    "SE Track Background",

    # "WE BG",
    # "WE Line",
    # "WE Fill",
    # "WE Peak Fill",
    # "WE Grid",
    # "WE Selection",
    # "WE Stretch Cue",
    # "WE Split Bar",
    # "WE Slice Bar",
    # "WE Wakeup Peaks",
    # "WE Sleep Peaks",

    # "EE Line",
    # "EE Fill",
    # "EE Dot",
    # "EE Sustain",
    # "EE Dot Selected",
    # "EE BG",
    # "EE Grid"
]



class BaseColors:
    def shade(self, color, weight, *args, **kwargs):
        if isinstance(color, str):
            if color in self.colors:
                color = self.colors[color]

        if weight >= 0:       # lighter rgb color
            weight = min(1, weight)
            return (col + (1 - col) * weight for col in color)
        else:                  # darker rgb color
            weight = -max(-1, weight)
            return (col - (col * weight) for col in color)





class RouterColors(BaseColors):
    def __init__(self, colors):
        self.colors = colors

    def amp(self, is_handle):
        return self.colors["MV Amp Handle" if is_handle else "MV Amp BG"]

    def arrow(self, is_audio, is_border=False, border_direction=False):
        prefix = "MV Arrow" if is_audio else "MV Controller Arrow"
        suffix = (" Border In" if border_direction else " Border Out") if is_border else ""
        return self.colors[prefix + suffix]

    def plugin(self, type, is_selected=False, is_muted=False):
        prefix = "MV " + plugin_color_group[type]
        suffix = " Mute" if is_muted else ""
        key = prefix + suffix
        return self.shade(key, 0.2) if is_selected else self.colors[key]

    def text(self):
        return self.colors["MV Text"]

    def plugin_border(self, type, is_selected=False):
        key = "MV " + plugin_color_group[type]
        shade = -0.2 if is_selected else -0.5
        return self.shade(key, shade)

    def border(self):
        return self.colors["MV Border"]
    
    #state 0=off, 1=on 2=warning
    def led(self, type, state):
        if state == 2:
            return self.colors["MV Machine LED Warning"]

        prefix = "MV " + plugin_led_group[type]
        suffix = " LED On" if state else " LED Off"
        return self.colors[prefix + suffix]
    
    def background(self):
        return self.colors["MV Background"]

    def line(self):
        return self.colors["MV Line"]
    


class PatternColors(BaseColors):
    shades = ["PE BG Very Dark", "PE BG Dark", "PE BG", "PE BG Light", "PE BG Very Light"]

    def __init__(self, colors):
        self.colors = colors


    def background(self, shading=0, is_selected=False):
        if is_selected:
            return self.colors["PE Sel BG"]
        
        shading = max(-2, min(2, shading)) + 2
        shade = PatternColors.shades[shading]
        
        return self.colors[shade]

    def text(self, is_selected=False):
        return self.colors["PE Text"]

    def track_numbers(self):
        return self.colors["PE Track Numbers"]

    def row_numbers(self):
        return self.colors["PE Row Numbers"]



class SequencerColors(BaseColors):
    line_shades = ["SE Weak Line", "SE Strong Line"]

    def __init__(self, colors):
        self.colors = colors

    #darknen = 0 to +2
    def background(self, darknen=0, is_selected=False):
        if is_selected:
            darken = 3
        
        shade = min(0, 3)
        
        return self.shade("SE Background", darken)
#
    def text(self):
        return self.colors["SE Text"]

    def loop_line(self):
        return self.colors["SE Loop Line"]

    def track_background(self):
        return self.colors["SE Track Background"]
    

    def line(self, is_light: False):
        strength_name = PatternColors.line_shades[bool(is_light)]
        return self.colors[strength_name]


class Colors(BaseColors):
    def __init__(self, config):
        self.config = config
        self.colors = {}
        self.router = RouterColors(self.colors)
        self.sequencer = SequencerColors(self.colors)
        self.pattern = PatternColors(self.colors)
        self.refresh()

    def refresh(self):
        for color in theme_properties:
            self.colors[color] = self.config.get_float_color(color)


__all__ = [
    'Colors',
    'PatternColors',
    'SequencerColors',
    'RouterColors',
    'get_plugin_color_key',
    'get_plugin_color_group',
]