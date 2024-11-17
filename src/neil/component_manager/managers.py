from typing import Dict, List, TYPE_CHECKING

if TYPE_CHECKING:
    import player, router, utils.ui, eventbus

from .package import PackageInfo
from .loader import NamedComponentLoader
from .. import errordlg

# the component manager is the main registry for the service locator
# it has a factoryinfo for each class id 
# there is almost always only one class for each class id
# only more than one when custom components in ~/.local/neil/components made to override the default ones 


def get_neil_classes(neil_dict) -> List[type]:
    return neil_dict.get('classes', [])



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
                catlist = self.categories.get(category, [])
                catlist.append(factory_info.id)
                self.categories[category] = catlist


    def throw(self, id, arg):
        class_ = self.factory(id)
        raise class_(arg)


    def factory(self, id) -> FactoryInfo:
        # get metainfo for object
        factory_info = self.factories.get(id, None)
        if not factory_info:
            print("no factory info found for classid '%s'" % id)
            return None
        
        if not factory_info.can_build():
            print("factory config problem for classid '%s'" % id)
            return None
        
        return factory_info


    def exception(self, id):
        return self.factory(id)


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
        factory = self.factory(id)

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


    def get_player(self) -> 'player.NeilPlayer':
        return self.get('neil.core.player')

    def get_config(self) -> 'config.NeilConfig':
        return self.get('neil.core.config')
    
    def get_eventbus(self) -> 'eventbus.EventBus':
        return self.get('neil.core.eventbus')

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


    def get_contextmenu(self, menu_name, *args, prefix='neil.core.contextmenu') -> 'utils.ui.EasyMenu':
        return self.get_menu(menu_name, *args, prefix=prefix)
    

    def get_menu(self, menu_name, *args, prefix) -> 'utils.ui.EasyMenu':
        return self.components.get(prefix + '.' + menu_name, *args)


    def get_router(self) -> 'router.RouteView':
        return self.components.get('neil.core.router.view')



__all__ = [
    'ComponentManager', 
    'ViewComponentManager'
]