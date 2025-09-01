from neil.utils import Area
from zzub import Plugin, Connection



class ClickedArea:
    def __init__(self, id, object, rect:Area, area_type):
        self.id = id
        self.object = object
        self.rect = rect
        self.type = area_type

    def __repr__(self):
        return f"ClickedArea({self.id} {self.object}, {self.rect}, {self.type})"



class ClickArea:
    def __init__(self):
        self.objects = []


    def add_object(self, id, object, area, area_type):
        """
        Registers a new object.

        @param obj: The ui object to associate with the click box.
        @param area: The click box rectangle
        @param area_type Area type
        """
        self.objects.append(ClickedArea(id, object, area, area_type))


    def remove_object(self, obj):
        """
        Removes an object.

        @param obj: The object to remove
        @return: The number of objects removed.
        """
        prev_size = len(self.objects)
        self.objects = [click_box for click_box in self.objects if click_box.object != obj]
        return prev_size - len(self.objects)
    

    def remove_by_id(self, id):
        """
        Removes an object.

        @param id: id of the oject to remove. It's is an int for plugins and ConnID for connections
        @return: The number of objects removed.
        """
        prev_size = len(self.objects)
        self.objects = [click_box for click_box in self.objects if click_box.id != id]
        return prev_size - len(self.objects)
    

    def get_object_at(self, x, y) -> ClickedArea | None:
        """
        Finds the first object at a specific position.

        @param x: The x coordinate.
        @param y: The y coordinate.
        @return: The object and area at the specified position, or None,None if no object is found.
        """
        for item in self.objects:
            if item.rect.contains(x, y):
                return item
            
        return None


    def get_object_group_at(self, x, y, obj_group) -> ClickedArea | None:
        """
        Finds the first object of type 'obj_group' at a specific position.

        @param x: The x coordinate.
        @param y: The y coordinate.
        @return: The object and area at the specified position, or None,None if no object is found.
        """
        for found in self.objects:
            # print(found)
            if (found.type & obj_group) and found.rect.contains(x, y):
                return found
            
        return None


    def get_all_type_at(self, x, y, obj_type) -> list[ClickedArea]:
        """
        Finds all objects of type 'obj_group' at a specific position.

        @param x: The x coordinate.
        @param y: The y coordinate.
        @return: A list of objects and areas at the specified position.
        """
        return [found for found in self.objects if (found.type & obj_type) and found.rect.contains(x, y)]


    def clear(self):
        self.objects.clear()

