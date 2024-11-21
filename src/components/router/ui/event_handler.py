import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gdk

from enum import IntEnum


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
    def matches(self, evt, context):
        return True
    
    # if True then stop 
    def handle(self, evt, context):
        return self.end
    
    # 
    def when_false(self) -> 'ActionHandler':
        if not hasattr(self, 'anti_handler'):
            handler_cls = get_class('ActionHandler')
            self.anti_handler = handler_cls()

        return self.anti_handler




# applies the callback with EventHandler arguments (event, context)
class ContextCallbackHandler(Handler):
    def __init__(self, callback):
        self.callback = callback


    # if return True then don't run any more handler 
    def handle(self, evt, context):
        return self.callback and self.callback(evt, context)



# applies the callback with gtk argument list (widget, event)
class GtkEventCallbackHandler(Handler):
    def __init__(self, callback):
        self.callback = callback

    def handle(self, evt, context):
        return self.callback and self.callback(context['self'], evt)
    
    
class SelectedCallback(Handler):
    def __init__(self, callback, send_rect):
        self.callback = callback
        self.send_rect = send_rect

    # if return True then don't run any more handler 
    def handle(self, evt, context):
        if not self.callback:
            return False
        
        selected = getattr(context,'selected', None)

        if not selected:
            return False
        
        if self.send_rect:
            return self.callback(evt, selected.object, selected.rect)
        else:
            return self.callback(evt, selected.object)


class ListHandler(Handler):
    def __init__(self):
        self.handlers = []


    def handle(self, evt, context):
        for handler in self.handlers:
            if handler.matches(evt, context):
                if handler.handle(evt, context):
                    return True
                return False
            elif hasattr(handler, 'anti_handler') and isinstance(handler.anti_handler, Handler):
                if handler.anti_handle.handle(evt, context):
                    return True
                return False
        

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



class ActionHandler(ListHandler):
    def if_attr(self, attr) -> 'ActionHandler':
        """
        Checks an attribute on the context widget
        So set self.dragging=True in the widget this eventhandler was connected with
        ie: 
        eventhandler.click_handler(Btn.Left).if_attr('self.dragging').do(callback) 
        will call the callback when left mouse button clicked and self.dragging=True
        """
        return self.if_attr_eq(attr, cmp=True)


    def if_not_attr(self, attr) -> 'ActionHandler':
        """
        Like if_attr but checks if value False/None
        """
        return self.if_attr_eq(attr, cmp=False)


    def if_attr_eq(self, attr, cmp) -> 'ActionHandler':
        """
        """
        # if '.' in attr:
        obj_name, attr_name = attr.split('.')
        handler_class = get_class('ContextPropertyHandler')
        attrs = {'attr_name': attr_name, 'object_name': obj_name}
        handler = self.get_matching_handler(handler_class, attrs)
        
        return handler if handler else self.add_handler(handler_class(obj_name, attr_name, cmp))
        # else:
        #     handler_class = get_class('ContextAttrHandler')
        #     attrs = {'attr_name': attr_name}
        #     handler = self.get_matching_handler(handler_class, attrs)

        #     return handler if handler else self.add_handler(handler_class(attr_name, match_value))

    def on_object(self, area_type) -> 'OnSelected':
        """
        Set: context['object'] = ClickArea.get_object_at(event.x, event.y)
        Check: context['object'].area == area_type 
        """
        return self.get_object_matcher().on_object(area_type)
    

    def get_object_matcher(self) -> 'GetSelectedObject':
        area_class = get_class('GetSelectedObject')
        handler = self.get_handler_for_class(area_class)
        return handler

    # def on_area_group(self, area_group) -> 'OnSelectedAreaGroup':
    #     """
    #     context['selected_group'] = ClickArea.get_object_group_at() and compare area_type to the result
    #     then test context['selected_group'].area | area_group_flag
    #     Area group flag is probably a single bit eg all Plugin area use 128
    #     """
    #     return self.get_area_group_matcher().area_group(area_group)
    
    # def get_area_group_matcher(self) -> 'GetSelectedAreaGroup':
    #     area_class = get_class('GetSelectedAreaGroup')
    #     handler = self.get_handler_for_class(area_class)
    #     return handler

    def do_ctx(self, callback) -> 'ActionHandler':
        """
        The callback receives arguments (event, context)
        The context dict always has value in 'self' holding widget, 
        It may also have the value in 'clicked' with resultof ClickArea.get_object_at()
        """
        self.add_handler(ContextCallbackHandler(callback))
        return self

    def do(self, callback, want_pos=True, end=False) -> 'ActionHandler':
        """
        The callback receives arguments 
        where object is the value of context['selected'] 
        @param callback: the callback to run (event, object) or (event, object, neil.utils.Area)
        @param send_rect: send the rectangle of the selected object as the third argument

        """
        self.add_handler(SelectedCallback(callback, want_pos))
        return self

    def do_gtk(self, gtk_callback) -> 'ActionHandler':
        """
        The callback receives arguments (event, context)
        The context dict always has value in 'self' holding widget, 
        Sometimes 'selected' is set with result of ClickArea.get_object_at(event.x, event.y)
        """
        self.add_handler(GtkEventCallbackHandler(gtk_callback))
        return self


    def when(self, matcher, end=False) -> 'CallbackMatcher':
        handler_cls = get_class('CallbackMatcher')
        handler = handler_cls(matcher)
        return self.add_handler(handler)



class CallbackMatcher(ActionHandler):
    def __init__(self, matcher):
        ActionHandler.__init__(self)
        self.matcher = matcher

    def matches(self, evt, context):
        return self.matcher(evt, context)



# Matches a property on an object in context
# context[object_name].attr_name == True
class ContextPropertyHandler(ActionHandler):
    def __init__(self, object_name, attr_name, match_value):
        ActionHandler.__init__(self)
        self.attr_name = attr_name
        self.object_name = object_name
        self.match_value = match_value


    def matches(self, evt, context):
        return self.object_name in context and getattr(context[self.object_name], self.attr_name) == self.match_value
            
    

# class ContextAttrHandler(ActionHandler):
#     def __init__(self, attr_name, match_value):
#         ActionHandler.__init__(self)
#         self.attr_name = attr_name
#         self.match_value = match_value


#     def matches(self, evt, context):
#         return self.attr_name in context and context[self.attr_name] == self.match_value



# Succeeds when this area_type matches the area_type of the return value from ClickArea.get_object_at() 

class OnSelected(ActionHandler):
    def __init__(self, area_type):
        ActionHandler.__init__(self)
        self.area_type = area_type


    def matches(self, event, context):
        return 'selected' in context and context['selected'] and context['selected'].type == self.area_type
    



# Stores the return value from ClickArea.get_object_at(evt.x, evt.y)
class GetSelectedObject(ListHandler):
    def handle(self, evt, context):
        if 'selected' not in context:
            context['selected'] = context['self'].get_object_at(evt.x, evt.y)
        
        return super().handle(evt, context)
    
    
    def on_object(self, area_type):
        return self.add_handler(OnSelected(area_type))
    

# class OnSelectedAreaGroup(ActionHandler):
#     def __init__(self, area_group_type):
#         ActionHandler.__init__(self)
#         self.area_group_type = area_group_type


#     def matches(self, event, context):
#         return 'group' in context and context['group'] and context['group'].type & self.area_group_type
    


# # Stores the return value from ClickArea.get_object_at(evt.x, evt.y)
# # Uses SelectedAreaMatcher
# class OnSelectedAreaGroup(ListHandler):
#     def handle(self, evt, context):
#         if 'group' not in context:
#             context['group'] = context['self'].get_object_at(evt.x, evt.y)
        
#         return super().handle(evt, context)
    
    
#     def on_area(self, area_type):
#         return self.add_handler(OnSelectedArea(area_type))
    

class ClickHandler(ActionHandler):
    def __init__(self,  button: Btn):
        ActionHandler.__init__(self)
        self.button = button
        self.signal = 'button-press-event'


    def matches(self, event, context):
        return event.button == self.button and event.type == Gdk.EventType.BUTTON_PRESS



class DoubleClickHandler(ActionHandler):
    def __init__(self, button: Btn):
        ActionHandler.__init__(self)
        self.button = button
        self.signal = 'button-press-event'


    def matches(self, event, context):
        return event.button == self.button and event.type == Gdk.EventType._2BUTTON_PRESS


class MotionHandler(ActionHandler):
    def __init__(self, button: Btn):
        ActionHandler.__init__(self)
        self.button = button
        self.signal = 'motion-notify-event'


    def matches(self, event, context):
        return True




class ReleaseHandler(ActionHandler):
    def __init__(self, button: Btn):
        ActionHandler.__init__(self)
        self.button = button
        self.signal = 'button-release-event'


    def matches(self, event, context):
        return event.button == self.button and event.type == Gdk.EventType.BUTTON_RELEASE



class EventHandler:
    PRESS = 1
    RELEASE = 2
    MOTION = 3

    def __init__(self):
        self.connect_signals = {
            EventHandler.PRESS: ('button-press-event', self.on_press),
            EventHandler.RELEASE: ('button-press-event', self.on_release),
            EventHandler.MOTION: ('motion-notify-event', self.on_motion),
        }

        self.handlers = { }

        self.signals = { }


    def use_handler(self, handler_class, handler_group, button = False):
        if handler_group not in self.handlers:
            self.handlers[handler_group] = []

        for handler in self.handlers[handler_group]:
            if not button or handler.button == button:
                return handler

        handler = handler_class(button)
        self.handlers[handler_group].append(handler)
        return handler


    def click_handler(self, btn: Btn) -> ClickHandler:
        return self.use_handler(ClickHandler, EventHandler.PRESS, btn)


    def double_click_handler(self, btn: Btn) -> DoubleClickHandler:
        return self.use_handler(DoubleClickHandler, EventHandler.PRESS, btn)
    

    def release_handler(self, btn: Btn) -> ReleaseHandler:
        return self.use_handler(ReleaseHandler, EventHandler.RELEASE, btn)
    

    def motion_handler(self) -> MotionHandler:
        return self.use_handler(MotionHandler, EventHandler.MOTION)
    

    def connect(self, widget):
        for handler_group in self.handlers.keys():
            signal_name, func = self.connect_signals[handler_group]
            id = widget.connect(signal_name, func)
            self.signals[handler_group] = id


    def disconnect(self, widget):
        for signal in self.signals.values():
            widget.disconnect(signal)


    def handle(self, handlers, event, widget):
        context = {'self': widget}
        for handler in handlers:
            if handler.handle(event, context):
                return True


    def on_press(self, widget, event):
        return self.handle(self.handlers[EventHandler.PRESS], event, widget)


    def on_release(self, widget, event):
        return self.handle(self.handlers[EventHandler.RELEASE], event, widget)


    def on_motion(self, widget, event):
        return self.handle(self.handlers[EventHandler.MOTION], event, widget)



