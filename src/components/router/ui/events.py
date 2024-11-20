import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gdk

from enum import IntEnum
from .click_area import ClickArea


# Event filter uses bitfields as button id instead of 1,2,3 so we can use (Btn.LEFT + Btn.MIDDLE) 
# in on_click_area() to filter either left or middle button press
class Btn(IntEnum):
    LEFT = 1
    RIGHT = 2
    MIDDLE = 3


def get_class(class_name):
    if class_name in globals():
       return globals().get(class_name, None)


class Handler:
    def __init__(self, last_handler = False):
        self.last_handler = last_handler

    def matches(self, evt, context):
        return True
    
    # if return True then don't run any more handler 
    def do_handler(self, evt, context):
        return self.last_handler



class CallbackHandler(Handler):
    def __init__(self, callback, last_handler = False):
        Handler.__init__(self, last_handler)
        self.callback = callback

    # if return True then don't run any more handler 
    def do_handler(self, evt, context):
        return self.callback(evt, context) or self.last_handler



class ListHandler(Handler):
    def __init__(self, last_handler = False):
        Handler.__init__(self, last_handler)
        self.handlers = []

    def do_handler(self, evt, context):
        for handler in self.handlers:
            if handler.matches(evt, context):
                if handler.do_handler(evt, context):
                    self.last_handler

    def get_handler_for_class(self, handler_class, *args, **kwargs):
        for handler in self.handlers:
            if isinstance(handler, handler_class):
                return handler
        return self.add_handler(handler_class(*args, **kwargs))
    
    def get_handlers_for_class(self, handler_class):
        return [handler for handler in self.handlers if isinstance(handler, handler_class)]
    
    def get_matching_handler(self, handler_class, match_attrs):
        for handler in self.get_handlers_for_class(handler_class):
            matched = True
            for attr, value in match_attrs.items():
                if getattr(handler, attr) != value:
                    matched = False

            if matched:
                return handler

    def add_handler(self, new_handler):
        self.handlers.append(new_handler)
        return new_handler
    
    def do(self, callback, last_handler = False):
        self.add_handler(CallbackHandler(callback, last_handler))
        return self


class ActionHandler(ListHandler):
    def if_attr(self, attr):
        if '.' in attr:
            obj_name, attr_name = attr.split('.')
            handler_class = get_class('ContextObjAttrHandler')
            attrs = {'attr_name': attr_name, 'object_name': obj_name}
            handler = self.get_matching_handler(handler_class, attrs)
            
            return handler if handler else self.add_handler(handler_class(obj_name, attr_name))
        else:
            handler_class = get_class('ContextAttrHandler')
            attrs = {'attr_name': attr_name}
            handler = self.get_matching_handler(handler_class, attrs)

            return handler if handler else self.add_handler(handler_class(attr_name))

    def when_area(self, area_type, last_handler = False) -> 'AreaHandler':
        area_class = get_class('AreaListHandler')
        handler = self.get_handler_for_class(area_class)
        return handler.add_area(area_type, last_handler)


class ContextObjAttrHandler(ActionHandler):
    def __init__(self, object_name, attr_name, last_handler = False):
        ActionHandler.__init__(self, last_handler)
        self.attr_name = attr_name
        self.object_name = object_name

    def matches(self, evt, context):
        return self.object_name in context and getattr(context[self.object_name], self.attr_name)
            
    

class ContextAttrHandler(ActionHandler):
    def __init__(self, attr_name, last_handler = False):
        ActionHandler.__init__(self, last_handler)
        self.attr_name = attr_name

    def matches(self, evt, context):
        return self.attr_name in context and context[self.attr_name]







class AreaHandler(ActionHandler):
    def __init__(self, area_type, last_handler = False):
        ActionHandler.__init__(self, last_handler)
        self.area_type = area_type

    def matches(self, event, context):
        return context['selected'][2] == self.area_type
    
    def when_area(self, area_type):
        pass



class AreaListHandler(ListHandler):
    def do_handler(self, evt, context):
        if 'selected' not in context:
            context['selected'] = context['self'].get_object_at(evt.x, evt.y)
        
        return super().do_handler(evt, context)

    def add_area(self, area_type, last_handler = False):
        return self.add_handler(AreaHandler(area_type, last_handler))



class ClickHandler(ActionHandler):
    def __init__(self,  button: Btn):
        ActionHandler.__init__(self)
        self.button = button

    def matches(self, event, context):
        return event.button == self.button and event.type == Gdk.EventType.BUTTON_RELEASE



class DoubleClickHandler(ActionHandler):
    def __init__(self, button: Btn):
        ActionHandler.__init__(self)
        self.button = button

    def matches(self, event, context):
        return event.button == self.button and event.type == Gdk.EventType._2BUTTON_PRESS



class ReleaseHandler(ActionHandler):
    def __init__(self, button: Btn):
        ActionHandler.__init__(self)
        self.button = button

    def matches(self, event, context):
        return event.button == self.button and event.type == Gdk.EventType.BUTTON_RELEASE



class EventHandler(ListHandler):
    def __init__(self):
        ListHandler.__init__(self)
        self.signals = []



    def get_click_handler(self, handler_class, button):
        for handler in self.get_handlers_for_class(handler_class):
            if handler.button == button:
                return handler
        return self.add_handler(handler_class(button))

    def click_handler(self, btn: Btn) -> ClickHandler:
        return self.get_click_handler(ClickHandler, btn)

    def double_click_handler(self, btn: Btn) -> DoubleClickHandler:
        return self.get_click_handler(DoubleClickHandler, btn)
    
    def release_handler(self, btn: Btn) -> ReleaseHandler:
        return self.get_click_handler(ReleaseHandler, btn)
    
    def connect(self, widget):
        self.add_signal(widget, 'button-press-event')
        self.add_signal(widget, 'button-release-event')

    def disconnect(self, widget):
        for signal in self.signals:
            signal[1].disconnect(signal[0])


    def add_signal(self, widget, signal_name):
        id = widget.connect(signal_name, lambda widget, event: self.do_handler(event, {'self': widget}))

        if id:
            self.signals.append((id, widget, signal_name))

        return id


