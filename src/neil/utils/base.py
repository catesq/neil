# Base utils have zero internal dependencies because other utils depend on them and these need to be imported first  

from __future__ import annotations
import os, sys, re
from typing import Optional

class Vec2:
    x: int | float
    y: int | float
    
    # The str argument version of x and y is used internally by utils.Sizes class to parse int and float values
    def __init__(self, x: int | float | str | Vec2, y: Optional[int | float | str] = None):
        if isinstance(x, Vec2):
            self.x = x.x
            self.y = x.y
        else:
            self.x = x                          # type: ignore
            self.y = y if y is not None else x  # type: ignore

    def set(self, x, y=None):
        if isinstance(x, Vec2):
            self.x = x.x
            self.y = x.y
        else:
            self.x = x
            self.y = y if y is not None else x
        return self

    def move_by(self, x, y=None):
        if isinstance(x, Vec2):
            self.x += x.x
            self.y += x.y
        else:
            self.x += x
            self.y += y if y is not None else x
        return self
    
    def ints(self):
        self.x = int(self.x)
        self.y = int(self.y)
        return self
    
    def clear(self):
        self.x = self.y = 0

    def is_zero(self):
        return self.x == 0 and self.y == 0

    def __sub__(self, o):
        if isinstance(o, float | int):
            return Vec2(self.x - o, self.y - o)
        else:
            return Vec2(self.x - o.x, self.y - o.y)

    def __add__(self, o):
        if isinstance(o, float | int):
            return Vec2(self.x + o, self.y + o)
        else:
            return Vec2(self.x + o.x, self.y + o.y)
    
    def __truediv__(self, o):
        if isinstance(o, float | int):
            return Vec2(self.x / o, self.y / o)
        else:
            return Vec2(self.x / o.x, self.y / o.y)
    
    def __mul__(self, o):
        if isinstance(o, float | int):
            return Vec2(self.x * o, self.y * o)
        else:
            return Vec2(self.x * o.x, self.y * o.y)
    
    def __str__(self):
        return f"Vec2({self.x}, {self.y})"
    
    def __eq__(self, o):
        return self.x == o.x and self.y == o.y
    
    def __iter__(self):
        yield from (self.x, self.y)


class Area:
    def __init__(self, first = None, second = None, x2 = None, y2 = None):
        if isinstance(first, Area):
            self.pos = Vec2(first.pos)
            self.size = Vec2(first.size)
        elif isinstance(first, Vec2) and isinstance(second, Vec2):
            self.pos = Vec2(first)
            self.size = Vec2(second)
        elif first is not None and second is not None and x2 is not None and y2 is not None:
            self.pos = Vec2(first, second)
            self.size = Vec2(x2, y2)
        else:
            self.pos = Vec2(0, 0)
            self.size = Vec2(0, 0)

    # copies Area or gtk.Rectangle
    def set(self, rect):
        self.pos.set(rect.x, rect.y)
        self.size.set(rect.width, rect.height)
        return self
        
    def clear(self):
        self.pos.clear()
        self.size.clear()
    
    def __mul__(self, o):
        return Area(self.pos * o, self.size * o)
    
    def __truediv__(self, o):
        return Area(self.pos / o, self.size / o)
        
    # property access so x,y,width, height access properties of area
    @property
    def x(self):
        return self.pos.x

    @property
    def y(self):
        return self.pos.y

    @property
    def width(self):
        return self.size.x

    @property
    def height(self):
        return self.size.y

    def get_mid(self):
        return Vec2(self.x + self.width / 2, self.y + self.height / 2)

    def is_empty(self):
        return self.size.x == 0 or self.size.y == 0

    def contains(self, x, y):
        return (self.pos.x <= x <= self.pos.x + self.size.x and 
                self.pos.y <= y <= self.pos.y + self.size.y)

    def ints(self):
        self.pos.ints()
        self.size.ints()
        return self

    def __str__(self):
        return f"Area({self.pos}, {self.size})"

    def __eq__(self, o):
        return (self.x == o.x and self.y == o.y and 
               self.width == o.width and self.height == o.height)


    

# if the value is a string then calculate sizes using arithmetic with on values set earlier in the dict
# accepts lists and tuples and can access them in definitions using numbers as index (see pos & pos2 in example) 
# accepts Vec2, Area and floats
# everything else is converted to an int

# Sizes{
#   'width': 4, 
#   'height': 2, 
#   'size': 'width * height',
#   'rect': Area(0, 0, 'width', 'height'),
#   'pos': (10, 5),
#   'pos2': Vec2('pos.0 + width', 'pos.1 + height')
# }

# it handles floats and strings that generate a float
# it has some support for tuples and Vec2 containing the same types

class Sizes():
    _scale_ = 1.0
    
    def __init__(self, defaults = {'margin' : 6}, **kwargs):
        self.__defaults = defaults
        #copy defaults

        self.__values = defaults.copy()

        if kwargs:
            self.set_values(kwargs)

    @classmethod
    def one(cls, to_parse):
        return cls.instance().parse_value(to_parse)
    
    @classmethod
    def instance(cls):
        if not hasattr(cls, "_instance"):
            cls._instance = cls()
        return cls._instance

    def __getitem__(self, name) -> int | float | Area | Vec2 | list[int | float]:
        if name in self.__values:
            return self.__values[name]
        else:
            return 0
    

    def get(self, name):
        if name in self.__values:
            return self.__values[name]
        else:
            return 0  
    

    def half(self, name):
        return self.get(name) / 2



    def twice(self, name):
        return self.get(name) * 2

    
    def set_values(self, values):
        self.__values = self.__defaults.copy()
        
        for key, value in values.items():
            self.__values[key.lower()] = self.parse_value(value)
    
    
    def parse_value(self, value):
        if isinstance(value, str):
            return self.parse_size(value)
        elif isinstance(value, (list, tuple)):
            return [self.parse_value(v) for v in value]
        elif isinstance(value, Vec2):
            return Vec2(self.parse_value(value.x), self.parse_value(value.y))
        elif isinstance(value, Area):
            return Area(
                self.parse_value(value.pos.x), 
                self.parse_value(value.pos.y), 
                self.parse_value(value.size.x), 
                self.parse_value(value.size.y)
            )
        elif isinstance(value, float):
            return value
        else:
            return int(value)
        
        
    # uses eval to do basic math
    # treat text as the property name of an object in self.__values
    # treat numbers as the index of a list in self.__values  
    def parse_size(self, equation:str) -> int | float:
        match_pattern = re.compile(r'([\w\.]+)?')
        matches = match_pattern.findall(equation)

        for match in matches:
            match = match.strip()
            key = match.lower()
            
            if not match:
                continue
            
            if '.' in key:
                parts = key.split('.')

                value = self.__values
                for part in parts:
                    if part.isdigit() and isinstance(value, (list, tuple)):
                        value = value[int(part)]
                    elif hasattr(value, part):
                        value = getattr(value, part)
                    else:
                        value = value.get(part, 0)
                equation = re.sub(r'\b{}\b'.format(re.escape(match)), str(value), equation)
            elif key in self.__values:
                equation = re.sub(r'\b{}\b'.format(re.escape(match)), str(self.__values[key]), equation)

        try:
            result = eval(equation)
        except Exception as e:
            result = 0

        return result


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




__all = [
    'is_frozen', 
    'roundint', 
    'is_win32',

    'Vec2',
    'Area',
    'OffsetArea',
    'Sizes',
]