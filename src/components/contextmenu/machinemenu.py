from gi.repository import Gtk

import zzub
from neil.com import com
from neil.utils import (Menu, is_generator, is_root, is_effect, iconpath, prepstr)
from neil.preset import Preset
from .actions import on_popup_bypass
import os.path

class Connection:
    def __init__(self, metaplugin, connection_id):
        self.metaplugin = metaplugin
        self.id = connection_id


class MachineMenu(Menu):
    plugin_tree = {
        '@krzysztof_foltman/generator/infector;1': ['Synthesizers', 'Subtractive'],
        'jamesmichaelmcdermott@gmail.com/generator/primifun;1': ['Synthesizers', 'Subtractive'],
        '@cameron_foale/generator/green_milk;1': ['Synthesizers', 'Subtractive'],
        '@makk.org/M4wII;1': ['Synthesizers', 'Subtractive'],
        '@libneil/oomek/generator/aggressor': ['Synthesizers', 'Subtractive'],
        '@libneil/mda/generator/jx10': ['Synthesizers', 'Subtractive'],
        'jamesmichaelmcdermott@gmail.com/generator/4fm2f;1': ['Synthesizers', 'FM'],
        '@libneil/somono/generator/fm303;1': ['Synthesizers', 'FM'],
        '@libneil/mda/generator/dx10': ['Synthesizers', 'FM'],
        'jamesmichaelmcdermott@gmail.com/generator/pluckedstring;1': ['Synthesizers', 'Physical Modelling'],
        'jamesmichaelmcdermott@gmail.com/generator/dynamite6;1': ['Synthesizers', 'Physical Modelling'],
        '@mda-vst/epiano;1': ['Synthesizers', 'Physical Modelling'],
        '@libneil/fsm/generator/kick_xp': ['Synthesizers', 'Percussive'],
        '@libneil/somono/generator/cloud;1': ['Synthesizers', 'Granular'],
        'jamesmichaelmcdermott@gmail.com/generator/DTMF_1;1': ['Synthesizers', 'Other'],
        '@rift.dk/generator/Matilde+Tracker;1.5': ['Samplers'],
        '@trac.zeitherrschaft.org/aldrin/lunar/effect/delay;1': ['Effects', 'Time based'],
        '@trac.zeitherrschaft.org/aldrin/lunar/effect/phaser;1': ['Effects', 'Time based'],
        '@trac.zeitherrschaft.org/aldrin/lunar/effect/reverb;1': ['Effects', 'Time based'],
        '@libneil/somono/effect/mverb': ['Effects', 'Time based'],
        '@libneil/somono/effect/mdaDubDelay': ['Effects', 'Time based'],
        '@mda/effect/mdaThruZero;1': ['Effects', 'Time based'],
        '@libneil/somono/effect/chebyshev;1': ['Effects', 'Distortion'],
        '@libneil/arguru/effect/distortion': ['Effects', 'Distortion'],
        'graue@oceanbase.org/effect/softsat;1': ['Effects', 'Distortion'],
        '@mda/effect/mdaBandisto;1': ['Effects', 'Distortion'],
        '@neil/lunar/effect/bitcrusher;1': ['Effects', 'Distortion'],
        '@bblunars/effect/mdaDegrade': ['Effects', 'Distortion'],
        '@libneil/edsca/effect/Migraine': ['Effects', 'Distortion'],
        '@bblunars/effect/mdaSubSynth': ['Effects', 'Distortion'],
        '@libneil/mda/effect/combo': ['Effects', 'Distortion'],
        '@libneil/mda/effect/leslie': ['Effects', 'Modulation'],
        '@bigyo/frequency+shifter;1': ['Effects', 'Modulation'],
        'jamesmichaelmcdermott@gmail.com/effect/btdsys_ringmod;1': ['Effects', 'Modulation'],
        'jamesmichaelmcdermott@gmail.com/effect/modulator;1': ['Effects', 'Modulation'],
        '@libneil/mrmonkington/effect/mcp_chorus': ['Effects', 'Modulation'],
        '@libneil/arguru/effect/compressor': ['Effects', 'Dynamics'],
        '@binarywerks.dk/multi-2;1': ['Effects', 'Dynamics'],
        '@libneil/mda/effect/transient': ['Effects', 'Dynamics'],
        '@libneil/mda/effect/multiband': ['Effects', 'Dynamics'],
        '@libneil/mda/effect/dynamics': ['Effects', 'Dynamics'],
        '@FireSledge.org/ParamEQ;1': ['Effects', 'Filter'],
        '@trac.zeitherrschaft.org/aldrin/lunar/effect/philthy;1': ['Effects', 'Filter'],
        'jamesmichaelmcdermott@gmail.com/effect/dffilter;1': ['Effects', 'Filter'],
        '@libneil/somono/effect/filter': ['Effects', 'Filter'],
        '@libneil/mda/effect/vocoder': ['Effects', 'Filter'],
        '@libneil/mda/effect/talkbox': ['Effects', 'Filter'],
        '@libneil/mda/effect/rezfilter': ['Effects', 'Filter'],
        'jamesmichaelmcdermott@gmail.com/effect/sprayman;1': ['Effects', 'Sampling'],
        '@libneil/somono/effect/stutter;1': ['Effects', 'Sampling'],
        '@libneil/mda/effect/repsycho': ['Effects', 'Sampling'],
        '@libneil/mda/effect/tracker': ['Effects', 'Other'],
        '@libneil/mda/effect/beatbox': ['Effects', 'Other'],
        '@libneil/mda/effect/envelope': ['Effects', 'Other'],
        '@libneil/mda/effect/shepard': ['Effects', 'Other'],
        '@libneil/mda/effect/detune': ['Effects', 'Other'],
        '@libneil/mda/effect/vocinput': ['Effects', 'Other'],
        '@libneil/mda/effect/stereo': ['Effects', 'Other'],
        '@libneil/somono/controller/lfnoise;1': ['Control'],
        '@neil/lunar/controller/Control;1': ['Control'],
        '@trac.zeitherrschaft.org/aldrin/lunar/controller/LunarLFO;1': ['Control'],
        '@zzub.org/input': ['Utility'],
        '@zzub.org/output': ['Utility'],
        '@zzub.org/recorder/file': ['Utility'],
        '@zzub.org/recorder/wavetable': ['Utility'],
        '@libneil/gershon/gfx/Oscilloscope': ['Analyzers'],
        '@libneil/gershon/gfx/Spectrum': ['Analyzers'],
        '@libneil/gershon/gfx/Spectogram': ['Analyzers']
    }

    def create_add_machine_submenu(self, connection=False):
        def get_icon_name(pluginloader):
            # uri = pluginloader.get_uri()
            #if uri.startswith('@zzub.org/dssidapter/'):
            #    return iconpath("scalable/dssi.svg")
            #if uri.startswith('@zzub.org/ladspadapter/'):
            #    return iconpath("scalable/ladspa.svg")
            #if uri.startswith('@psycle.sourceforge.net/'):
            #    return iconpath("scalable/psycle.svg")
            filename = pluginloader.get_name()
            filename = filename.strip().lower()
            for c in '():[]/,.!"\'$%&\\=?*#~+-<>`@ ':
                filename = filename.replace(c, '_')
            while '__' in filename:
                filename = filename.replace('__', '_')
            filename = filename.strip('_')
            return "%s.svg" % iconpath("scalable/" + filename)

        def add_path(tree, path, loader):
            if len(path) == 1:
                tree[path[0]] = loader
                return tree
            elif path[0] not in tree:
                tree[path[0]] = add_path({}, path[1:], loader)
                return tree
            else:
                tree[path[0]] = add_path(tree[path[0]], path[1:], loader)
                return tree

        def populate_from_tree(menu, tree):
            for key, value in tree.items():
                if not isinstance(value, dict):
                    icon = Gtk.Image()
                    filename = get_icon_name(value)
                    if os.path.isfile(filename):
                        icon.set_from_file(get_icon_name(value))
                    item = Gtk.ImageMenuItem(prepstr(key, fix_underscore=True))
                    item.set_image(icon)
                    item.connect('activate', create_plugin, value)
                    menu.add(item)
                else:
                    item, submenu = menu.add_submenu(key)
                    populate_from_tree(submenu, value)

        def create_plugin(item, loader):
            player = com.get('neil.core.player')
            if connection:
                player.create_plugin(loader, connection=connection)
            else:
                player.plugin_origin = menu.context
                player.create_plugin(loader)

        player = com.get('neil.core.player')
        plugins = {}
        tree = {}
        item, add_machine_menu = self.add_submenu("Add machine")
        for pluginloader in player.get_pluginloader_list():
            plugins[pluginloader.get_uri()] = pluginloader
        for uri, loader in plugins.items():
            try:
                path = self.plugin_tree[uri]
                if connection and (path[0] not in ["Effects", "Analyzers"]):
                    continue
                path = path + [loader.get_name()]
                tree = add_path(tree, path, loader)
            except KeyError:
                pass
        populate_from_tree(add_machine_menu, tree)
