from .base import settingspath, basepath, is_win32
from .path_config import path_cfg
import os

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