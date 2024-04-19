from ..utils.path_config import path_cfg
import os,sys,glob
from .package import PackageParser


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

        for path in component_path:
            print("scanning " + path + " for components")
            if os.path.isdir(path):
                use_pkg_dir = False
                for filename in glob.glob(os.path.join(path, '**.neil-component')):
                    pkg = parser.parse_package(filename)
                    if pkg.is_valid():
                        packages.append(pkg)
                        use_pkg_dir = True

                if use_pkg_dir and (not path in sys.path):
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
                manager.register(pkg, module_.__neil__, module_.__file__, modulename)
            except:
                from .. import errordlg
                errordlg.print_exc()

        

# looks for a __neil__ var in all python modules/packages under component path to auto detect components
class NeilDictComponentLoader():
    pass