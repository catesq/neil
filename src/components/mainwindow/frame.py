
# Neil
# Modular Sequencer
# Copyright (C) 2006,2007,2008 The Neil Development Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Gdk, GLib, Gio

import os
import re
import ctypes
from functools import cmp_to_key

from neil.utils import (
    hicoloriconpath, CancelException, 
    settingspath, filepath, show_manual, ui
)


from neil import errordlg, common
from neil import components

import config
import zzub
from preferences import show_preferences

from .cmp import cmp_view
from .statusbar import StatusBar


class FramePanel(Gtk.Notebook):
    __neil__ = dict(
        id = 'neil.core.framepanel',
        singleton = True,
        categories = [
        ],
    )

    def __init__(self):
        Gtk.Notebook.__init__(self)
        self.set_tab_pos(Gtk.PositionType.LEFT)
        self.set_show_border(True)
        self.set_border_width(1)
        self.set_show_tabs(True)
        components.get("neil.core.icons") # make sure theme icons are loaded
        defaultpanel = None
        self.statusbar = components.get('neil.core.statusbar')
        self.pages = sorted(components.get_from_category('neil.viewpanel'), key=cmp_to_key(cmp_view))

        for index, panel in enumerate(self.pages):
            if not panel or not hasattr(panel, '__view__'):
                print(("panel", panel, "misses attribute __view__"))
                continue

            options = panel.__view__
            stockid = options['stockid']
            label = options['label']
            key = options.get('shortcut', '')

            if options.get('default'):
                defaultpanel = panel
                
            panel.show_all()

            theme_img = ui.new_theme_image(stockid, Gtk.IconSize.MENU)
            header = Gtk.VBox()
            labelwidget = Gtk.Label(label=label)
            labelwidget.set_angle(90)
            header.pack_start(labelwidget, True, True, 0)
            header.pack_start(theme_img, True, True, 0)
            header.show_all()

            if key:
                header.set_tooltip_text("%s (%s)" % (label, key))
            else:
                header.set_tooltip_text(label)

            self.append_page(panel, header)

        if defaultpanel:
            self.select_viewpanel(defaultpanel)
        else:
            self.select_viewpanel(self.pages[0])

        self.show_all()

    def select_viewpanel(self, panel):
        prev_index = self.get_current_page()

        if prev_index < 0 or prev_index > len(self.pages):
            return

        if self.pages[prev_index] == panel:
            return
        
        try:
            next_index = self.pages.index(panel)
        except ValueError:
            return
        
        self.statusbar.clear_both_sides()

        if hasattr(self.pages[prev_index], 'remove_focus'):
            self.pages[prev_index].remove_focus()

        self.set_current_page(next_index)

        if hasattr(panel, 'handle_focus'):
            panel.handle_focus()


class Accelerators(Gtk.AccelGroup):
    __neil__ = dict(
        id = 'neil.core.accelerators',
        singleton = True,
        categories = [],
    )

    def __init__(self):
        Gtk.AccelGroup.__init__(self)

    def add_accelerator(self, shortcut, widget, signal="activate"):
        key, modifier = Gtk.accelerator_parse(shortcut)
        return widget.add_accelerator(signal, self,  key,  modifier, Gtk.AccelFlags.VISIBLE)


#~class NeilToolbar(Gtk.Toolbar):
#~ __neil__ = dict(
#~        id = 'neil.core.toolbar',
#~        singleton = True,
#~        categories = [
#~                'view',
#~        ],
#~ )
#~
#~  __view__ = dict(
#~                label = "Toolbar",
#~                order = 0,
#~                toggle = True,
#~  )

# class NeilStatusbar(Gtk.Statusbar):
#   __neil__ = dict(
#           id = 'neil.core.statusbar',
#           singleton = True,
#           categories = [
#                   'view',
#           ],
#   )

#   __view__ = dict(
#                   label = "Statusbar",
#                   order = 0,
#                   toggle = True,
#   )

#   def __init__(self):
#     GObject.GObject.__init__(self)
#     self.push(0, "Ready to rok again")



class NeilFrame(Gtk.Window):
    """
    The application main window class.
    """

    __neil__ = dict(
        id = 'neil.core.window.root',
        singleton = True,
        categories = [
            'rootwindow',
        ],
    )

    OPEN_SONG_FILTER = [
        ui.file_filter("CCM Songs (*.ccm)", "*.ccm"),
        ui.file_filter("CCM Song Backups (*.ccm.???.bak)", "*.ccm.???.bak"),
    ]

    SAVE_SONG_FILTER = [
        ui.file_filter("CCM Songs (*.ccm)","*.ccm"),
    ]

    DEFAULT_EXTENSION = '.ccm'

    title = "Neil"
    filename = ""

    event_to_name = dict([(getattr(zzub,x),x) for x in dir(zzub) if \
                          x.startswith('zzub_event_type_')])
    pages = {}

    def __init__(self):
        """
        Initializer.
        """

        Gtk.Window.__init__(self, type=Gtk.WindowType.TOPLEVEL)

        # 
        components.get_player().set_host_info(1, 1, ctypes.c_void_p(hash(self)))

        
        theme = config.get_config().get_style()

        theme_file = Gio.File.new_for_path(os.path.join(settingspath(), "themes", theme, "main.css"))


        provider = Gtk.CssProvider()
        screen = Gdk.Screen.get_default()
        
        if screen is None:
            return
        
        style_context = Gtk.StyleContext()
        style_context.add_provider_for_screen(
            screen, provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )
        provider.load_from_file(theme_file)


        errordlg.install(self)

        geometry = Gdk.Geometry()
        geometry.min_height = 800
        geometry.min_width = 800
        hints = Gdk.WindowHints(3) # WindowHints.POS + WindowsHints.Min_SIZE

        self.set_geometry_hints(self, geometry, hints)
        self.set_position(Gtk.WindowPosition.CENTER)

        self.open_dlg = Gtk.FileChooserDialog(
            title="Open",
            parent=self,
            action=Gtk.FileChooserAction.OPEN
        )

        self.open_dlg.add_buttons(
            Gtk.STOCK_CANCEL,
            Gtk.ResponseType.CANCEL,
            Gtk.STOCK_OPEN,
            Gtk.ResponseType.OK
        )

        self.open_dlg.add_shortcut_folder(filepath('demosongs'))


        display = Gdk.Display.get_default()
        if not display:
            return
        
        monitor = display.get_primary_monitor()

        if not monitor:
            return
        
        geometry = monitor.get_geometry()
        common.set_screen_size(geometry.width, geometry.height)
        

        for filefilter in self.OPEN_SONG_FILTER:
            self.open_dlg.add_filter(filefilter)

        self.save_dlg = Gtk.FileChooserDialog(
            title="Save",
            parent=self,
            action=Gtk.FileChooserAction.SAVE
        )

        self.save_dlg.add_buttons(
            Gtk.STOCK_CANCEL,
            Gtk.ResponseType.CANCEL,
            Gtk.STOCK_SAVE,
            Gtk.ResponseType.OK
        )

        self.save_dlg.set_do_overwrite_confirmation(True)
        for filefilter in self.SAVE_SONG_FILTER:
            self.save_dlg.add_filter(filefilter)

        vbox = Gtk.VBox()
        self.add(vbox)

        self.accelerators = components.get('neil.core.accelerators')
        self.add_accel_group(self.accelerators)

        # build menu Bar
        menubar = self.populate_menubar( ui.EasyMenuBar())
        vbox.pack_start(menubar, False, False, 0)

        # create some panels that are always visible
        self.master = components.get('neil.core.panel.master')
        self.statusbar = components.get('neil.core.statusbar')
        self.transport = components.get('neil.core.transport')
        self.playback_info = components.get('neil.core.playback')
        self.framepanel = components.get('neil.core.framepanel')

        hbox = Gtk.HBox()
        hbox.pack_start(self.framepanel, True, True, 0)
        hbox.pack_end(self.master, False, True, 0)
        vbox.add(hbox)

        vbox.pack_end(self.statusbar, False, True, 0)
        self.statusbar.set_size_request(1, 60)

        self.update_title()
        theme = Gtk.IconTheme.get_default()
        # todo: why doesn't add_resource_path work with the app icon
        # theme.add_resource_path(hicoloriconpath(""))

        icons = []
        icon_name=("neil")
        for size in [48, 32, 24, 22, 16]:
            theme.append_search_path(hicoloriconpath(f"{size}x{size}/apps"))

        for size in [48, 32, 24, 22, 16]:
            icon = theme.load_icon(icon_name, size, 0)
            icons.append(icon)
        self.set_icon_list(icons)

        # Gtk.Window_set_default_icon_list(
        #     GdkPixbuf.Pixbuf.new_from_file(hicoloriconpath("48x48/apps/neil.png")),
        #     GdkPixbuf.Pixbuf.new_from_file(hicoloriconpath("32x32/apps/neil.png")),
        #     GdkPixbuf.Pixbuf.new_from_file(hicoloriconpath("24x24/apps/neil.png")),
        #     GdkPixbuf.Pixbuf.new_from_file(hicoloriconpath("22x22/apps/neil.png")),
        #     GdkPixbuf.Pixbuf.new_from_file(hicoloriconpath("16x16/apps/neil.png"))
        # )
        self.resize(800, 600)

        self.connect('key-press-event', self.on_key_down)
        self.connect('destroy', self.on_destroy)
        self.connect('delete-event', self.on_close)

        self.framepanel.connect('switch-page', self.page_select)

        GLib.timeout_add(500, self.update_title)
        self.activated=0

        self.show_all()
        self.master.hide()
        self.load_view()

        eventbus = components.get('neil.core.eventbus')
        eventbus.document_path_changed += self.on_document_path_changed
        eventbus.print_mapping()
        options, args = components.get('neil.core.options').get_options_args()
        if len(args) > 1:
            self.open_file(args[1])
        
        for driver in components.get_from_category('driver'):
            if driver.init_failed:
                GLib.timeout_add(50, show_preferences, self, 1)
                break

    def on_undo(self, *args):
        """
        Called when an undo item is being called.
        """
        player = components.get('neil.core.player')
        player.set_callback_state(False)
        player.undo()
        player.set_callback_state(True)
        eventbus = components.get('neil.core.eventbus')
        eventbus.document_loaded()
        #self.print_history()

    def on_redo(self, *args):
        """
        Called when an undo item is being called.
        """
        player = components.get('neil.core.player')
        player.set_callback_state(False)
        player.redo()
        player.set_callback_state(True)
        eventbus = components.get('neil.core.eventbus')
        eventbus.document_loaded()
        #self.print_history()


    def print_history(self):
        """
        Dumps the current undo history to console.
        """
        player = components.get('neil.core.player')
        pos = player.history_get_position()
        historysize = player.history_get_size()
        if not historysize:
            print("no history.")
            return
        print("----")
        for index in range(historysize):
            desc = str(player.history_get_description(index))
            s = '#%i: "%s"' % (index,desc)
            if pos == index:
                s += ' <-'
            print(s)
        print("----")


    def can_activate_undo(self, *args):
        """
        handler for can-activate-accel signal by Undo menuitem. Checks if undo can be executed.
        """
        player = components.get('neil.core.player')
        return player.can_undo()


    def can_activate_redo(self, *args):
        """
        handler for can-activate-accel signal by Redo menuitem. Checks if redo can be executed.
        """
        player = components.get('neil.core.player')
        return player.can_redo()


    def populate_menubar(self, menubar):
        menuitem, _ = menubar.add_submenu("_File", ui.EasyMenu(), self.update_filemenu)
        self.update_filemenu(menuitem)

        menuitem, _ = menubar.add_submenu("_Edit", ui.EasyMenu(), self.update_editmenu)
        self.update_editmenu(menuitem)

        menubar.add_submenu("_View", components.get('neil.core.viewmenu'))

        _, toolsmenu = menubar.add_submenu("_Tools", Gtk.Menu())
        components.get_from_category('menuitem.tool', toolsmenu)
                
        menubar.add_submenu("_Help", self.populate_help_menu())
        return menubar
    

    def populate_help_menu(self):
        menu = ui.EasyMenu()

        menu.add_stock_image_item(Gtk.STOCK_HELP, self.on_help_contents)
        menu.add_item('Ask on IRC', self.on_irc)
        menu.add_separator()
        menu.add_item('Report a Bug', self.on_bug_report)
        menu.add_separator()
        menu.add_item('Donate', self.on_donate)
        menu.add_separator()
        menu.add_stock_image_item(Gtk.STOCK_ABOUT, self.on_about)

        return menu



    def update_editmenu(self, menuitem, *args):
        """
        Updates the edit menu, including the undo menu.
        """
        editmenu = ui.easy_menu_wrapper(menuitem.get_submenu())

        for item in editmenu:
            item.destroy()
            
        player = components.get('neil.core.player')

        pos = player.history_get_position()
        self.print_history()

        accel = components.get('neil.core.accelerators')
        item = editmenu.add_item("Undo", "", self.on_undo)
        accel.add_accelerator("<Control>Z", item)
        if player.can_undo():
            item.get_children()[0].set_label('Undo "%s"' % player.history_get_description(pos-1))
        else:
            item.set_sensitive(False)
        item.connect('can-activate-accel', self.can_activate_undo)

        item = editmenu.add_item("Redo", "", self.on_redo)
        accel.add_accelerator("<Control>Y", item)
        if player.can_redo():
            item.get_children()[0].set_label('Redo "%s"' % player.history_get_description(pos))
        else:
            item.set_sensitive(False)
        item.connect('can-activate-accel', self.can_activate_redo)

        editmenu.add_separator()
        editmenu.add_stock_image_item(Gtk.STOCK_CUT, self.on_cut)
        editmenu.add_stock_image_item(Gtk.STOCK_COPY, self.on_copy)
        editmenu.add_stock_image_item(Gtk.STOCK_PASTE, self.on_paste)
        editmenu.add_separator()
        editmenu.add_stock_image_item(Gtk.STOCK_PREFERENCES, self.on_preferences)

        editmenu.show_all()


    def page_select(self, notebook, page, page_num, *args):
        new_page = notebook.get_nth_page(page_num)
        if hasattr(new_page, 'page_selected'):
            new_page.page_selected()


    def update_filemenu(self, menuitem, *args):
        """
        Updates the most recent files in the file menu.

        @param widget: the Menu item.
        @type widget: Gtk.MenuItem
        """
        filemenu = ui.easy_menu_wrapper(menuitem.get_submenu())

        for item in filemenu:
            item.destroy()

        filemenu.add_stock_image_item(Gtk.STOCK_NEW, self.on_new_file, frame=self, shortcut="<Control>N")
        filemenu.add_stock_image_item(Gtk.STOCK_OPEN, self.on_open, frame=self, shortcut="<Control>O")
        filemenu.add_stock_image_item(Gtk.STOCK_SAVE, self.on_save, frame=self, shortcut="<Control>S")
        filemenu.add_stock_image_item(Gtk.STOCK_SAVE_AS, self.on_save_as)

        recent_files = config.get_config().get_recent_files_config()
        if recent_files:
            filemenu.add_separator()
            for i,filename in enumerate(recent_files):
                filetitle=os.path.basename(filename).replace("_","__")
                filemenu.add_item("_%i %s" % (i+1,filetitle), self.open_recent_file, filename)

        filemenu.add_separator()
        filemenu.add_stock_image_item(Gtk.STOCK_QUIT, self.on_exit, frame=self, shortcut="<Control>Q")
        filemenu.show_all()

    def get_active_view(self):
        """
        Returns the active panel view.
        """
        for pindex,(ctrlid,(panel,menuitem)) in self.pages.items():
            if panel.is_visible() and hasattr(panel,'view'):
                return panel.view

    def on_copy(self, event):
        """
        Sent when the copy function is selected from the menu.

        @param event: Menu event.
        @type event: MenuEvent
        """
        view = self.get_active_view()
        if view and hasattr(view, 'on_copy'):
            view.on_copy(event)

    def on_cut(self, event):
        """
        Sent when the cut function is selected from the menu.

        @param event: Menu event.
        @type event: MenuEvent
        """
        view = self.get_active_view()
        if view and hasattr(view, 'on_cut'):
            view.on_cut(event)

    def on_paste(self, event):
        """
        Sent when the paste function is selected from the menu.

        @param event: Menu event.
        @type event: MenuEvent
        """
        view = self.get_active_view()
        if view and hasattr(view, 'on_paste'):
            view.on_paste(event)

    def load_view(self):
        """
        Called to load view settings from config
        """
        cfg = config.get_config()
        cfg.load_window_pos("MainFrameWindow", self)
        #~cfg.load_window_pos("Toolbar", self.neilframe_toolbar)
        #cfg.load_window_pos("MasterToolbar", self.mastertoolbar)
        # cfg.load_window_pos("Transport", self.transport)
        cfg.load_window_pos("Playback", self.playback_info)
        #cfg.load_window_pos("StatusBar", self.neilframe_statusbar)

    def save_view(self):
        """
        Called to store view settings to config
        """
        cfg = config.get_config()
        cfg.save_window_pos("MainFrameWindow", self)
        #~cfg.save_window_pos("Toolbar", self.neilframe_toolbar)
        #cfg.save_window_pos("MasterToolbar", self.mastertoolbar)
        # cfg.save_window_pos("Transport", self.transport)
        cfg.save_window_pos("Playback", self.playback_info)
        #cfg.save_window_pos("StatusBar", self.neilframe_statusbar)

    def on_help_contents(self, *args):
        """
        Event handler triggered by the help menu option.

        @param event: menu event.
        @type event: MenuEvent
        """
        show_manual()

    def on_irc(self, *args):
        import webbrowser
        webbrowser.open("http://webchat.freenode.net/?channels=neil-tracker&uio=d4")

    def on_bug_report(self, *args):
        import webbrowser
        webbrowser.open("https://bitbucket.org/bucket_brigade/neil/issues/new")

    def on_donate(self, *args):
        import webbrowser
        webbrowser.open("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=TCNUSE2A74ZSG&lc=LT&item_name=Neil%20Modular%20Tracker&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donate_SM%2egif%3aNonHosted")

    def on_about(self, *args):
        """
        Event handler triggered by the "About" menu option.

        @param event: menu event.
        @type event: MenuEvent
        """
        components.get('neil.core.dialog.about', self).show()

    def on_preferences(self, *args):
        """
        Event handler triggered by the "Preferences" menu option.

        @param event: menu event.
        @type event: MenuEvent
        """
        show_preferences(self)

    def on_key_down(self, widget, event):
        """
        Event handler for key events.
        """
        k = Gdk.keyval_name(event.keyval)
        player = components.get('neil.core.player')
        if k == 'F6':
            self.play_from_cursor(event)
        elif k == 'F5':
            player.play()
        elif k == 'F8':
            player.stop()
        else:
            return False
        return True

    # TODO probabley replace by framepanel
    # def on_activate_page(self, widget, unused, page_num):
        # self.select_page(page_num)

    def open_recent_file(self, widget, filename):
        """
        Event handler triggered by recent file menu options.

        @param event: menu event.
        @type event: MenuEvent
        """
        try:
            self.save_changes()
            self.open_file(filename)
        except CancelException:
            pass

    def update_title(self):
        """
        Updates the title to display the filename of the currently
        loaded document.
        """
        player = components.get('neil.core.player')
        filename = os.path.basename(player.document_path)
        if not filename:
            filename = 'Unsaved'
        if player.document_changed():
            filename = '*'+filename
        if filename:
            title = filename + ' - ' + self.title
        else:
            title = self.title
        self.set_title(title)
        return True


    def open_file(self, filename):
        """
        Loads a song from disk. The old document will be wiped, and
        the song will be added to the recent file list.

        @param filename: Path to song.
        @type filename: str
        """
        if not os.path.isfile(filename):
            return
        self.clear()
        player = components.get('neil.core.player')
        base, ext = os.path.splitext(filename)
        if re.search(r"""\.ccm$|\.ccm.\d\d\d\.bak$""", filename):
            dlg = Gtk.Dialog('Neil', parent=self, flags=Gtk.DialogFlags.MODAL)
            progBar = Gtk.ProgressBar()
            progBar.set_text('Loading CCM Song...')
            progBar.set_size_request(300, 40)
            progBar.set_fraction(0)
            progBar.show()
            dlg.get_content_area().pack_start(progBar, True, True, 0)
            dlg.show()
            done = False
            def progress_callback():
                progBar.pulse()
                return not done
            progBar.pulse()
            ui.refresh_gui()
            GLib.timeout_add(50, progress_callback)
            player.load_ccm(filename)
            done = True
            # The following loads sequencer step size.
            try:
                seq = components.get('neil.core.sequencerpanel')
                index = seq.toolbar.steps.index(player.get_seqstep())
                seq.toolbar.stepselect.set_active(index)
            except ValueError:
                seq.toolbar.stepselect.set_active(5)
            ui.refresh_gui()
            dlg.destroy()
        else:
            ui.message(self, "'%s' is not a supported file format." % ext)
            return

    def on_document_path_changed(self, path):
        self.update_title()
        components.get('neil.core.config').add_recent_file_config(path)

    def save_file(self, filename):
        """
        Saves a song to disk. The document will also be added to the
        recent file list.

        @param filename: Path to song.
        @type filename: str
        """
        player = components.get('neil.core.player')
        try:
            if not os.path.splitext(filename)[1]:
                filename += self.DEFAULT_EXTENSION
            if os.path.isfile(filename):
                if config.get_config().get_incremental_saving():
                # rename incremental
                    path,basename = os.path.split(filename)
                    basename, ext = os.path.splitext(basename)
                    i = 0
                    while True:
                        newpath = os.path.join(path,"%s%s.%03i.bak" % (basename, ext, i))
                        if not os.path.isfile(newpath):
                            break
                        i += 1
                    print(('%s => %s' % (filename, newpath)))
                    os.rename(filename, newpath)
                else:
                    # store one backup copy
                    path,basename = os.path.split(filename)
                    basename,ext = os.path.splitext(basename)
                    newpath = os.path.join(path,"%s%s.bak" % (basename,ext))
                    if os.path.isfile(newpath):
                        os.remove(newpath)
                    print(('%s => %s' % (filename, newpath)))
                    os.rename(filename, newpath)
            base,ext = os.path.splitext(filename)
            result = player.save_ccm(filename)
            assert result == 0
            player.document_unchanged()
        except:
            import traceback
            text = traceback.format_exc()
            traceback.print_exc()
            ui.error(self, "<b><big>Error saving file:</big></b>\n\n%s" % text)
        #~ progress.Update(100)
        #self.update_title()
        #components.get('neil.core.config').add_recent_file_config(filename)

    def on_open(self, *args):
        """
        Event handler triggered by the "Open File" menu option.

        @param event: menu event.
        @type event: MenuEvent
        """
        try:
            self.save_changes()
            self.open()
        except CancelException:
            pass

    def open(self):
        """
        Shows the open file dialog and if successful, loads the
        selected song from disk.
        """
        response = self.open_dlg.run()
        self.open_dlg.hide()
        if response == Gtk.ResponseType.OK:
            self.open_file(self.open_dlg.get_filename())

    def on_save(self, *args):
        """
        Event handler triggered by the "Save" menu option.
        """
        try:
            self.save()
        except CancelException:
            pass

    def save(self):
        """
        Shows a save file dialog if filename is unknown and saves the file.
        """
        player = components.get('neil.core.player')
        if not player.document_path:
            self.save_as()
        else:
            self.save_file(player.document_path)

    def save_as(self):
        """
        Shows a save file dialog and saves the file.
        """
        player = components.get('neil.core.player')
        self.save_dlg.set_filename(player.document_path)
        response = self.save_dlg.run()
        self.save_dlg.hide()
        if response == Gtk.ResponseType.OK:
            filepath = self.save_dlg.get_filename()
            self.save_file(filepath)
        else:
            raise CancelException

    def on_save_as(self, *args):
        """
        Event handler triggered by the "Save As" menu option.

        @param event: menu event.
        @type event: MenuEvent
        """
        try:
            self.save_as()
        except CancelException:
            pass

    def clear(self):
        """
        Clears the current document.
        """
        common.get_plugin_infos().reset()
        player = components.get('neil.core.player')
        player.clear()
        player.set_loop_start(0)
        player.set_loop_end(components.get('neil.core.sequencerpanel').view.step)
        player.get_plugin(0).set_parameter_value(1, 0, 1, config.get_config().get_default_int('BPM', 126), 1)
        player.get_plugin(0).set_parameter_value(1, 0, 2, config.get_config().get_default_int('TPB', 4), 1)
        player.history_flush_last()

    def play(self, *args):
        """
        Event handler triggered by the "Play" toolbar button.

        @param event: menu event.
        @type event: MenuEvent
        """
        player = components.get('neil.core.player')
        player.play()

    def play_from_cursor(self, *args):
        """
        Event handler triggered by the F6 key.

        @param event: menu event.
        @type event: MenuEvent
        """
        player = components.get('neil.core.player')
        player.set_position(max(components.get('neil.core.sequencerpanel').view.row,0))
        player.play()

    def on_select_theme(self, widget, data):
        """
        Event handler for theme radio menu items.

        @param event: menu event.
        @type event: MenuEvent
        """
        cfg = config.get_config()
        if not data:
            cfg.select_theme(None)
        else:
            cfg.select_theme(data)
        player = components.get('neil.core.player')
        player.document_changed()

    def stop(self, *args):
        """
        Event handler triggered by the "Stop" toolbar button.

        @param event: menu event.
        @type event: MenuEvent
        """
        player = components.get('neil.core.player')
        player.stop()

    def save_changes(self):
        """
        Asks whether to save changes or not. Throws a {CancelException} if
        cancelled.
        """
        player = components.get('neil.core.player')
        if not player.document_changed():
            return
        if player.document_path:
            text = "<big><b>Save changes to <i>%s</i>?</b></big>" % os.path.basename(player.document_path)
        else:
            text = "<big><b>Save changes?</b></big>"

        response = ui.question(self, text)
        if response == int(Gtk.ResponseType.CANCEL) or response == int(Gtk.ResponseType.DELETE_EVENT):
            raise CancelException

        if response == int(Gtk.ResponseType.YES):
            self.save()

    def on_new_file(self, *args):
        """
        Event handler triggered by the "New" menu option.

        @param event: menu event.
        @type event: MenuEvent
        """
        try:
            self.save_changes()
            self.clear()
            self.update_title()
            components.get('neil.core.player').document_unchanged()
        except CancelException:
            pass

    def on_destroy(self, *args):
        """
        Event handler triggered when the window is being destroyed.
        """
        eventbus = components.get('neil.core.eventbus')
        eventbus.shutdown()

    def on_exit(self, *args):
        """
        Event handler triggered by the "Exit" menu option.

        @param event: menu event.
        @type event: MenuEvent
        """
        if not self.on_close(None, None):
            self.destroy()

    def on_close(self, *args):
        """
        Event handler triggered when the window is being closed.
        """
        self.save_view()
        try:
            self.save_changes()
            self.hide()
            return False
        except CancelException:
            return True



