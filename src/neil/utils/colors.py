from .plugin import PluginType, get_plugin_type
import zzub
from typing import TYPE_CHECKING, Optional

from neil.common import PluginInfo


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



def get_plugin_color_key(plugin: zzub.Pluginloader | zzub.Plugin, suffix: Optional[str] = None, prefix: Optional[str] = None):
    prefix = prefix.strip() + ' ' if prefix else ''
    suffix = ' ' + suffix.strip() if suffix else ''
        
    return prefix + plugin_color_group[get_plugin_type(plugin)] + suffix
    


def get_plugin_color_group(plugin: zzub.Pluginloader | zzub.Plugin):
    return plugin_color_group[get_plugin_type(plugin)] 



def blend_float(rgb1, rgb2, weight = 0.5):
    return [
        rgb1[0] * weight + rgb2[0] * (1 - weight),
        rgb1[1] * weight + rgb2[1] * (1 - weight),
        rgb1[2] * weight + rgb2[2] * (1 - weight)
    ]

def blend(color1, color2, weight = 0.5):
    """
        Blend (lerp) two Gdk.Colors
    """
    return [
        color1.red_float   * weight + color2.red_float   * (1 - weight),
        color1.green_float * weight + color2.green_float * (1 - weight),
        color1.blue_float  * weight + color2.blue_float  * (1 - weight)
    ]

def from_hsb(h=0.0,s=1.0,b=1.0):
    """
    Converts hue/saturation/brightness into red/green/blue components.
    """
    if not s:
        return b,b,b
    scaledhue = (h%1.0)*6.0
    index = int(scaledhue)
    fraction = scaledhue - index
    p = b * (1.0 - s)
    q = b * (1.0 - s*fraction)
    t = b * (1.0 - s*(1.0 - fraction))
    if index == 0:
        return b,t,p
    elif index == 1:
        return q,b,p
    elif index == 2:
        return p,b,t
    elif index == 3:
        return p,q,b
    elif index == 4:
        return t,p,b
    elif index == 5:
        return b,p,q
    return b,p,q

def to_hsb(r,g,b):
    """
    Converts red/green/blue into hue/saturation/brightness components.
    """
    if (r == g) and (g == b):
        h = 0.0
        s = 0.0
        b = r
    else:
        v = float(max(r,g,b))
        temp = float(min(r,g,b))
        diff = v - temp
        if v == r:
            h = (g - b)/diff
        elif v == g:
            h = (b - r)/diff + 2
        else:
            h = (r - g)/diff + 4
        if h < 0:
            h += 6
        h = h / 6.0
        s = diff / v
        b = v
    return h,s,b


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
    colors:dict[str, tuple[float,float,float]]


    def shade(self, color: str | tuple[float, float, float], weight: int | float, *args, **kwargs) -> tuple[float, float, float]:
        if isinstance(color, str):
            if color in self.colors:
                color = self.colors[color]
            else:
                return (0, 0, 0)

        if weight >= 0:       # lighter rgb color
            weight = min(1, weight)
        else:                # darker rgb color
            weight = -max(-1, weight)

        return (color[0] + (1 - color[0]) * weight,
                color[1] + (1 - color[1]) * weight,
                color[2] + (1 - color[2]) * weight)





class RouterColors(BaseColors):
    def __init__(self, colors):
        self.colors = colors


    def amp(self, is_handle):
        return self.colors["MV Amp Handle" if is_handle else "MV Amp BG"]


    def arrow(self, is_audio, is_border=False, border_direction=False):
        prefix = "MV Arrow" if is_audio else "MV Controller Arrow"
        suffix = (" Border In" if border_direction else " Border Out") if is_border else ""
        return self.colors[prefix + suffix]


    def plugin(self, type: PluginType | PluginInfo, shade_or_selected:float | bool = False, muted=False):
        if isinstance(type, PluginInfo):
            shade_or_selected = type.selected
            muted = type.muted
            type = type.type

        prefix = "MV " + plugin_color_group[type]
        suffix = " Mute" if muted else ""
        key = prefix + suffix

        if isinstance(shade_or_selected, float):
            return self.shade(key, shade_or_selected)
        elif shade_or_selected:
            return self.shade(key, 0.2)
        else:
            return self.colors[key]


    def text(self):
        return self.colors["MV Text"]


    def plugin_border(self, type : PluginType | PluginInfo, is_selected=False):
        if isinstance(type, PluginInfo):
            is_selected = type.selected
            type = type.type

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
    def background(self, darken=0, is_selected=False):
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

    # def line(self, is_light = False):
    #     strength_name = PatternColors.line_shades[bool(is_light)]
    #     return self.colors[strength_name]


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