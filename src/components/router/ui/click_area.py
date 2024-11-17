from typing import Tuple
from neil.utils import Area







class ClickArea:
    def __init__(self):
        self.objects = []

    def add_object(self, obj, area: Area, area_type: int):
        """
        Registers a new object.

        @param obj: The ui object to associate with the click box.
        @param area: The x coordinate of the top-left corner of the click box.
        """
        self.objects.append((obj, area, area_type))

    def remove_object(self, obj):
        """
        Removes an object.

        @param obj: The object whose click box should be removed.
        """
        prev_size = self.objects.size()
        self.objects = [click_box for click_box in self.objects if click_box[0] != obj]
        return self.objects.size() == prev_size

    def get_object_at(self, x, y) -> Tuple[object, Area, int]:
        """
        Finds the object at a specific position.

        @param x: The x coordinate.
        @param y: The y coordinate.
        @return: The object and area at the specified position, or None,None if no object is found.
        """
        for obj, area, area_type in self.objects:
            if area.contains(x, y):
                return obj, area, area_type
        return None, None, None

    def get_object_group_at(self, x, y, obj_group) -> Tuple[object, Area, int]:
        """
        Finds the object at a specific position.

        @param x: The x coordinate.
        @param y: The y coordinate.
        @return: The object and area at the specified position, or None,None if no object is found.
        """
        for obj, area, area_type in self.objects:
            if (area_type & obj_group) == obj_group and area.contains(x, y):
                return obj, area, area_type
        return None, None, None

    def clear(self):
        self.objects.clear()

