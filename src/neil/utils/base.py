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
        return Vec2(self.x - o.x, self.y - o.y)

    def __add__(self, o):
        return Vec2(self.x + o.x, self.y + o.y)

    def __str__(self):
        return f"Vec2({self.x}, {self.y})"
    
    def eq(self, o):
        return self.x == o.x and self.y == o.y 

class OffsetArea:
    def __init__(self, pos, offset, size):
        self.pos = pos
        self.offset = offset
        self.size = size

    def contains(self, x, y):
        return (self.pos.x + self.offset.x <= x <= self.pos.x + self.offset.x + self.size.x and 
                self.pos.y + self.offset.y <= y <= self.pos.y + self.offset.y + self.size.y)

    def __str__(self):
        return f"OffsetArea({self.pos}, {self.size}, {self.offset})"

    
# this is the abstract canvas area in made up dimensions , not the screen pos in pixels
class Area:
    def __init__(self, pos, size):
        self.pos = pos
        self.size = size

    def move_to(self, x, y):
        self.pos.x = x
        self.pos.y = y

    def contains(self, x, y):
        return (self.pos.x <= x <= self.pos.x + self.size.x and 
                self.pos.y <= y <= self.pos.y + self.size.y)

    def __str__(self):
        return f"Area({self.pos}, {self.size})"



# store pixel dimensions for the ui
# it can scale for hidpi &
# calculate sizes based on earlier values using parse_size())
# eg values={
#   'width': 4, 
#   'height': 2, 
#   'area': 'width * height'
# }

class Sizes():
    _scale_ = 1.0

    def __init__(self, defaults = {'margin' : 6}, **kwargs):
        self.__defaults__ = defaults
        self.__init_args__ = kwargs
        self.__scale__ = 1.0
        self.set_values(kwargs)

    def set_scale(self, new_scale):
        self.__scale__ = new_scale
        self.set_values(self.__init_args__)

    def get_scale(self, new_scale):
        return self.__scale__
    
    def get(self, name) -> float:
        if name in self.values:
            return self.values[name]
    
    def half(self, name):
        return self.get(name, 0.5)

    def twice(self, name):
        return self.get(name, 2)

    def __getattr__(self, name):
        return self.get(name)
    
    def set_values(self, values):
        self.values = self.__defaults__
        for key, value in values.items():
            self.values[key.lower()] = self.parse_value(value)
    
    def parse_value(self, value):
        if isinstance(value, str):
            return self.parse_size(value)
        elif isinstance(value, (list, tuple)):
            return [self.parse_value(v) for v in value]
        elif isinstance(value, Vec2):
            return Vec2(self.parse_value(value.x, self.parse_value(value.y)))
        elif isinstance(value, float):
            return value
        else:
            return int(value)
        
    #uses eval to do basic math. treat any text as the key to an entry in self.values
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
                value = self.values
                for part in parts:
                    if part.isdigit():
                        value = value[int(part)]
                    else:
                        value = value.get(part, 0)
                equation = re.sub(r'\b{}\b'.format(re.escape(match)), str(value), equation)
            elif key in self.values:
                equation = re.sub(r'\b{}\b'.format(re.escape(match)), str(self.values[key]), equation)

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




__all__ = [
    'is_frozen', 
    'roundint', 
    'is_win32',

    'Vec2',
    'Area',
    'OffsetArea',
    'Sizes',
]