from typing import Dict, List, TYPE_CHECKING

if TYPE_CHECKING:
    import player, router, utils.ui

from .package import PackageInfo
from .loader import NamedComponentLoader



def get_neil_classes(neil_dict) -> List[type]:
    return neil_dict.get('classes', [])

class FactoryInfo:
    def __init__(self, classobj, pkg: PackageInfo):
        self.id = id
        self.singleton = False
        self.categories = []
        
        self.__dict__.update(classobj.__neil__)
        self.classobj = classobj
        self.pkg = pkg


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
    def register(self, info: PackageInfo, neil_dict: Dict, modulefilename):
        self.packages.append(info)

        for class_ in get_neil_classes(neil_dict):
            if not hasattr(class_, '__neil__'):
                #show error message showing name of class and file
                print("%s class does not have __neil__ metadict: %s" % (class_.__name__, modulefilename))
                continue
            
            factory_info = FactoryInfo(class_, info)
            self.factories[factory_info.id] = factory_info

            for category in factory_info.categories:
                catlist = self.categories.get(category, [])
                catlist.append(factory_info.id)
                self.categories[category] = catlist


    def throw(self, id, arg):
        class_ = self.factory(id)
        raise class_(arg)


    def factory(self, id):
        # get metainfo for object
        factory_info = self.factories.get(id, None)
        if not factory_info:
            print("no factory info found for classid '%s'" % id)
            return None
        
        class_ = factory_info.classobj
        if not class_:
            print("no factory found for classid '%s'" % id)
            return None
        return class_


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
        class_ = self.factory(id)

        if not class_:
            return None
        
        if class_.__neil__.get('singleton', False):
            self.singletons[id] = None  # fill with an empty slot so we get no recursive loop

        # create a new object
        obj = None
        try:
            obj = class_(*args, **kwargs)
        except:
            import traceback
            from . import errordlg
            traceback.print_exc()
            msg = "<b><big>Could not create component</big></b>"
            msg2 = "while trying to create '" + id + "'"
            errordlg.error(None, msg, msg2, traceback.format_exc(), offer_quit=True)
        if class_.__neil__.get('singleton', False):
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