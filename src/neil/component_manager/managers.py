from __future__ import annotations
from typing import Dict, List, TYPE_CHECKING, Optional

if TYPE_CHECKING:
    from components.mainwindow.statusbar import StatusBar
    from components.router import RouteView
    from components.player import NeilPlayer
    from components.config import NeilConfig
    from components.eventbus import EventBus
    from components.options import OptionParser
    from components.driver import AudioDriver, MidiDriver
    from ..utils import ui

from .package import PackageInfo
from .loader import NamedComponentLoader
from .. import errordlg

import gi
gi.require_version('Gdk', '3.0')
from gi.repository import Gtk

# The component manager is the main registry for the service locator
# it has a factoryinfo for each class id 
# usually only one class for each class id
# custom components eg in ~/.local/neil/components can override the default ones 


def get_neil_classes(neil_dict) -> List[type]:
    return neil_dict.get('classes', [])


class ComponentInfo:
    def __init__(self, pkg_info: PackageInfo):
        self.id = id
        self.singleton = False
        self.categories = []
        self.package = pkg_info
        self._builders = []

    def add_custom_builder(self, custom: 'FactoryInfo'):
        if custom.priority > self.priority:
            self.priority = custom.priority
            self._classobj = custom._classobj

        self._builders.append(custom._classobj)


class FactoryInfo:
    def __init__(self, classobj, pkg_info: PackageInfo):
        self.id = id
        self.singleton = False
        self.categories = []
        self.priority = 1
        self.__dict__.update(classobj.__neil__)
        self._classobj = classobj
        self._pkg = pkg_info
        self._builders = [self._classobj]
        

    ## get a setting from one of the __neil__ dicts from the class
    def get_config(self, key: str, default=False):
        if key and key[0] != '_':
            return self.__dict__.get(key, default)
        else:
            return default
    

    def create_instance(self, *args, **kwargs):
        return self._classobj(*args, **kwargs)
    

    def can_build(self):
        return self._classobj and callable(self._classobj)
    
    
    def add_custom_builder(self, custom: 'FactoryInfo'):
        if custom.priority > self.priority:
            self.priority = custom.priority
            self._classobj = custom._classobj

        self._builders.append(custom._classobj)




class ComponentManager():
    def __init__(self):
        self.clear()
        
        
    def clear(self):
        self.is_loaded = False
        self.singletons = {}
        self.factories = {}
        self.categories = {}
        self.packages = []
        

    def init(self):
        loader = NamedComponentLoader()
        loader.load(self)
        print("Categories:", self.categories)
        self.is_loaded = True


    #
    def register(self, info: PackageInfo, neil_dict: dict, modulefilename: str):
        self.packages.append(info)
        
        for class_ in get_neil_classes(neil_dict):
            if not hasattr(class_, '__neil__'):
                #show error message showing name of class and file
                print("%s class does not have __neil__ metadict: %s" % (class_.__name__, modulefilename))
                continue
            
            factory_info = FactoryInfo(class_, info)

            if factory_info.id in self.factories:
                self.factories[factory_info.id].add_custom_builder(factory_info)
            else:
                self.factories[factory_info.id] = factory_info
            
            for category in factory_info.categories:
                self.add_category(category, factory_info.id)


    def throw(self, id, arg):
        class_ = self.get_factory(id)
        raise class_(arg) # type: ignore


    def add_category(self, category_name, component_id):
        if not category_name in self.categories:
            self.categories[category_name] = []

        self.categories[category_name].append(component_id)

    def has_factory(self, id) -> bool:
        return id in self.factories

    def get_factory(self, id) -> FactoryInfo | None:
        # get metainfo for object
        factory_info = self.factories.get(id, None)

        if not factory_info:
            print("no factory info found for classid '%s'" % id)
            return None
        
        if not factory_info.can_build():
            print("factory config problem for classid '%s'" % id)
            return None
        
        return factory_info

    def get_factory_priority(self, id) -> int:
        factory_info = self.get_factory(id)

        return 0 if factory_info is None else factory_info.priority


    def exception(self, id):
        return self.get_factory(id)


    def singleton(self, id):
        # try to get a singleton first
        instance = self.singletons.get(id, False)
        if instance != False:
            assert instance, "got empty instance for classid '%s'. recursive loop?" % id
            return instance
        return None


    def get(self, id, *args, **kwargs):
        instance = self.singleton(id)

        if instance:
            return instance
        
        # retrieve factory
        factory = self.get_factory(id)

        if not factory:
            return None
        
        if factory.get_config('singleton'):
            self.singletons[id] = None  # fill with an empty slot so we get no recursive loop

        # create a new object
        obj = None
        try:
            obj = factory.create_instance(*args, **kwargs)
        except:
            import traceback
            traceback.print_exc()
            msg = "<b><big>Could not create component</big></b>"
            msg2 = "while trying to create '" + id + "'"
            errordlg.error(None, msg, msg2, traceback.format_exc(), offer_quit=True)
    
        if factory.get_config('singleton'):
            if not obj:
                del self.singletons[id]
            else:
                self.singletons[id] = obj  # register as singleton

        return obj


    def get_ids_from_category(self, category):
        return self.categories.get(category, [])


    def category(self, cat, *args, **kwargs):
        return self.get_from_category(cat, *args, **kwargs)


    def get_from_category(self, category, *args, **kwargs):
        return [self.get(classid, *args, **kwargs) for classid in self.categories.get(category, [])]


    def get_player(self) -> 'NeilPlayer':
        return self.get('neil.core.player')        # pyright: ignore[reportReturnType]


    def get_config(self) -> 'NeilConfig':
        return self.get('neil.core.config')        # pyright: ignore[reportReturnType]
    

    def get_eventbus(self) -> 'EventBus':
        return self.get('neil.core.eventbus')      # pyright: ignore[reportReturnType]


    def get_audio_driver(self) -> 'AudioDriver':
        return self.get('neil.core.driver.audio')  # pyright: ignore[reportReturnType]


    def get_options(self) -> 'OptionParser':
        return self.get('neil.core.options')       # pyright: ignore[reportReturnType]
    

    def get_categories(self) -> Dict[str, List[str]]:
        return self.categories


    def get_factories(self) -> Dict[str, Dict[str, str]]:
        return self.factories


    def get_factory_names(self):
        return self.factories.keys()
    

    def get_packages(self) -> List[PackageInfo]:
        return self.packages





class ViewComponentManager:
    def __init__(self, components: ComponentManager):
        self.components = components


    # parent is only necessary when the router is created in components.router.RoutePanel.__init__() 
    def get_router(self, parent: Optional[Gtk.Widget] = None) -> RouteView:
        return self.components.get('neil.core.router.view', parent) # pyright: ignore[reportReturnType]


    def get_statusbar(self) -> StatusBar:
        return self.components.get('neil.core.statusbar') # pyright: ignore[reportReturnType]


    def get_contextmenu(self, menu_name, *args, prefix='neil.core.contextmenu') -> ui.EasyMenu:
        return self.components.get(prefix + '.' + menu_name, *args) # pyright: ignore[reportReturnType]
    

    def get_view(self, view_name, prefix='neil.core', *args) -> Gtk.Widget:
        if not self.components.has_factory(view_name):
            view_name = prefix + '.' + view_name
            if not self.components.has_factory(view_name):
                error = "View '{}' not found".format(view_name, self.components.get_ids_from_category('view'))
                self.components.throw('neil.core.error', error)

        component = self.components.get(view_name, *args)

        if not component:
            error = "Unable to build view '{}'".format(view_name, self.components.get_ids_from_category('view'))
            self.components.throw('neil.core.error', error)
        
        return component # pyright: ignore[reportReturnType]
        

    def get_dialog(self, dialog_name, *args) -> Gtk.Widget:
        close_matches = []
        
        for id in self.components.get_ids_from_category('viewdialog'):
            if id == dialog_name:
                return self.components.get(id, *args) # pyright: ignore[reportReturnType]
            elif id.endswith(dialog_name):
                close_matches.append(id)

        if len(close_matches) == 1:
            return self.components.get(close_matches[0], *args) # pyright: ignore[reportReturnType]
        elif len(close_matches) > 1:
            top_id = self.get_closest_match(close_matches)
            return self.components.get(top_id, *args) # pyright: ignore[reportReturnType]
        else:
            error = "Dialog '{}' not found in {}".format(dialog_name, self.components.get_ids_from_category('viewdialog'))
            self.components.throw('neil.core.error', error)


    def get_closest_match(self, close_matches):
        close_matches.sort(key=lambda id: self.components.get_factory_priority(id))
        return close_matches[0]



__all__ = [
    'ComponentManager', 
    'ViewComponentManager'
]