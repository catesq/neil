import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gdk

from enum import IntEnum
from . import click_area

# Event filter uses bitfields as button id instead of 1,2,3 so we can use (Btn.LEFT + Btn.MIDDLE) 
# in on_click_area() to filter either left or middle button press
class Btn(IntEnum):
    LEFT = 1
    MIDDLE = 2
    RIGHT = 3


def get_class(class_name):
    if class_name in globals():
       return globals().get(class_name, None)



def run_handlers(handlers, evt, context):
    for handler in handlers:
        # print("try handler:", handler)
        if handler.matches(evt, context):
            # print("handler matched", handler)
            if handler.handle(evt, context):
                return True
        elif hasattr(handler, 'not_handler') and isinstance(handler.not_handler, Handler):
            # print("running not handler")
            if handler.not_handler.handle(evt, context):
                return True



class Handler:
    def __init__(self):
        self.click_area: click_area.ClickArea = None
        self.not_handler: 'ActionHandler' = None

    def matches(self, evt, context) -> bool:
        return True
    
    # if True then stop 
    def handle(self, evt, context) -> bool:
        return False
    
    # 
    def if_false(self) -> 'ActionHandler':
        if not hasattr(self, 'not_handler'):
            handler_cls = get_class('ActionHandler')
            self.not_handler = handler_cls()

        return self.not_handler




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
    
    
class SelectedObjectCallback(Handler):
    def __init__(self, callback, context_key, want_area):
        self.callback = callback
        self.context_key = context_key
        self.want_area = want_area

    def matches(self, evt, context):
        return self.context_key in context and context[self.context_key] and hasattr(context[self.context_key], 'object')

    def handle(self, evt, context):
        result = context[self.context_key]
        
        if self.want_area:
            return self.callback(evt, result.object, result.rect)
        else:
            return self.callback(evt, result.object)


class SelectedResultCallback(Handler):
    def __init__(self, callback, context_key, always_call = False):
        self.callback = callback
        self.context_key = context_key
        self.always_call = always_call

    def matches(self, evt, context):
        return self.context_key in context
        
    def handle(self, evt, context):
        if context[self.context_key] or self.always_call:
            return self.callback(evt, context[self.context_key])
        


class BasicCallback(Handler):
    def __init__(self, callback):
        self.callback = callback

    def handle(self, evt, context):
        return self.callback(evt)

        
        


class ListHandler(Handler):
    def __init__(self):
        self.handlers = []
        
    def handle(self, evt, context):
        return run_handlers(self.handlers, evt, context)

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

        if hasattr(self, 'context_key') and not getattr(new_handler, 'context_key', False):
            new_handler.context_key = self.context_key

        if hasattr(self, 'click_area') and not getattr(new_handler, 'click_area', False):
            new_handler.click_area = self.click_area
            
        return new_handler



class ActionHandler(ListHandler):
    def __init__(self):
        ListHandler.__init__(self)
        

    def if_attr(self, attr) -> 'ActionHandler':
        """
        Checks an attribute on the context widget
        So set self.dragging=True in the widget this eventhandler was connected with
        ie: eventhandler.click_handler(Btn.Left).if_attr('self.dragging').do(callback) 
            runs callback(event, context) when left mouse button clicked and widget.dragging=True
        """
        return self.if_attr_eq(attr, cmp=True)


    def if_not_attr(self, attr) -> 'ActionHandler':
        """
        Like if_attr but checks if value False/None
        @param attr: the name of the attribute
        """
        return self.if_attr_eq(attr, cmp=False)


    def if_attr_eq(self, attr, cmp) -> 'ActionHandler':
        """
        Check an attr has a specific value
        @param attr: the name of the attribute on the connected widget to check
        @param cmp: the value to compare against
        """
        # if '.' in attr:
        obj_name, attr_name = attr.split('.')
        handler_class = get_class('ContextPropertyHandler')
        attrs = {'attr_name': attr_name, 'object_name': obj_name}
        handler = self.get_matching_handler(handler_class, attrs)
        
        return handler if handler else self.add_handler(handler_class(obj_name, attr_name, cmp))


    def do_ctx(self, callback) -> 'ActionHandler':
        """
        The callback receives arguments (event, context)
        The context dict always has value in 'self' holding widget, 
        It may also have the value in 'clicked' with resultof ClickArea.get_object_at()
        """
        self.add_handler(ContextCallbackHandler(callback))
        return self


    def do(self, callback, want_area=False) -> 'ActionHandler':
        """
        The callback receives arguments:
            (event)                                            # when on_group() or on_object() not used
            (event, object: zzub.Plugin|items.ConnId)          # after on_group() or on_object()
            (event, object: zzub.Plugin|items.ConnId, neil.utils.Area)  # if want_area = True 

        @param callback: the callback to run (event, object) or (event, object, neil.utils.Area)
        @param want_area: send the rectangle of the selected object as the third argument
        """
        # When context_key is set then use SelectedObjectCallback check context[context_key]
        #    The context_key is either 'object' or 'group', 
        #    context[context_key] is the result of a ClickArea search. 
        #    The SelectedObjectCallback isn't called when the result is false
        # When context key not set then use BasicCallback and always call the result
        if hasattr(self, 'context_key'):
            self.add_handler(SelectedObjectCallback(callback, self.context_key, want_area))
        else:
            self.add_handler(BasicCallback(callback))

        return self


    def do_if_result(self, callback) -> 'ActionHandler':
        """
        The callback only runs if the result was True
        The callback receives arguments (event, clicked: ClickedArea) if result not False
        @param callback: the callback to run 
        """
        self.add_handler(SelectedResultCallback(callback, self.context_key))
        return self
    

    def do_result(self, callback) -> 'ActionHandler':
        """
        The callback receives arguments (event, clicked: ClickedArea|False) whatever result is
        @param callback: the callback to run 
        """
        self.add_handler(SelectedResultCallback(callback, self.context_key, True))
        return self


    def do_gtk(self, gtk_callback) -> 'ActionHandler':
        """
        The callback receives arguments (event, context)
        The context dict always has value in 'self' holding widget, 
        Sometimes 'selected' is set with result of ClickArea.get_object_at(event.x, event.y)
        """
        self.add_handler(GtkEventCallbackHandler(gtk_callback))
        return self


    def when(self, matcher) -> 'CallbackMatcher':
        """
        @param matcher: callable
        """
        handler_cls = get_class('CallbackMatcher')
        handler = handler_cls(matcher)
        return self.add_handler(handler)

    
    def on_object(self, object_type) -> 'OnObject':
        """
        Find which part of which widget was clicked
        Set: context['object'] = ClickArea.get_object_at(event.x, event.y)
        Check: context['object'].area == area_type 
        """
        return self.get_object_matcher(object_type).on_object(object_type)
    

    def get_object_matcher(self, object_type) -> 'OnObjectSearch':
        """
        Run clickArea.get_object_at(object_type) then set context['object'] then run sub handlers 
        
        Will reuse an existing object matcher if it searched for the same group
        If a new group matcher is created it replaces the context['group'] result
        """
        object_search_class = get_class('OnObjectSearch')
        # handler = self.get_matching_handler(object_search_class, {'match_object_type': object_type})

        return self.get_handler_for_class(object_search_class, object_type)


    def on_group(self, group_type) -> 'OnGroup':
        """
        If a widget has multiple objects that can be clicked, they can be grouped using the id
        so on_group can match any of them
        Set context['group'] to ClickArea.get_object_group_at(group_type) and compare area_type of the result
        """
        return self.get_group_matcher(group_type).on_group(group_type)
    

    def get_group_matcher(self, group_type) -> 'OnGroupSearch':
        """
        Run clickArea.get_object_group_at(group_type) then set context['group'] then run sub handlers
        
        Will reuse an existing group matcher if it searched for the same group
        If a new group matcher is created it replaces the context['group'] result
        """
        group_search_class = get_class('OnGroupSearch')
        handler = self.get_matching_handler(group_search_class, {'match_area_group': group_type})

        return handler if handler else self.add_handler(group_search_class(group_type))



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
        self.object_name = object_name
        self.attr_name = attr_name
        self.match_value = match_value

    def matches(self, evt, context):
        # print("ContextPropertyHandler object '{}', attr '{}', to match '{}'".format(self.object_name, self.attr_name, 'true' if self.match_value else 'false'))
        res = self.object_name in context and getattr(context[self.object_name], self.attr_name) == self.match_value
        # print("self.dragging = {} -> {} ".format(str(getattr(context[self.object_name], self.attr_name)), str(res)))

        return self.object_name in context and getattr(context[self.object_name], self.attr_name) == self.match_value
            
    

# class ContextAttrHandler(ActionHandler):
#     def __init__(self, attr_name, match_value):
#         ActionHandler.__init__(self)
#         self.attr_name = attr_name
#         self.match_value = match_value


#     def matches(self, evt, context):
#         return self.attr_name in context and context[self.attr_name] == self.match_value



# Succeeds when this area_type matches the area_type of the return value from ClickArea.get_object_at() 

class OnObject(ActionHandler):
    def __init__(self, match_area):
        ActionHandler.__init__(self)
        # Used in OnObject.matches() to check the result's area type
        self.match_area = match_area
        self.context_key = 'object'

    def matches(self, event, context):
        return 'object' in context and context['object'] and context['object'].type == self.match_area
    



# Stores the return value from ClickArea.get_object_at(evt.x, evt.y)
class OnObjectSearch(ListHandler):
    def __init__(self, match_object_type):
        ListHandler.__init__(self)

    def matches(self, evt, context):
        if 'object' not in context:
            context['object'] = self.click_area.get_object_at(evt.x, evt.y)
        
        return True
    
    
    def on_object(self, match_area):
        return self.add_handler(OnObject(match_area))
    





class OnGroup(ActionHandler):
    def __init__(self, match_area_group):
        ActionHandler.__init__(self)
        # Used in OnGroup.matches() to check the result's area type
        self.match_group = match_area_group
        self.context_key = 'group'


    def matches(self, event, context):
        return 'group' in context and context['group'] and context['group'].type & self.match_group
    
# Stores the return value from ClickArea.get_object_at(evt.x, evt.y)
class OnGroupSearch(ListHandler):
    def __init__(self, match_area_group):
        ListHandler.__init__(self)
        # used in ListHandler.get_group_matcher() to see if this OnGroupSearch handler can be reused
        self.match_area_group = match_area_group

    def matches(self, evt, context):
        if 'group' not in context:
            context['group'] = self.click_area.get_object_group_at(evt.x, evt.y, self.match_area_group)
        
        return True
    
    
    def on_group(self, match_area_group):
        return self.add_handler(OnGroup(match_area_group))
    

    

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


"""
Front end for event handler
Register events using EventHandler

def left_click_callback(event):
    pass
    
def dbl_click_callback(event):
    pass


event_handler = EventHandler(click_area)
event_handler.click_handler(Btn.LEFT).do(left_click_callback)
event_handler.double_click_handler(Btn.LEFT).do(dbl_click_callback)

event_handler.connect()
"""

class EventHandler:
    PRESS = 0
    RELEASE = 1
    MOTION = 2

    def __init__(self, click_area: click_area.ClickArea):
        self.click_area = click_area
        self.connect_signals = [
            ('button-press-event', self.on_press),
            ('button-release-event', self.on_release),
            ('motion-notify-event', self.on_motion),
        ]

        self.handlers = { }
        self.signals = { }


    def use_handler(self, handler_class, handler_group, button = None):
        """
        There up a single list of event matchers for on_motion

        There's 2 handler types for button press (single & double) and 1 type for button release
        Each of those 3 type of handlers has 0,1,2 or 3 lists of event matchers (for left/right/middle)

        So check if the requested button handlers has already been made or if a new one is needed
        by checking the signal group type, then the handler type then which button

        @param: handler_class: ClickHandler | DoubleClickHandler | ReleaseHandler | MotionHandler
        @param: handler_group: EventHandler.PRESS | EventHandler.RELEASE | EventHandler.MOTION
        @param: button: Btn.LEFT | Btn.RIGHT | Btn.MIDDLE | None

        """
        if handler_group not in self.handlers:
            self.handlers[handler_group] = []

        for handler in self.handlers[handler_group]:
            if not button:
                return handler
            if handler.__class__ == handler_class and handler.button == button:
                return handler

        handler = handler_class(button)
        handler.click_area = self.click_area
        self.handlers[handler_group].append(handler)

        return handler


    def click_handler(self, btn: Btn) -> ClickHandler:
        """
        Add event processing functions for clicks
        @param btn: Btn.Left/Btn.RIGHT/Btn.MIDDLE
        """
        return self.use_handler(ClickHandler, EventHandler.PRESS, btn)


    def double_click_handler(self, btn: Btn) -> DoubleClickHandler:
        """
        Add event processing functions for double clicks
        @param btn: Btn.Left/Btn.RIGHT/Btn.MIDDLE
        """
        return self.use_handler(DoubleClickHandler, EventHandler.PRESS, btn)
    

    def release_handler(self, btn: Btn) -> ReleaseHandler:
        """
        Add event processing functions for release
        @param btn: Btn.Left/Btn.RIGHT/Btn.MIDDLE
        """
        return self.use_handler(ReleaseHandler, EventHandler.RELEASE, btn)
    

    def motion_handler(self) -> MotionHandler:
        """
        Motion event functions
        """
        return self.use_handler(MotionHandler, EventHandler.MOTION)


    def connect(self, widget):
        """
        Only call this after all the event processing callbacks have been added
        """
        for handler_group in self.handlers.keys():
            signal_name, func = self.connect_signals[handler_group]
            id = widget.connect(signal_name, func)
            self.signals[handler_group] = id


    def disconnect(self, widget):
        """
        Disconnect, can reconnect later"""
        for signal in self.signals.values():
            widget.disconnect(signal)


    def on_press(self, widget, event):
        """ internal event listener """
        return run_handlers(self.handlers[EventHandler.PRESS], event, {'self': widget})


    def on_release(self, widget, event):
        """ internal event listener """
        return run_handlers(self.handlers[EventHandler.RELEASE], event, {'self': widget})


    def on_motion(self, widget, event):
        """ internal event listener """
        return run_handlers(self.handlers[EventHandler.MOTION], event, {'self': widget})



__all__ = [
    'EventHandler',
    'Btn',
]