from .base import settingspath, basepath, is_win32
import os
import sys
from configparser import ConfigParser


if 'NEIL_BASE_PATH' in os.environ:
    BASE_PATH = os.environ['NEIL_BASE_PATH']
else:
    BASE_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), '../..'))



class PathConfig(ConfigParser):
    def __init__(self):
        ConfigParser.__init__(self)
        self.settings_dir = settingspath()

        try_paths = [
            os.path.join(settingspath(), 'path.cfg'), 
            os.environ['NEIL_PATH_CFG'] if 'NEIL_PATH_CFG' in os.environ else os.path.join(BASE_PATH, 'share/neil/path.cfg')
        ]

        path = self.get_cfg_path(try_paths)
        
        assert path, "Unable to find path.cfg"

        print("Using path config: " + path)

        self.read([path])

        site_packages = self.get_path('site_packages')
        if not site_packages in sys.path:
            print(site_packages + "  missing in sys.path, prepending")
            sys.exit(0)
            # sys.path = [site_packages] + sys.path


    def get_cfg_path(self, paths):
        for path in paths:
            path = os.path.expanduser(path)
            # if not os.path.isabs(path):
            #     path = os.path.normpath(os.path.join(BASE_PATH,path))
            # print("searching " + path)
            if os.path.isfile(path):
                return path


    def get_paths(self, pathid, append=''):
        paths = []
        default_path = self.get_path(pathid, append)
        if default_path:
            paths.append(default_path)
        paths.append(os.path.expanduser(os.path.join(self.settings_dir, pathid)))
        return paths
    

    def get_path(self, pathid, append=''):
        if not self.has_option('Paths', pathid):
            return ''
        value = os.path.expanduser(self.get('Paths', pathid))
        if not os.path.isabs(value):
            value = os.path.normpath(os.path.join(BASE_PATH, value))
        if append:
            value = os.path.join(value, append)
        return value


path_cfg = PathConfig()



def sharepath(path):
    """
    Translates a path relative to the config directory into an absolute
    path.

    @param path: Relative path to file.
    @type path: str
    @return: Absolute path to file.
    @rtype: str
    """
    return path_cfg.get_path('share', path)


def iconpath(path):
    """
    Translates a path relative to the neil icon directory into an absolute
    path.

    @param path: Relative path to file.
    @type path: str
    @return: Absolute path to file.
    @rtype: str
    """

    return path_cfg.get_path('icons_neil', path)


def hicoloriconpath(path):
    """
    Translates a path relative to the hicolor icon directory into an absolute
    path.

    @param path: Relative path to file.
    @type path: str
    @return: Absolute path to file.
    @rtype: str
    """
    return path_cfg.get_path('icons_hicolor', path)


def imagepath(path):
    """
    Translates a path relative to the image directory into an absolute
    path.

    @param path: Relative path to file.
    @type path: str
    @return: Absolute path to file.
    @rtype: str
    """
    return path_cfg.get_path('pixmaps', path)


def sharedpath(path):
    """
    Translates a path relative to the shared directory into an absolute
    path.

    @param path: Relative path to file.
    @type path: str
    @return: Absolute path to file.
    @rtype: str
    """
    return path_cfg.get_path('share', path)


def docpath(path):
    """
    Translates a path relative to the doc directory in to an absolute
    path.
    """
    return path_cfg.get_path('doc', path)


def filepath(path):
    """
    Translates a path relative to a base dir into an absolute
    path.
    WIN32: If the path leads to an svg and there is a similar file with
    png extension, use that one instead.

    @param path: Relative path to file.
    @type path: str
    @return: Absolute path to file.
    @rtype: str
    """
    path = os.path.abspath(os.path.normpath(os.path.join(basepath(),path)))
    if is_win32() and path.lower().endswith('.svg'):
        pngpath = os.path.splitext(path)[0] + '.png'
        if os.path.isfile(pngpath):
            return pngpath
    return path



__all__ = [
    'path_cfg',
    'settingspath',
    'basepath',
    'sharepath', 
    'iconpath', 
    'hicoloriconpath', 
    'imagepath', 
    'sharedpath', 
    'docpath', 
    'filepath',
]