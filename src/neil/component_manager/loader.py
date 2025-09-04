from ..utils.path_config import path_cfg
import os,sys,glob
from .package import PackageParser
from .. import errordlg


# looks for files ending in ".neil-component" in component path and the 'packages' subdirectory of eahc directory in component path - 
# then build a PackageInfo from the info in that file

# For each PackageInfo it tries importing the module name. 

# If the module has a __neil__ dict it registers the PackageInfo, the module file and the class names listed in the __neil__ dict 

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
                    if pkg and pkg.is_valid():
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
    def get_component_files(self, top_path):
        return glob.glob(os.path.join(top_path, '*.neil-component')) + glob.glob(os.path.join(top_path, 'packages', '*.neil-component')) 



# looks for a __neil__ var in all python modules under component path to auto detect view components
class NeilDictComponentLoader():
    pass