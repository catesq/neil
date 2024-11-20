from typing import Tuple
from neil.utils import Area

class Clicked:
    def __init__(self, object, rect:Area, area_type):
        self.object = object
        self.rect = rect
        self.type = area_type

class ClickArea:
    def __init__(self):
        self.objects = []

    def add_object(self, obj, area: Area, area_type: int):
        """
        Registers a new object.

        @param obj: The ui object to associate with the click box.
        @param area: The x coordinate of the top-left corner of the click box.
       
        """
        self.objects.append(Clicked(obj, area, area_type))

    def remove_object(self, obj):
        """
        Removes an object.

        @param obj: The object whose click box should be removed.
        @return: The number of objects removed.
        """
        prev_size = self.objects.size()
        self.objects = [click_box for click_box in self.objects if click_box.object != obj]
        return prev_size - self.objects.size()
    
    def get_object_at(self, x, y) -> Clicked:
        """
        Finds the object at a specific position.

        @param x: The x coordinate.
        @param y: The y coordinate.
        @return: The object and area at the specified position, or None,None if no object is found.
        """
        for item in self.objects:
            if item.rect.contains(x, y):
                return item

    def get_object_group_at(self, x, y, obj_group) -> Clicked:
        """
        Finds the object at a specific position.

        @param x: The x coordinate.
        @param y: The y coordinate.
        @return: The object and area at the specified position, or None,None if no object is found.
        """
        for item in self.objects:
            if (item.type & obj_group) == obj_group and item.rect.contains(x, y):
                return item

    def clear(self):
        self.objects.clear()

