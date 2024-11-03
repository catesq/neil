from ..utils.path_config import path_cfg
import os,sys,glob
from .package import PackageParser
from .. import errordlg



# looks for files like "core-about.neil-component" in component path then tries to load matching package
class NamedComponentLoader():
    def __init__(self):
        pass

    def load(self, manager):
        self.is_loaded = True
        self.packages = []
        packages = []
        names = []

        component_path = [
            path_cfg.get_path('components') or os.path.join(path_cfg.get_path('share'), 'components'),
            os.path.expanduser('~/.local/neil/components'),
        ]

        parser = PackageParser()

        print("scanning for components in: " + ', '.join(component_path))
        for path in component_path:
            if os.path.isdir(path):
                for filename in self.get_component_files(path):
                    pkg = parser.parse_package(filename)
                    if pkg.is_valid():
                        packages.append(pkg)

                if not path in sys.path:
                    sys.path = [path] + sys.path

        for pkg in packages:
            try:
                modulename = pkg.module
                module_ = __import__(modulename)
                
                names = modulename.split('.')
                for name in names[1:]:
                    module_ = getattr(module_, name)

                if not hasattr(module_, '__neil__'):
                    print("module", modulename, "has no __neil__ metadict")
                    continue

                manager.register(pkg, module_.__neil__, module_.__file__)
            except:
                errordlg.print_exc()


    # gets all .neil-component info files in path and path/package_info 
    # the matching .py file for each component in immediately under path
    # did this for a cleaner file structure as the components subdir was very messy
    def get_component_files(self, top_path):
        return glob.glob(os.path.join(top_path, '*.neil-component')) + glob.glob(os.path.join(top_path, 'packages', '*.neil-component')) 



# looks for a __neil__ var in all python modules under component path to auto detect view components
class NeilDictComponentLoader():
    pass