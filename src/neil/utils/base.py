# utils with zero internal dependencies that can be imported first when other utils depend on them  

import os, sys



def is_frozen():
    """
    Determines whether the application is being executed by
    a Python installation or it is running standalone (as a
    py2exe executable.)

    @return: True if frozen, otherwise False
    @rtype: bool
    """
    return (hasattr(sys, "frozen") or # new py2exe
            hasattr(sys, "importers")) # old py2exe
            # or imp.is_frozen("__main__")) # tools/freeze



def basepath():
    """
    Returns the base folder from which this script is being
    executed. This is mainly used for windows, where loading
    of resources relative to the execution folder must be
    possible, regardless of current working directory.

    @return: Path to execution folder.
    @rtype: str
    """
    if is_frozen():
        return os.path.dirname(sys.executable)
    return os.path.abspath(os.path.normpath(os.path.join(os.path.dirname(__file__))))


def settingspath():
    if os.name == 'nt':
        return os.path.expanduser('~/neil')
    elif os.name == 'posix':
        return os.path.expanduser('~/.neil')
    else:
        print("Unsupported OS")
        sys.exit(1)


def roundint(v):
    """
    Rounds a float value to the next integer if its
    fractional part is larger than 0.5.

    @type v: float
    @rtype: int
    """
    return int(v+0.5)



def is_win32():
    return sys.platform == 'win32'




__all__ = [
    'is_frozen', 
    'roundint', 
    'is_win32'
]