
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, Gdk, GdkPixbuf, GLib, GObject

from neil import components
import weakref
import types
from .textify import prepstr
import ctypes

from . import colors 
from .colors import *

def set_clipboard_text(data):
    clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
    clipboard.set_text(data, len(data))
    clipboard.store()


def get_clipboard_text():
    clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
    return clipboard.wait_for_text()


def refresh_gui():
    main_context = GLib.MainContext.default()
    while main_context.pending():
        main_context.iteration(False)


def make_submenu_item(submenu, name, use_underline=False):
    item = Gtk.MenuItem(label=name)
    item.set_use_underline(use_underline)
    item.set_submenu(submenu)
    return item


def make_stock_menu_item(stockid, func, frame=None, shortcut=None, *args):
    item = Gtk.ImageMenuItem.new_from_stock(stockid, None)
    if frame and shortcut:
        acc = components.get('neil.core.accelerators')
        acc.add_accelerator(shortcut, item)
    if func:
        item.connect('activate', func, *args)
    return item


def make_stock_tool_item(stockid, func, *args):
    item = Gtk.ToolButton(stockid)
    if func:
        item.connect('clicked', func, *args)
    return item


def make_stock_toggle_item(stockid, func, *args):
    item = Gtk.ToggleToolButton(stockid)
    if func:
        item.connect('toggled', func, *args)
    return item


def make_stock_radio_item(stockid, func, *args):
    item = Gtk.RadioToolButton(stock_id=stockid)
    if func:
        item.connect('toggled', func, *args)
    return item


def make_menu_item(label, desc, func, underline=False, *args):
    item = Gtk.MenuItem(label=label)
    item.set_use_underline(underline)
    if desc:
        item.set_tooltip_text(desc)
    if func:
        item.connect('activate', func, *args)
    return item

def quick_menu_item(label, func, *args):
    if '_' in label:
        item = Gtk.MenuItem(label=label, use_underline=True)
    else:
        item = Gtk.MenuItem(label=label)

    if func:
        item.connect('activate', func, *args)

    return item

def make_check_item(label, desc, func, *args):
    item = Gtk.CheckMenuItem(label=label)
    if desc:
        item.set_tooltip_text(desc)
    if func:
        item.connect('toggled', func, *args)
    return item


def make_radio_item(label, desc, func, *args):
    item = Gtk.RadioMenuItem(label=label)
    if desc:
        item.set_tooltip_text(desc)
    if func:
        item.connect('toggled', func, *args)
    return item



class AcceleratorMap:
    def __init__(self):
        self.__keymap = {}

    def add_accelerator(self, shortcut, func, *args, **kargs):
        # cleanup string by converting to values and back
        km_key, km_mod = Gtk.accelerator_parse(shortcut)
        shortcut = Gtk.accelerator_name(km_key, km_mod)
        ref = None
        funcname = None
        if hasattr(func, '__self__'):
            ref = weakref.ref(func.__self__)
            funcname = func.__name__
        else:
            ref = weakref.ref(func)
        self.__keymap[shortcut] = ref,funcname,args,kargs

    def handle_key_press_event(self, widget, event):
        """
        handles a key-press-event forwarded by any widget and calls the
        function stored for that accelerator, if existing. returns True
        if successful.
        """
        # remove numlock from the key modifiers
        key_mod = Gdk.ModifierType(event.get_state() & (~Gdk.ModifierType.MOD2_MASK))
        name = Gtk.accelerator_name(event.keyval, key_mod)
        if name == None:
            return False
        for shortcut, (ref,funcname,args,kargs) in self.__keymap.items():
            if shortcut == name:
                if funcname:
                    func = getattr(ref(), funcname)
                else:
                    func = ref()
                func(*args,**kargs)
                return True
        #print "unknown shortcut:",name
        return False



class ObjectHandlerGroup:
    """
    allows to block/unblock a bank of handlers
    """

    class Unblocker:
        def __init__(self, ohg):
            self.ohg = ohg
        def __del__(self):
            self.ohg.unblock()

    def __init__(self):
        self.handlers = []

    def connect(self, widget, eventname, *args):
        handler = widget.connect(eventname, *args)
        self.handlers.append((widget,handler))

    def autoblock(self):
        """
        autoblock will return a token which calls
        unblock() when going out of scope. you must
        not store the token permanently.
        """
        self.block()
        return self.Unblocker(self)

    def block(self):
        for widget,handler in self.handlers:
            widget.handler_block(handler)

    def unblock(self):
        for widget,handler in self.handlers:
            widget.handler_unblock(handler)




class ImageToggleButton(Gtk.ToggleButton):
    """
    GTK ToggleButton with Image
    """
    def __init__(self, path, tooltip=None, width=20, height=20):
        self.image = Gtk.Image()
        self.image.set_from_pixbuf(GdkPixbuf.Pixbuf.new_from_file_at_size(path, width, height))
        GObject.GObject.__init__(self)
        if tooltip:
            self.set_tooltip_text(tooltip)
        self.set_image(self.image)



# helper functions shared by the EasyMenu and EasyMenuBar 
class MenuWrapper:
    def add_submenu(self, label, submenu = None, func = None, *args) -> tuple[Gtk.MenuItem, Gtk.Menu]:
        if not submenu:
            submenu = EasyMenu()

        item = self.add_item(label, func, args)
        item.set_submenu(submenu)

        return item, submenu


    def add_item(self, label, func = None, *args) -> Gtk.MenuItem:
        item = quick_menu_item(label, func, *args)
        self.append(item)
        return item
    

    def add_tooltip_item(self, label, desc, func, *args) -> Gtk.MenuItem:
        item = self.add_item(label, func, *args)
        item.set_tooltip_text(desc)
        return item


    def add_separator(self) -> Gtk.SeparatorMenuItem:
        self.append(Gtk.SeparatorMenuItem())

    
    def add_check_item(self, label, toggled, func, *args) -> Gtk.CheckMenuItem:
        item = Gtk.CheckMenuItem(label=label, use_underline=True)
        item.set_active(toggled)
        item.connect('toggled', func, *args)
        self.append(item)
        return item


    def add_image_item(self, label, icon_or_path, func, *args) -> Gtk.ImageMenuItem:
        # print("imagemenu.new_from_stock deprecated", label)
        item = Gtk.ImageMenuItem.new_from_stock(stock_id=label)
        if isinstance(icon_or_path, str):
            image = Gtk.Image()
            image.set_from_pixbuf(GdkPixbuf.Pixbuf.new_from_file(icon_or_path))
        elif isinstance(icon_or_path, Gtk.Image):
            image = icon_or_path
        item.set_image(image)
        item.connect('activate', func, *args)
        self.append(item)
        return item
    

    def add_stock_image_item(self, stockid, func, frame=None, shortcut=None, *args):
        item = Gtk.ImageMenuItem.new_from_stock(stockid, None)
        if frame and shortcut:
            acc = components.get('neil.core.accelerators')
            acc.add_accelerator(shortcut, item)
        if func:
            item.connect('activate', func, *args)
        self.append(item)
        return item





class EasyMenuBar(Gtk.MenuBar, MenuWrapper):
    pass



class EasyMenu(Gtk.Menu, MenuWrapper):
    def easy_popup(self, parent, event=None):
        self.show_all()
        if not self.get_attach_widget():
            self.attach_to_widget(parent and parent.get_toplevel(), None)
        if event:
            event_button = event.button
            event_time = event.time
        else:
            event_button = 0
            event_time = 0

        return super().popup(None, None, None, None, button=event_button, activate_time=event_time)



# the get_submenu() method of Gtk.MenuItem returns a Gtk.Menu
# so use this to get the EasyMenu methods 
def easy_menu_wrapper(menu: Gtk.Menu) -> EasyMenu:
    for name in dir(MenuWrapper):
        if name.startswith('add'):
            func = getattr(MenuWrapper, name)
            setattr(menu, name, types.MethodType(func, menu))
    return menu



def wave_names_generator():
    player = components.get('neil.core.player')
    for i in range(player.get_wave_count()):
        w = player.get_wave(i)
        name = "%02X. %s" % ((i + 1), prepstr(w.get_name()))
        yield name


def test_view(classname):
    obj = components.get(classname)
    if isinstance(obj, Gtk.Window):
        pass
    elif isinstance(obj, Gtk.Dialog):
        pass
    elif isinstance(obj, Gtk.Widget) and not obj.get_parent():
        dlg = components.get('neil.test.dialog', embed=obj, destroy_on_close=False)






class PropertyEventHandler(type):
    def __new__(cls, name, bases, namespace, methods={}):
        obj = super().__new__(cls, name, bases, namespace)
        for name, args in methods.items():
            obj.__generate_methods(name, args)
        return obj

    def __generate_methods(self, name, args):
        doc = args.get('doc', '')

        if args.get('list', False):
            vtype = args['vtype']
            getter = lambda self: PropertyEventHandler.listgetter(self, name,args)
            setter = lambda self,value: PropertyEventHandler.listsetter(self, name,args,value)
            default = args.get('default', [])
        else:
            if 'default' in args:
                default = args['default']
                vtype = args.get('vtype', type(default))
            else:
                vtype = args['vtype']
                default = {float: 0.0, int:0, int:0, str:'', str:'', bool:False}.get(vtype, None)
            getter = lambda self: PropertyEventHandler.getter(self, name,args)
            setter = lambda self,value: PropertyEventHandler.setter(self, name,args,value)

        setattr(self, '__' + name, default)

        getter.__name__ = 'get_' + name
        getter.__doc__ = 'Returns ' + doc
        setattr(self, 'get_' + name, getter)

        setter.__name__ = 'set_' + name
        setter.__doc__ = 'Sets ' + doc
        setattr(self, 'set_' + name, setter)

        # add a property
        prop = property(getter, setter, doc=doc)
        setattr(self, name, prop)

    def getter(self, membername, kwargs):
        return getattr(self, '__' + membername)

    def setter(self, membername, kwargs, value):
        setattr(self, '__' + membername, value)
        eventname = kwargs.get('event', membername + '_changed')
        getattr(components.get('neil.core.eventbus'), eventname)(value)

    def listgetter(self, membername, kwargs):
        return getattr(self, '__' + membername)[:]

    def listsetter(self, membername, kwargs, values):
        setattr(self, '__' + membername, values)
        eventname = kwargs.get('event', membername + '_changed')
        getattr(components.get('neil.core.eventbus'), eventname)(values[:])



def run_function_with_progress(parent, msg, allow_cancel, func, *args):
    """
    Shows a progress dialog.
    """
    buttons = []
    if allow_cancel:
        buttons = (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL)
    else:
        buttons = None
    dialog = Gtk.Dialog('',
                        parent and parent.get_toplevel(),
                        Gtk.DialogFlags.DESTROY_WITH_PARENT,
                        buttons
                    )
    label = Gtk.Label()
    label.set_markup(msg)
    label.set_alignment(0,0.5)
    progress = Gtk.ProgressBar()
    vbox = Gtk.VBox(False, 6)
    vbox.set_border_width(6)
    vbox.pack_start(label, True, True, 0)
    vbox.pack_start(progress, True, True, 0)
    dialog._label = label
    dialog._progress = progress
    dialog.markup = msg
    dialog._response = None
    dialog.fraction = 0.0
    dialog.get_content_area().add(vbox, True, True, 0)
    dialog.show_all()
    def update_progress(dlg):
        dlg._progress.set_fraction(dlg.fraction)
        dlg._label.set_markup(dlg.markup)
        time.sleep(0.01)
        return True
    def on_response(dialog, response):
        dialog._response = response
    dialog.connect('response', on_response)
    def run_function(dlg, func, args):
        if func(dlg, *args) and dlg._response == None:
            dlg.response(Gtk.ResponseType.OK)
    GObject.timeout_add(50, update_progress, dialog)
    import _thread
    _thread.start_new_thread(run_function, (dialog,func,args))
    response = dialog.run()
    dialog.destroy()
    return response


def gettext(parent, msg, text=''):
    """
    Shows a dialog to get some text.
    """
    dialog = Gtk.Dialog(
        '',
        parent and parent.get_toplevel(),
        Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT,
        (Gtk.STOCK_OK, True, Gtk.STOCK_CANCEL, False))
    label = Gtk.Label()
    label.set_markup(msg)
    label.set_alignment(0, 0.5)
    entry = Gtk.Entry()
    entry.set_text(text)
    entry.connect('activate', lambda widget: dialog.response(True))
    vbox = Gtk.VBox(False, 6)
    vbox.set_border_width(6)
    vbox.pack_start(label, True, True, 0)
    vbox.pack_end(entry, False, True, 0)
    dialog.get_content_area().add(vbox)
    dialog.show_all()
    response = dialog.run()
    text = entry.get_text()
    dialog.destroy()
    if response:
        return text


def question(parent, msg, allowcancel = True):
    """
    Shows a question dialog.
    """
    dialog = Gtk.MessageDialog(parent.get_toplevel(),
                               Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT,
                               Gtk.MessageType.QUESTION , Gtk.ButtonsType.NONE)
    dialog.set_markup(msg)
    dialog.add_buttons(
            Gtk.STOCK_YES, Gtk.ResponseType.YES,
            Gtk.STOCK_NO, Gtk.ResponseType.NO)
    if allowcancel:
        dialog.add_button(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL)
    response = dialog.run()
    dialog.destroy()
    return response


def error(parent, msg, msg2=None, details=None):
    """
    Shows an error message dialog.
    """
    dialog = Gtk.MessageDialog(parent and parent.get_toplevel(),
                               Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT,
                               Gtk.MessageType.ERROR , Gtk.ButtonsType.NONE)
    dialog.set_markup(msg)
    dialog.set_resizable(True)
    if msg2:
        dialog.format_secondary_text(msg2)
    if details:
        expander = Gtk.Expander("Details")
        dialog.get_content_area().pack_start(expander, False, False, 0)
        label = Gtk.TextView()
        label.set_editable(False)
        label.get_buffer().set_property('text', details)
        label.set_wrap_mode(Gtk.WrapMode.NONE)

        sw = Gtk.ScrolledWindow()
        sw.set_shadow_type(Gtk.ShadowType.ETCHED_IN)
        sw.set_policy(Gtk.PolicyType.AUTOMATIC, Gtk.PolicyType.AUTOMATIC)

        sw.add(label)
        expander.add(sw)
        dialog.show_all()
    dialog.add_buttons(Gtk.STOCK_OK, Gtk.ResponseType.OK)
    response = dialog.run()
    dialog.destroy()
    return response


def message(parent, msg):
    """
    Shows an info message dialog.
    """
    dialog = Gtk.MessageDialog(parent.get_toplevel(),
                               Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT,
                               Gtk.MessageType.INFO , Gtk.ButtonsType.NONE)
    dialog.set_markup(msg)
    dialog.add_buttons(Gtk.STOCK_OK, Gtk.ResponseType.OK)
    response = dialog.run()
    dialog.destroy()
    return response


def warning(parent, msg):
    """
    Shows an warning message dialog.
    """
    dialog = Gtk.MessageDialog(parent.get_toplevel(),
                               Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT,
                               Gtk.MessageType.WARNING, Gtk.ButtonsType.NONE)
    dialog.set_markup(msg)
    dialog.add_buttons(Gtk.STOCK_OK, Gtk.ResponseType.OK)
    response = dialog.run()
    dialog.destroy()
    return response


def new_listview(columns):
    """
    Creates a list store with multiple columns.
    """
    treeview = Gtk.TreeView()
    treeview.set_rules_hint(True)
    store, columncontrols = new_liststore(treeview, columns)
    return treeview,store,columncontrols


def new_combobox(columns):
    """
    Creates a combobox.
    """
    combobox = Gtk.ComboBox()
    store, columncontrols = new_liststore(combobox, columns)
    return combobox


def new_liststore(view, columns):
    """
    Creates a Gtk.TreeView for a list store with multiple columns.
    """
    class ToggledHandler:
        def fixed_toggled(self, cell, path, model):
            itr = model.get_iter((int(path),))
            checked = model.get_value(iter, self.column)
            checked = not checked
            model.set(itr, self.column, checked)

    liststore = Gtk.ListStore(*[col[1] for col in columns])
    view.set_model(liststore)
    columncontrols = []
    for i,args in enumerate(columns):
        assert len(args) >= 2
        options = {}
        if len(args) == 2:
            name,coltype = args
        else:
            name,coltype,options = args
        if name == None:
            continue
        if isinstance(view, Gtk.ComboBox):
            if i > 0:
                break
            column = view
        else:
            column = Gtk.TreeViewColumn(name)
        if coltype == str:
            if isinstance(column, Gtk.TreeViewColumn):
                column.set_resizable(True)
            if options.get('icon',False):
                cellrenderer = Gtk.CellRendererPixbuf()
                column.pack_start(cellrenderer, True)
                column.add_attribute(cellrenderer, 'icon-name', i)
            else:
                cellrenderer = Gtk.CellRendererText()
                column.pack_start(cellrenderer, True)
                if options.get('markup',False):
                    column.add_attribute(cellrenderer, 'markup', i)
                else:
                    column.add_attribute(cellrenderer, 'text', i)
                if options.get('wrap',False):
                    cellrenderer.set_property('wrap-width', options.get('width', 250))
        elif coltype == bool:
            th = ToggledHandler()
            th.column = i
            cellrenderer = Gtk.CellRendererToggle()
            cellrenderer.connect('toggled', th.fixed_toggled, liststore)
            column.pack_start(cellrenderer, True)
            column.add_attribute(cellrenderer, 'active', i)
        elif coltype == GdkPixbuf.Pixbuf:
            cellrenderer = Gtk.CellRendererPixbuf()
            column.pack_start(cellrenderer, True)
            column.add_attribute(cellrenderer, 'pixbuf', i)
        if isinstance(view, Gtk.TreeView):
            view.append_column(column)
            column.set_sort_column_id(i)
        columncontrols.append(column)
    if isinstance(view, Gtk.TreeView):
        view.set_search_column(0)
    return liststore, columncontrols


def new_image_button(path, tooltip, width=20, height=20):
    """
    Creates a button with a single image.
    """
    image = Gtk.Image()
    image.set_from_pixbuf(GdkPixbuf.Pixbuf.new_from_file_at_size(path, width, height))
    button = Gtk.Button()
    button.set_tooltip_text(tooltip)
    button.set_image(image)
    return button


def new_stock_image_button(stockid, tooltip=None):
    """
    Creates a button with a stock image.
    """
    image = Gtk.Image()
    image.set_from_stock(stockid, Gtk.IconSize.BUTTON)
    button = Gtk.Button()
    button.set_image(image)
    button.set_tooltip_text(tooltip)
    return button


def new_stock_image_toggle_button(stockid, tooltip=None, tooltips_object=None):
    """
    Creates a toggle button with a stock image.
    """
    image = Gtk.Image()
    image.set_from_stock(stockid, Gtk.IconSize.BUTTON)
    button = Gtk.ToggleButton()
    button.set_image(image)
    if tooltips_object:
        tooltips_object.set_tip(button, tooltip)
    elif tooltip:
        button.set_tooltip_text(tooltip)
    return button


def new_image_toggle_button(path, tooltip=None, width=24, height=24):
    """
    Creates a toggle button with a single image.
    """
    image = Gtk.Image()
    image.set_from_pixbuf(GdkPixbuf.Pixbuf.new_from_file_at_size(path, width, height))
    button = Gtk.ToggleButton()
    if tooltip:
        button.set_tooltip_text(tooltip)
    button.set_image(image)
    return button


def new_theme_image(name,size):
    theme = Gtk.IconTheme.get_default()
    image = Gtk.Image()
    if theme.has_icon(name):
        pixbuf = theme.load_icon(name, size, 0)
        image.set_from_pixbuf(pixbuf)
    return image


def new_theme_image_toggle_button(name, tooltip=None, tooltips_object=None):
    """
    Creates a toggle button with a default icon theme image.
    """
    image = new_theme_image(name,Gtk.IconSize.BUTTON)
    button = Gtk.ToggleButton()
    if tooltips_object:
        tooltips_object.set_tip(button, tooltip)
    elif tooltip:
        button.set_tooltip_text(tooltip)
    button.set_image(image)
    return button


def get_item_count(model):
    """
    Returns the number of items contained in a tree model.
    """
    class Count:
        value = 0
    def inc_count(model, path, iter, data):
        data.value += 1
    count = Count()
    model.foreach(inc_count,count)
    return count.value


def add_scrollbars(view):
    """
    adds scrollbars around a view
    """
    scrollwin = Gtk.ScrolledWindow()
    scrollwin.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
    if isinstance(view, Gtk.TreeView):
        scrollwin.set_shadow_type(Gtk.ShadowType.IN)
        scrollwin.add(view)
    else:
        scrollwin.add_with_viewport(view)
    return scrollwin


def add_vscrollbar(view):
    """
    adds a vertical scrollbar to a view
    """
    scrollwin = Gtk.ScrolledWindow()
    scrollwin.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
    if isinstance(view, Gtk.TreeView):
        scrollwin.set_shadow_type(Gtk.ShadowType.IN)
        scrollwin.add(view)
    else:
        scrollwin.add_with_viewport(view)
    return scrollwin


def add_hscrollbar(view):
    """
    adds a vertical scrollbar to a view
    """
    scrollwin = Gtk.ScrolledWindow()
    scrollwin.set_policy(Gtk.PolicyType.ALWAYS, Gtk.PolicyType.NEVER)
    if isinstance(view, Gtk.TreeView):
        scrollwin.set_shadow_type(Gtk.ShadowType.IN)
        scrollwin.add(view)
    else:
        scrollwin.add_with_viewport(view)
    return scrollwin


def file_filter(name,*patterns):
    ff = Gtk.FileFilter()
    ff.set_name(name)
    for pattern in patterns:
        ff.add_pattern(pattern.upper())
        ff.add_pattern(pattern.lower())
    return ff




# the X window pointer is needed by the lv2 and vst adapters to use the XEmbed protocol so 
# the plugin ui is shown in the gtk host window that the lv2/vst adapters will open

# https://stackoverflow.com/questions/23021327/how-i-can-get-drawingarea-window-handle-in-gtk3/27236258#27236258
# http://git.videolan.org/?p=vlc/bindings/python.git;a=blob_plain;f=examples/gtkvlc.py;hb=HEAD
def get_window_pointer(gtk_window):
    """ Use the window.__gpointer__ PyCapsule to get the C void* pointer to the window
    """
    # get the c gpointer of the gdk window
    ctypes.pythonapi.PyCapsule_GetPointer.restype = ctypes.c_void_p
    ctypes.pythonapi.PyCapsule_GetPointer.argtypes = [ctypes.py_object]
    return ctypes.pythonapi.PyCapsule_GetPointer(gtk_window.__gpointer__, None)


__all = [
    'set_clipboard_text',
    'get_clipboard_text',
    'refresh_gui',
    'make_submenu_item',
    'make_stock_menu_item',
    'make_stock_tool_item',
    'make_stock_toggle_item',
    'make_stock_radio_item',
    'make_menu_item',
    'quick_menu_item',
    'make_check_item',
    'make_radio_item',
    'new_listview',
    'new_combobox',
    'new_liststore',
    'new_image_button',
    'new_stock_image_button',
    'new_stock_image_toggle_button',
    'new_image_toggle_button',
    'new_theme_image_toggle_button',
    'new_theme_image',
    'get_item_count',
    'add_scrollbars',
    'add_vscrollbar',
    'add_hscrollbar',
    'file_filter',
    'run_function_with_progress',
    'gettext',
    'question',
    'error',
    'message',
    'warning',
    'ImageToggleButton',
    'ObjectHandlerGroup',
    'PropertyEventHandler',
    'AcceleratorMap',
    'EasyMenuBar',
    'EasyMenu',
    'easy_menu_wrapper',
    'wave_names_generator',
    'test_view',
    'get_window_pointer',
    'colors',
    'Colors',
    'PatternColors',
    'SequencerColors',
    'RouterColors',
    'get_plugin_color_key',
    'get_plugin_color_group',
]