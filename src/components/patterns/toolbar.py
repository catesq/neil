import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GObject

from typing import cast
from neil import components
from neil.utils import ui, show_machine_manual, filenameify, sizes

from .views import PatternView


class PatternToolBar(Gtk.HBox):
    """
    Pattern Toolbar

    Contains lists of the plugins, patterns, waves and octaves available.
    """
    def __init__(self, pattern_view: PatternView):
        """
        Initialization.
        """
        player = components.get_player()
        Gtk.HBox.__init__(self, expand=False, margin=sizes.get('margin'))
        
        self.pattern_view = pattern_view
        self.set_border_width(sizes.get('margin'))
        eventbus = components.get_eventbus()

        self.pluginselect = Gtk.ComboBoxText()
        self.pluginselect.set_size_request(100, 0)
        self.pluginselect.set_tooltip_text("Machine to edit a pattern for")
        
        self.patternlabel = Gtk.Label()
        self.patternlabel.set_text_with_mnemonic("_Patt")

        self.patternselect = Gtk.ComboBoxText()
        self.patternselect.set_tooltip_text("The pattern to edit")
        self.patternselect.set_size_request(100, 0)
        self.patternlabel.set_mnemonic_widget(self.patternselect)

        # Wave selector combo box.
        self.wavelabel = Gtk.Label()
        self.wavelabel.set_text_with_mnemonic("_Wave")
        self.waveselect = Gtk.ComboBoxText()
        self.waveselect.set_tooltip_text("Which wave to use")
        self.waveselect.set_size_request(100, 0)
        self.wavelabel.set_mnemonic_widget(self.waveselect)

        # An octave selector combo box.
        self.octavelabel = Gtk.Label()
        self.octavelabel.set_text_with_mnemonic("_Oct")
        self.octaveselect = Gtk.ComboBoxText()
        self.octaveselect.set_tooltip_text("Choose which octave you can enter notes from")
        self.octavelabel.set_mnemonic_widget(self.octaveselect)
        for octave in range(1, 9):
            self.octaveselect.append_text(str(octave))
        self.octaveselect.set_active(player.octave - 1)

        # An edit step selector combo box.
        self.edit_step_label = Gtk.Label()
        self.edit_step_label.set_text_with_mnemonic("_Step")
        self.edit_step_box = Gtk.ComboBoxText()
        self.edit_step_box.set_tooltip_text("Set how many rows the cursor will jump when editting")
        for step in range(12):
            self.edit_step_box.append_text(str(step + 1))
        self.edit_step_box.set_active(0)
        
        self.playnotes = Gtk.CheckButton(label="_Play")
        self.playnotes.set_active(True)
        self.playnotes.set_tooltip_text("If checked, the notes will be played as you enter them in the editor")

        self.btnhelp = ui.new_stock_image_button(Gtk.STOCK_HELP)
        self.btnhelp.set_tooltip_text("Machine help page")

        vsep_a = Gtk.VSeparator()
        vsep_b = Gtk.VSeparator()
        self.pack_start(self.pluginselect, False, True, 0)
        self.pack_start(self.patternselect, False, True, 0)
        self.pack_start(self.waveselect, False, True, 0)

        self.pack_start(vsep_a, False, True, 0)
        self.pack_start(self.octavelabel, False, True, 0)
        self.pack_start(self.octaveselect, False, True, 0)
        self.pack_start(self.edit_step_label, False, True, 0)
        self.pack_start(self.edit_step_box, False, True, 0)
        self.pack_start(self.playnotes, False, True, 0)
        self.pack_start(vsep_b, False, True, 0)
        self.pack_start(self.btnhelp, False, True, 0)

        self.gtk_handlers = {}
        self.gtk_handlers['pluginselect']  = self.pluginselect.connect('changed', self.set_plugin_sel)
        self.gtk_handlers['patternselect'] = self.patternselect.connect('changed', self.set_pattern_sel)
        self.gtk_handlers['waveselect']    = self.waveselect.connect('changed', self.set_wave_sel)
        self.gtk_handlers['octaveselect']  = self.octaveselect.connect('changed', self.octave_set)
        self.gtk_handlers['edit_step_box'] = self.edit_step_box.connect('changed', self.edit_step_changed)
        self.gtk_handlers['playnotes']     = self.playnotes.connect('clicked', self.on_playnotes_click)
        self.gtk_handlers['btnhelp']       = self.btnhelp.connect('clicked', self.on_button_help)


        
    def block_gtk_handler(self, widget_name):
        if widget_name in self.gtk_handlers:
            GObject.signal_handler_block(getattr(self, widget_name), self.gtk_handlers[widget_name])
        else:
            print("no handler for {}", widget_name)

        return widget_name in self.gtk_handlers


    def unblock_gtk_handler(self, widget_name):
        if widget_name in self.gtk_handlers:
            GObject.signal_handler_unblock(getattr(self, widget_name), self.gtk_handlers[widget_name])
        else:
            print("no handler for {}", widget_name)
        


    def handle_focus(self):
        self.register_events()

    def register_events(self):
        eventbus = components.get_eventbus()
        eventbus.attach(['new_plugin', 'delete_plugin', 'document_loaded', 'active_plugins_changed'], self.pluginselect_update)
        eventbus.attach(['active_plugins_changed', 'active_patterns_changed', 'delete_pattern', 'new_pattern', 'pattern_changed'], self.get_pattern_source)
        eventbus.attach(['active_waves_changed', 'delete_plugin', 'document_loaded', 'active_plugins_changed'], self.pluginselect_update)
        eventbus.attach(['new_plugin', 'wave_allocated', 'wave_changed', 'delete_wave', 'document_loaded'], self.pluginselect_update)
        eventbus.attach('octave_changed', self.octave_update)


    def remove_events(self):
        eventbus = components.get_eventbus()
        eventbus.detach(self.pluginselect_update)
        eventbus.detach(self.get_pattern_source)
        eventbus.detach(self.waveselect_update)
        eventbus.detach(self.octave_update)



    def remove_focus(self):
        self.remove_events()


    def octave_set(self, event):
        player = components.get_player()
        player.octave = int(self.octaveselect.get_active_text() or 4)
        self.pattern_view.grab_focus()
            

    def on_button_help(self, *args):
        player = components.get_player()

        if len(player.active_plugins) < 1:
            return
        
        name = filenameify(player.active_plugins[0].get_pluginloader().get_name())

        if not show_machine_manual(name):
            info = Gtk.MessageDialog(
                transient_for=cast(Gtk.Window, self.get_toplevel()), 
                message_type=Gtk.MessageType.INFO, 
                text="Sorry, there's no help for this plugin yet",
                buttons=Gtk.ButtonsType.OK
            )

            info.run()
            info.destroy()


    def pluginselect_update(self, *args):
        player = components.get_player()

        if not self.block_gtk_handler('pluginselect'):
            return

        plugins = self.get_plugin_source()
        active = -1
        
        if player.active_plugins != []:
            for plugin, i in zip(plugins, range(len(plugins))):
                if plugin[1] == player.active_plugins[0]:
                    active = i

        model = self.pluginselect.get_model()
        model.clear()

        for plugin in plugins:
            self.pluginselect.append_text(plugin[0])

        if active != -1:
            self.pluginselect.set_active(active)

        self.unblock_gtk_handler('pluginselect')


    def octave_update(self, *args):
        """
        This function is called when the current octave for entering
        notes is changed somewhere.
        """
        player = components.get_player()
        self.octaveselect.set_active(player.octave - 1)


    def waveselect_update(self, *args):
        """
        This function is called whenever it is decided that
        the wave list in the combox box is outdated.
        """
        if not self.block_gtk_handler('waveselect'):
            return
        
        player = components.get_player()
        sel = player.active_waves
        # active = self.waveselect.get_active()
        model = self.waveselect.get_model()
        model.clear()

        for i in range(player.get_wave_count()):
            w = player.get_wave(i)
            if w.get_level_count() >= 1:
                self.waveselect.append_text("%02X. %s" % (i + 1, w.get_name()))
        if sel != []:
            index = sel[0].get_index()
            self.waveselect.set_active(index)
        else:
            self.waveselect.set_active(0)
        self.unblock_gtk_handler('waveselect')


    def edit_step_changed(self, event):
        step = int(self.edit_step_box.get_active_text() or 4)
        self.pattern_view.edit_step = step
        self.pattern_view.grab_focus()

    def on_playnotes_click(self, event):
        self.pattern_view.play_notes = self.playnotes.get_active()
        self.pattern_view.grab_focus()

    def get_plugin_source(self):
        player = components.get_player()

        plugins = sorted(list(player.get_plugin_list()), key=lambda plugin: str.lower(plugin.get_name()))
        return [(plugin.get_name(), plugin) for plugin in plugins]

    def get_plugin_sel(self):
        player = components.get_player()
        sel = player.active_plugins
        sel = (sel[0] if sel else None)
        return sel

    def set_plugin_sel(self, *args):
        sel_index = self.pluginselect.get_active()
        plugins = self.get_plugin_source()
        sel = plugins[sel_index][1]
        if sel:
            player = components.get_player()
            player.active_plugins = [sel]
            if sel.get_pattern_count() > 0:
                player.active_patterns = [(sel, 0)]
            else:
                player.active_patterns = []
        self.pattern_view.grab_focus()

    def get_pattern_source(self, *args):
        player = components.get_player()
        plugin = self.get_plugin_sel()
        if not plugin:
            self.patternselect.get_model().clear()
            return
        
        #def cmp_func(a,b):
        #    aname = a[0].get_pattern_name(a[1])
        #    bname = b[0].get_pattern_name(b[1])
        #    return cmp(aname.lower(), bname.lower())
        #patterns = sorted([(plugin, i) for i in
        #                   xrange(plugin.get_pattern_count())], cmp_func)
        
        self.patternselect.get_model().clear()
        names = [(i, plugin.get_pattern_name(i)) for i in range(plugin.get_pattern_count())]
        for i, name in names:
            self.patternselect.append_text("%d %s" % (i, name))
            
        # Block signal handler to avoid infinite recursion.
        if not self.block_gtk_handler('patternselect'):
            return

        if len(player.active_patterns) > 0:
            self.patternselect.set_active(player.active_patterns[0][1])
        self.unblock_gtk_handler('patternselect')


    def get_pattern_sel(self):
        player = components.get_player()
        sel = player.active_patterns
        return sel[0] if sel else None

    def set_pattern_sel(self, sel):
        player = components.get_player()
        try:
            sel = (player.active_plugins[0], self.patternselect.get_active())
        except IndexError:
            return
        if sel[1] >= 0:
            player.active_patterns = [sel]
        self.pattern_view.grab_focus()

    def set_wave_sel(self, *args):
        player = components.get_player()
        sel = player.get_wave(self.waveselect.get_active())
        if sel:
            player.active_waves = [sel]
        self.pattern_view.grab_focus()

    def activate_wave(self, w):
        player = components.get_player()
        if w and w.get_level_count() >= 1:
            player.preview_wave(w)
        else:
            player.stop_preview()
