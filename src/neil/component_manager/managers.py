from __future__ import annotations
from typing import Dict, List, TYPE_CHECKING, Optional

if TYPE_CHECKING:
    from components.mainwindow.statusbar import StatusBar
    from components.mainwindow.framepanel import FramePanel
    from components.mainwindow.helpers import Accelerators
    from components.router import RouteView
    from components.player import NeilPlayer
    from components.config import NeilConfig
    from components.eventbus import EventBus
    from components.options import OptionParser
    from components.driver import AudioDriver, MidiDriver
    from components.sequencer import SequencerPanel
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
# custom components eg in ~/.local/neil/components and can override the default ones 


def get_neil_classes(neil_dict) -> List[type]:
    return neil_dict.get('classes', [])


# class ComponentInfo:
#     def __init__(self, pkg_info: PackageInfo):
#         self.id = id
#         self.singleton = False
#         self.categories = []
#         self.package = pkg_info
#         self._builders = []

#     def add_custom_builder(self, custom: 'FactoryInfo'):
#         if custom.priority > self.priority:
#             self.priority = custom.priority
#             self.classobj = custom.classobj

#         self._builders.append(custom.classobj)


class FactoryInfo:
    def __init__(self, classobj, pkg_info: PackageInfo):
        self.id = id
        self.singleton = False
        self.exception = False
        self.categories = []
        self.priority = 1
        self.__dict__.update(classobj.__neil__)

        self.classobj = classobj
        self.pkg = pkg_info
        self.builders = [self.classobj]

        if self.exception and 'exception' not in self.categories:
            self.categories.append('exception')
    

    ## get a setting from the __neil__ dict or FactoryInfo defaults
    def get_config(self, key: str, default=False):
        if key and key[0] != '_':
            return self.__dict__.get(key, default)
        else:
            return default
    

    def create_instance(self, *args, **kwargs):
        return self.classobj(*args, **kwargs)


    def get_build_class(self):
        return self.classobj


    def can_build(self):
        return self.classobj and callable(self.classobj)
    
    
    def add_custom_builder(self, custom: 'FactoryInfo'):
        if custom.priority > self.priority:
            self.priority = custom.priority
            self.classobj = custom.classobj

        self.builders.append(custom.classobj)




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


    def throw(self, id, arg: str):
        # class_ = self.get_factory(id)
        factory_id = self.get_closest_id_in_category(id, 'exception')

        if factory_id is None:
            raise Exception("No neil exception class found for id '%s':\n".format(id, arg)) 

        factory = self.get_factory(factory_id) 

        raise factory.create_instance(arg)  # pyright: ignore[reportOptionalMemberAccess]


    def exception(self, id):
        # factory = self.get_factory(id)
        factory_id = self.get_closest_id_in_category(id, 'exception')
    
        if factory_id is None:
            raise Exception("No neil exception class found for id '%s':\n".format(id, arg)) 

        factory = self.get_factory(factory_id) 

        return factory.get_build_class() # pyright: ignore[reportOptionalMemberAccess]
        
        
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


    def get_ids_from_category(self, category) -> list[str]:
        """
        """
        return self.categories.get(category, [])


    # def category(self, cat, *args, **kwargs):
    #     """
    #     shortcut for get_from_category
    #     """
    #     return self.get_from_category(cat, *args, **kwargs)



    def get_from_category(self, category, *args, **kwargs):
        """
        Build and return all components in a category

        @param category: The category to build components of
        @param args: Arguments passed to each component's constructor
        @param kwargs: Keyword arguments passed to each component's constructor

        @return: Array with all built components in the category
        """
        return [self.get(classid, *args, **kwargs) for classid in self.categories.get(category, [])]
    


    def get_closest_id_in_category(self, find_id: str, category: str, selector = None):
        """
        Finds the closest matching component id in a category. 
        
        Returns the first id that matches exactly.
        
        Otherwise it finds all components whose id contains the substring find_id 
        then uses the priority level of the partial matching components to select the one with the highest priority

        @param find_id: The id to find.
        @param category: The category to search.
        @param selector: Callable used to choose one of multiple partial matches. gets an array with the id's of the partial matches and returns a single id. 
                         Defaults to self.select_closest_match(self, partial_matches)

        @return: The closest matching id or False if no match was found.
        """
        
        if selector is None:
            selector = self.select_closest_match

        partial_matches = []

        for cat_id in self.get_ids_from_category(category):
            if find_id == cat_id:
                return find_id
            elif find_id in cat_id:
                partial_matches.append(cat_id)
        
        if len(partial_matches) == 0:
            return None
        elif len(partial_matches) == 1:
            return partial_matches[0]
        else:
            return selector(partial_matches)

    
    def select_closest_match(self, partial_matches):
        """
        Default select function for get_closest_match_in_category
        Returns the component with highest priority

        @param close_matches: Array with component id's to choose from

        @return: The id of the component with highest priority
        """
        partial_matches.sort(key=lambda id: self.get_factory_priority(id))
        return partial_matches[0]


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
    

    def get_accelerators(self) -> Accelerators:
        return self.get('neil.core.accelerators') # pyright: ignore[reportReturnType]


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


    def get_panels(self) -> FramePanel:
        return self.components.get('neil.core.framepanel')    # pyright: ignore[reportReturnType]


    # parent is only necessary when the router is created in components.router.RoutePanel.__init__() 
    def get_router(self, parent: Optional[Gtk.Widget] = None) -> RouteView:
        return self.components.get('neil.core.router.view', parent) # pyright: ignore[reportReturnType]


    def get_statusbar(self) -> StatusBar:
        return self.components.get('neil.core.statusbar') # pyright: ignore[reportReturnType]


    def get_sequencer(self) -> SequencerPanel:
        return self.components.get('neil.core.sequencerpanel')  # pyright: ignore[reportReturnType]


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


    def get_dialog(self, dialog_name, *args) -> Gtk.Window:
        component_id = self.components.get_closest_id_in_category(dialog_name, 'viewdialog')
        
        if dialog_name == 'searchplugins':
            print("views.get_dialog('searchplugins') found: ", component_id)

        if component_id:
            return self.components.get(component_id, *args)  # pyright: ignore[reportReturnType]
        else:
            all_dialogs = self.components.get_ids_from_category('viewdialog')
            error_msg = "Dialog '{}' not found in {}".format(dialog_name, all_dialogs)
            self.components.throw('neil.core.error', error_msg)



__all__ = [
    'ComponentManager', 
    'ViewComponentManager'
]