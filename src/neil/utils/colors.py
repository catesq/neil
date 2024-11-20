from neil.utils import PluginType, plugin_color_schemes


import zzub


# class ColorTypes:
#     DEFAULT = 0
#     MUTED = 1
#     LED_OFF = 2
#     LED_ON = 3
#     LED_BORDER = 4
#     LED_WARNING = 7
#     CPU_OFF = 2
#     CPU_ON = 3
#     CPU_BORDER = 4
#     CPU_WARNING = 4
#     BORDER_IN = 8
#     BORDER_OUT = 8
#     BORDER_SELECT = 8
#     TEXT = 9

class ColorType:
    Default = 0
    Muted = 1
    LedOff = 3
    LedOn = 2
    IndicatorBg = 4
    IndicatorFg = 5
    IndicatorBorder = 6
    IndicatorWarning = 7
    Border = 8
    Text = 9
    Background = 10
    Line = 11


color_keys = [
    'MV ${PLUGIN}',
    'MV ${PLUGIN} Mute',
    'MV ${PLUGIN} LED Off',
    'MV ${PLUGIN} LED On',
    'MV Indicator Background',
    'MV Indicator Foreground',
    'MV Indicator Border',
    'MV Indicator Warning',
    'MV Border',
    'MV Text',
    'MV Background',
    'MV Line'
]

# this has the colour options from 
class RouterColors:
    def __init__(self, config):
        self.config = config

        # one of neil.utils.PluginType constant
        self.plugin_type = None

        # each scheme has the colors listed in color_keys
        self.plugin_scheme = None

        # all the colors schemes
        self.plugin_schemes = None

        #either zzub.zzub_connection_type_cv or zzub_connection_type_audio
        self.arrow_type = None
        #
        self.arrow_colors = None
        
        self.refresh()
        

    def refresh(self):
        self.weight_rgb = (1,1,1)
        self.plugin_schemes = self.rebuild_plugin_colors()
        self.arrow_colors = self.rebuild_arrow_colors()
        self.set_plugin_type(PluginType.Instrument)


    def rebuild_plugin_colors(self):
        all_schemes = []
        for plugintype, name in plugin_color_schemes.items():
            plugin_scheme = []
            for name in [x.replace('${PLUGIN}', name) for x in color_keys]:
                plugin_scheme.append(self.config.get_float_color(name))
            all_schemes.append(plugin_scheme)

        return all_schemes
    

    def rebuild_arrow_colors(self):
        return [
            [
                self.config.get_float_color("MV Controller Arrow"),
                self.config.get_float_color("MV Controller Arrow Border In"),
                self.config.get_float_color("MV Controller Arrow Border Out"),
            ],
            [
                self.config.get_float_color("MV Arrow"),
                self.config.get_float_color("MV Arrow Border In"),
                self.config.get_float_color("MV Arrow Border Out"),
            ],
        ]


    def set_plugin_type(self, plugin_type: PluginType):
        self.plugin_type = plugin_type
        self.plugin_scheme = self.plugin_schemes[plugin_type]


    def set_audio_connection(self, is_audio):
        self.arrow_type = zzub.zzub_connection_type_audio if is_audio else zzub.zzub_connection_type_cv
        self.arrow_scheme = self.arrow_schemes[self.arrow_type]

    def default(self, is_muted, weight = None):
        color_name = ColorType.Muted if is_muted else ColorType.Default

        if weight is None:
            return self.plugin_scheme[color_name]
        else:
            return self.shade(color_name, weight)


    #
    def shade(self, color, weight):
        if isinstance(color, int):
            color = self.plugin_scheme[color]

        if weight >= 0:
             # lighten rgb color
            return (col + (1 - col) * weight for col in color)
        else:
            weight = -weight
            return (col * weight for col in color)

    def arrow(self, is_audio, amp=1):
        return self.shade(self.arrow_colors[is_audio][0], -1 + amp)

    def background(self):
        return self.plugin_scheme[ColorType.Background]
    
    def line(self):
        return self.plugin_scheme[ColorType.Line]

    def arrow_in(self, is_audio, amp=1):
        return self.shade(self.arrow_colors[is_audio][1], -1 + amp)


    def arrow_out(self, is_audio, amp=1):
        return self.shade(self.arrow_colors[is_audio][2], -1 + amp)


    def plugin(self):
        return self.plugin_scheme[ColorType.Default]
    

    def muted(self):
        return self.plugin_scheme[ColorType.Muted]


    def led_on(self):
        return self.plugin_scheme[ColorType.LedOn]


    def led_off(self):
        return self.plugin_scheme[ColorType.LedOff]


    def led_warning(self):
        return self.plugin_scheme[ColorType.IndicatorWarning]
    

    def cpu_on(self):
        return self.plugin_scheme[ColorType.LedOn]


    def cpu_off(self):
        return self.plugin_scheme[ColorType.LedOff]


    def cpu_border(self):
        return self.plugin_scheme[ColorType.IndicatorBg]


    def cpu_warning(self):
        return self.plugin_scheme[ColorType.IndicatorWarning]


    def border(self):
        return self.plugin_scheme[ColorType.Border]


    def border_in(self):
        return self.plugin_scheme[ColorType.Border]


    def border_out(self):
        return self.plugin_scheme[ColorType.Border]


    def text(self):
        return self.plugin_scheme[ColorType.Text]


__all__ = [
    'RouterColors'
]