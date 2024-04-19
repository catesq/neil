
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

from typing import Dict, List, TYPE_CHECKING

if TYPE_CHECKING:
    import player, router, utils.ui

from .package import PackageInfo
from .loader import NamedComponentLoader



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


    def register(self, info: PackageInfo, neil_dict: Dict, modulefilename, modulename):
        self.packages.append(info)
        #print("class", class_, "has no __neil__ metadict")
        # enumerate class factories
        for class_ in self.get_classes(neil_dict):
            if not hasattr(class_, '__neil__'):
                #show error message showing name of class and file
                print("%s class does not have __neil__ metadict: %s" % (class_.__name__, modulefilename))
                continue

            classinfo = class_.__neil__
            classid = classinfo['id']
            self.factories[classid] = dict(classobj=class_, modulename=modulename)
            self.factories[classid].update(classinfo)
            # register categories
            for category in classinfo.get('categories', []):
                catlist = self.categories.get(category, [])
                catlist.append(classid)
                self.categories[category] = catlist

    def get_classes(self, neil_dict) -> List[type]:
        return neil_dict.get('classes', [])

    def throw(self, id, arg):
        class_ = self.factory(id)
        raise class_(arg)


    def factory(self, id):
        # get metainfo for object
        metainfo = self.factories.get(id, None)
        if not metainfo:
            print("no factory metainfo found for classid '%s'" % id)
            return None
        class_ = metainfo.get('classobj', None)
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



if __name__ == '__main__':
    class MyClass:
        __neil__ = dict(
            id = 'neil.hub.myclass',
            categories = [
                'uselessclass',
                'uselessclass2',
            ]
        )

        def __repr__(self):
            return '<%s>' % repr(self.x)

        def __init__(self, y=0):
            import random
            self.x = y or random.random()


    class MyClass2(MyClass):
        __neil__ = dict(
            id = 'neil.hub.myclass.singleton',
            singleton = True,
            categories = [
                'uselessclass',
                'uselessclass2',
            ]
        )


    class CancelException(Exception):
        __neil__ = dict(
            id = 'neil.exception.cancel',
            exception = True,
            categories = [
            ]
        )


    pkginfo = dict(
        classes = [
            MyClass,
            MyClass2,
            CancelException,
        ],
    )

    from neil import components
    
    components.register(pkginfo)
    print(components.get('neil.hub.myclass').x)
    print(components.get('neil.hub.myclass').x)
    print(components.get('neil.hub.myclass').x)
    print(components.get('neil.hub.myclass.singleton').x)
    print(components.get('neil.hub.myclass.singleton').x)
    print(components.get('neil.hub.myclass.singleton').x)
    print(components.category('uselessclass'))
    try:
        components.throw('neil.exception.cancel', "argh.")
    except components.exception('neil.exception.cancel') as test:
        print("passed.", test)
