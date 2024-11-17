# utils with zero internal dependencies that can be imported first when other utils depend on them  

import os, sys, re


class Vec2:
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def move_to(self, x, y):
        self.x = x
        self.y = y

    def move_by(self, x, y):
        self.x += x
        self.y += y

    def __sub__(self, o):
        if isinstance(o, Vec2):
            return Vec2(self.x - o.x, self.y - o.y)
        else:
            return Vec2(self.x - o, self.y - o)

    def __add__(self, o):
        if isinstance(o, Vec2):
            return Vec2(self.x + o.x, self.y + o.y)
        else:
            return Vec2(self.x + o, self.y + o)
    
    def __truediv__(self, o):
        if isinstance(o, Vec2):
            return Vec2(self.x / o.x, self.y / o.y)
        else:
            return Vec2(self.x / o, self.y / o)
    
    def __mul__(self, o):
        if isinstance(o, Vec2):
            return Vec2(self.x * o.x, self.y * o.y)
        else:
            return Vec2(self.x * o, self.y * o)
    
    def __str__(self):
        return f"Vec2({self.x}, {self.y})"
    
    def eq(self, o):
        return self.x == o.x and self.y == o.y


    
# this is the abstract canvas area in made up dimensions , not the screen pos in pixels
class Area:
    def __init__(self, pos = None, size = None):
        self._pos = pos if pos is not None else Vec2(0, 0)
        self._size = size if size is not None else Vec2(0, 0)

    # copies Area or gtk.Rectangle
    def copy(self, rect):
        self._pos.move_to(rect.x, rect.y)
        self._size.move_to(rect.width, rect.height)
        return self
    
    # property access so x,y,width, height access properties of area
    @property
    def x(self):
        return self._pos.x
    
    @property
    def y(self):
        return self._pos.y
    
    @property
    def width(self):
        return self._size.x
    
    @property
    def height(self):
        return self._size.y
    
    def is_empty(self):
        return self._size.x == 0 or self._size.y == 0

    def contains(self, x, y):
        return (self._pos.x <= x <= self._pos.x + self._size.x and 
                self._pos.y <= y <= self._pos.y + self._size.y)

    def get_size(self):
        return self._size
    
    def __str__(self):
        return f"Area({self._pos}, {self._size})"
    
    def eq(self, o):
        return self._pos.eq(o._pos) and self._size.eq(o._size)

class OffsetArea(Area):
    def __init__(self, pos, offset, size):
        super().__init__(pos, size)
        self._offset = offset

    @property
    def x(self):
        return self._pos.x + self._offset.x

    @property
    def y(self):
        return self._pos.y + self._offset.y

    def contains(self, x, y):
        return super().contains(x - self._offset.x, y - self._offset.y)

    def __str__(self):
        return f"OffsetArea(pos={self._pos}, offset={self._offset}, size={self._size})"

# it can scale for hidpi &
# calculate sizes based on earlier values using parse_size())
#
# eg values={
#   'width': 4, 
#   'height': 2, 
#   'area': 'width * height',
#   'pos': (10, 5)
#   'pos2': Vec2('pos.0 + width', 'pos.1 + height')
# }

# it handles floats and strings that generate a float
# it has some support for tuples and Vec2 containing the same types

class Sizes():
    _scale_ = 1.0

    def __init__(self, defaults = {'margin' : 6}, **kwargs):
        self.__defaults = defaults
        self.__init_args = kwargs
        self.__scale = 1.0
        self.__values = []
        self.set_values(kwargs)

    def set_scale(self, new_scale):
        self.__scale = new_scale
        self.set_values(self.__init_args)

    def get_scale(self, new_scale):
        return self.__scale
    
    def __getitem__(self, name):
        return self.get(name)
    
    def get(self, name) -> float:
        if name in self.__values:
            # scale tuples
            if isinstance(self.__values[name], Vec2):
                return Vec2(
                    self.__values[name].x * self.__scale, 
                    self.__values[name].y * self.__scale
                )
            elif isinstance(self.__values[name], (list, tuple)):
                return [v * self.__scale for v in self.__values[name]]
            else:
                return self.__values[name] * self.__scale
    
    def half(self, name):
        return self.get(name) / 2

    def twice(self, name):
        return self.get(name) * 2

    
    def set_values(self, values):
        self.__values = self.__defaults
        for key, value in values.items():
            self.__values[key.lower()] = self.parse_value(value)
    
    def parse_value(self, value):
        if isinstance(value, str):
            return self.parse_size(value)
        elif isinstance(value, (list, tuple)):
            return [self.parse_value(v) for v in value]
        elif isinstance(value, Vec2):
            return Vec2(self.parse_value(value.x), self.parse_value(value.y))
        elif isinstance(value, float):
            return value
        else:
            return int(value)
        
    # uses eval to do basic math
    # treat text as the property name of an object in self.__values
    # treat numbers as the index of a list in self.__values  
    def parse_size(self, equation:str) -> int | float:
        match_pattern = re.compile(r'(\w+)?')
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
                    if part.isdigit():
                        value = value[int(part)]
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