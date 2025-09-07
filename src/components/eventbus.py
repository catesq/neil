"""
Supplies all user interface modules with a common bus
to handle status updates.

this is the order of operation:

1. on first time loading, GlobalEventBus registers all possible event types (add new ones below).
2. main window instantiates all other ui objects
3. no other ui object may register any new event id.
4. upon creation, all ui objects listening to events must connect their handlers using
   eventbus.<event name> += <function>[,<arg1>[,...]]
5. call events using
   eventbus.<event name>([<arg1>[,...]])

please note:

* event handling is slow and shouldn't be used for things
        that need to be called several times per second.
* event handler calls should always have the declared number of parameters, with the
        specified types.
* for clarity, don't register functions of another object in your object. always have
        objects connect their own methods.
"""

import sys, weakref
from typing import Any, Callable

EVENTS = [
    # these might become obsolete
    'ping', # (int, ... ). all connected objects should print out a random message.
    'finalize', # (hub, ... ). all module are loaded and available, finalize any cross-dependencies.
    'shutdown-request', # ( ... ). asks for quit. it is possible that quitting might not happen.
    'shutdown', # ( ... ). receiver should deinitialize and prepare for application exit.
    'song-opened', # ( ... ). called when the entire song changes in such a way that all connected objects should update.
    'show-plugin', # (plugin, ...) called when a plugin should be visualized overall.

    # most events serve for notification purposes, these are requests,
    # to be sent by a view to switch the workspace appearance.
    'edit_pattern_request', # (plugin, index, ...) called from e.g. the sequencer when editing a pattern is requested.
    'edit_sequence_request', # (plugin, index, time) called from e.g. the pattern editor, when editing a sequence is requested.

    # document ui events, translation is done in player.py
    # these events shall never be called directly from the application,
    # they are sent as notifications by neil.core.player.
    'octave_changed', # (octave, ...) called when player.octave changes.
    'active_plugins_changed', # ([plugin, ...], ...) called when player.active_plugins changes.
    'active_patterns_changed', # ([(plugin, index), ...], ...) called when player.active_patterns changes.
    'active_waves_changed', # ([wave, ...], ...) called when player.active_waves changes.
    'autoconnect_target_changed', # (plugin, ...) called when player.autoconnect_target changes.
    'sequence_step_changed', # (stepsize, ...) called when player.sequence_step changes.
    'plugin_origin_changed', # ((x,y), ...) called when player.plugin_origin changes.
    'solo_plugin_changed', # (plugin, ...) called when player.solo_plugin changes.
    'document_path_changed', # (path, ...) called when player.document_path changes.
    'document_loaded', # (...) called when load_* is called.

    # libzzub events, translation is done in player.py
    # note that these events shall never be called from the application
    # directly, instead they will only be triggered by neil.core.player.
    'zzub_all', # ( data,... )
    'zzub_connect', # ( from_plugin,to_plugin,type,... )
    'zzub_custom', # ( id,data,... )
    'zzub_delete_pattern', # ( plugin,index,... )
    'zzub_delete_plugin', # ( plugin,... )
    'zzub_delete_wave', # ( wave,... )
    'zzub_disconnect', # ( from_plugin,to_plugin,type,... )
    'zzub_double_click', # ( ... )
    'zzub_edit_pattern', # ( plugin,index,group,track,column,row,value,... )
    'zzub_envelope_changed', # ( ... )
    'zzub_load_progress', # ( ... )
    'zzub_midi_control', # ( status,data1,data2,... )
    'zzub_new_pattern', # ( plugin,index,... )
    'zzub_new_plugin', # ( plugin,... )
    'zzub_osc_message', # ( path,types,argv,argc,msg,... )
    'zzub_parameter_changed', # ( plugin,group,track,param,value,... )
    'zzub_pattern_changed', # ( plugin,index,... )
    'zzub_pattern_insert_rows', # ( plugin,index,row,rows,column_indices,indices,... )
    'zzub_pattern_remove_rows', # ( plugin,index,row,rows,column_indices,indices,... )
    'zzub_player_state_changed', # ( player_state,... )
    'zzub_plugin_changed', # ( plugin,... )
    'zzub_post_connect', # ( from_plugin,to_plugin,type,... )
    'zzub_post_set_tracks', # ( plugin,... )
    'zzub_pre_connect', # ( from_plugin,to_plugin,type,... )
    'zzub_pre_delete_pattern', # ( plugin,index,... )
    'zzub_pre_delete_plugin', # ( plugin,... )
    'zzub_pre_disconnect', # ( from_plugin,to_plugin,type,... )
    'zzub_pre_set_tracks', # ( plugin,... )
    'zzub_sequencer_add_track', # ( plugin,... )
    'zzub_sequencer_changed', # ( plugin,track,time,... )
    'zzub_sequencer_remove_track', # ( plugin,... ) ## FIXME has been renamed in zzub as sequence.destroy
    'zzub_set_sequence_event', # ( plugin,track,time,... )
    'zzub_set_sequence_tracks', # ( plugin,... )
    'zzub_set_tracks', # ( plugin,... )
    'zzub_slices_changed', # ( ... )
    'zzub_vu', # ( size,left_amp,right_amp,time,... )
    'zzub_wave_allocated', # ( wavelevel,... )
    'zzub_wave_changed', # ( wave,... )
]




class EventHandlerList:
    def __init__(self, name, handlers=None):
        self.debug_events = False
        self.name = name

        if not handlers:
            handlers = []
                
        self.handlers = handlers


    def clear(self):
        """
        Remove all handlers
        """
        self.handlers = []


    # funcargs is either:
    #   Any callable function
    #   A list with 1 item - a callable 
    #   A list with more than 1 item. The first item is a callable, the other items are extra arguments passed to that function
    def define_handler(self, funcargs):
        """
        Used by __add__. Builds a callback which is added to the handler list
        @param funcargs: callable | list[callable, Any, ...]  

        @return: tuple(weakreaf.ref, str, list) 
                 The weakreaf is to a callable or to an object which has a funcname
                 The str is optional - funcname found by make_ref_funcname
                 The list is optional - arguments passed to the callable
        """
        # print(self.name, "define handler", funcargs)
        func, args = self.make_func_def_args(funcargs)
        ref, funcname = self.make_ref_funcname(func)
        # print(self.name, "defined", ref, funcname, args)
        return ref, funcname, args
    
    
    
    def make_func_def_args(self, funcargs):
        """
        Used by self.define_handler to get the event handling function and it's arguments

        @param funcargs: callable | list[callable, Any, ...]    
                         Either a callable, or a list with callable/optional arguments

        @return: callable | list[Any]     
                 Callback function with optional extra arguments passed to callback 
        """
        func = None
        args = ()

        if isinstance(funcargs, (list, tuple)):
            if len(funcargs) >= 2:
                func, args = funcargs[0], funcargs[1:]
            elif len(funcargs) == 1:
                func = funcargs[0]
            else:
                func = None
        else:
            func = funcargs
        
        assert callable(func), "object %r must be callable." % func

        return func, args
    

    
    def make_ref_funcname(self, func):
        """
        Use weakref on the either a callable function or the __self__ object from an instance method 
        so the function/object can be garbage collected.

        Return (ref(callable), None) when callable was lambda/top level function 
        Return (ref(instance), str(method_name)) when callable was instance method
        
        @param func: callable
        @return: tuple[ref, Optional[str]]
        """
        ref = None
        funcname = None
        
        if hasattr(func, '__self__'):
            ref = weakref.ref(func.__self__)
            funcname = func.__name__
        else:
            ref = weakref.ref(func)

        return ref, funcname
    



    def __add__(self, funcargs):
        ref, funcname, args = self.define_handler(funcargs)

        self.handlers.append((ref,funcname,args))

        return self


    def __sub__(self, funcargs):
        rm_ref, rm_funcname, rm_args = self.define_handler(funcargs)

        self.handlers = [(ref,funcname,args) for ref, funcname, args in self.handlers if ref != rm_ref or funcname != rm_funcname or args != rm_args]

        return self



    def __len__(self):
        return len(self.handlers)


    def filter_dead_references(self):
        """
        filter handlers from dead references
        """
        self.handlers = [(ref,funcname,args) for ref,funcname,args in self.handlers if ref()]


    def __call__(self, *cargs):
        result = None
        deadrefs = False

        for ref, funcname, args in self:
            deref = ref()

            if not deref:
                deadrefs = True
                continue

            if funcname:
                func = getattr(deref, funcname)
            else:
                func = deref
                
            try:
                fargs = cargs + args
                result = func(*fargs) or result
            except:
                sys.excepthook(*sys.exc_info())
        
        if deadrefs:
            self.filter_dead_references()

        return result


    def __iter__(self):
        return iter(self.handlers)


    def print_mapping(self):
        self.filter_dead_references()
        if self.debug_events:
            print(("event [%s]" % self.name), end='\n')
        for ref,funcname,args in self.handlers:
            if funcname:
                func = getattr(ref(), funcname)
            else:
                func = ref()
            funcname = func.__name__
            if hasattr(func, '__self__'):
                funcname = func.__self__.__class__.__name__ + '.' + funcname
            if self.debug_events:
                print((" => %s(%s)" % (funcname,','.join([repr(x) for x in args]))), end='\n')


class EventBus:
    __readonly__ = False


    def __init__(self):
        self.handlers = []
        for name in self.names:  # type: ignore
            attrname = name.replace('-','_')
            self.handlers.append(attrname)
            setattr(self, attrname, EventHandlerList(name))
        self.__readonly__ = True


    def __setattr__(self, name, value):
        if name.startswith('__'):
            self.__dict__[name] = value
            return
        assert name in self.__dict__ or not self.__readonly__, "can't set attribute when object is read only"
        if name in self.__dict__ and isinstance(self.__dict__[name], EventHandlerList) and not isinstance(value, EventHandlerList):
            raise Exception("did you mean +=?")
        
        self.__dict__[name] = value


    def print_mapping(self):
        for idstr in sorted(self.handlers):
            handlerlist = getattr(self, idstr)
            handlerlist.print_mapping()


    def get_handler_list(self, event_name) -> EventHandlerList | None:
        if event_name in self.__dict__:
            return getattr(self, event_name, None)
        else:
            return getattr(self, 'zzub_' + event_name, None)


    # call __add__ in EventhandlerList
    def attach(self, event_name: str | list[str], *funcargs):
        if isinstance(event_name, list):
            for name in event_name:
                self.attach(name, *funcargs)
            return
        
        handler_list = self.get_handler_list(event_name)

        if handler_list is not None:
            handler_list.__add__(funcargs)
        else:
            print("could not attach event '{}' found to any of {}".format(event_name, self.__dict__.keys()))


    # call __sub__ in EventhandlerList
    def detach(self, event_name_or_func: str | Any, *funcargs):
        """
        Either:
            Remove the function handler in funcargs from an named event/list of named events
            Remove all handlers from the named event/list of events
            
        """
        if isinstance(event_name_or_func, str):
            self.detach_event(event_name_or_func, *funcargs)
        elif isinstance(event_name_or_func, list):
            for item in event_name_or_func:
                if isinstance(item, list) or isinstance(item, tuple):
                    self.detach(*item, *funcargs)
                else:
                    self.detach(item, *funcargs)
        elif callable(event_name_or_func):
            self.detach_func(event_name_or_func, *funcargs)
        else:
            print("Unknown: eventbus.detach({}, {})".format(event_name_or_func, funcargs))


    # 
    def detach_event(self, event_name: str, *funcargs):
        handler_list = self.get_handler_list(event_name)

        if handler_list is not None:
            if len(funcargs) == 0:
                handler_list.clear()
            else:
                handler_list.__sub__(funcargs)


    def detach_func(self, *funcargs):
        for event_name in self.handlers:
            handler_list = self.get_handler_list(event_name)

            if handler_list is not None:
                handler_list.__sub__(funcargs)


    def detach_all(self, *funcargs):
        """
        Call detach() for every argument
        Each item may be:
            a string dispatched to self.detach_event containg event name to unbind 
            a callable dispatched to self.detach_func to remove from all event handlers
        """
        for item in funcargs:
            self.detach(item)

    def call(self, event_name: str, *args):
        handler_list = self.get_handler_list(event_name)

        if handler_list is not None:
            handler_list.__call__(*args)



class NeilEventBus(EventBus):
    __neil__ = dict(
        id = 'neil.core.eventbus',
        singleton = True,
    )

    names = EVENTS

__all__ = [

]

__neil__ = dict(
    classes = [
        NeilEventBus,
    ],
)

if __name__ == '__main__':
    class MyHandler:
        def on_bang(self, otherbang, mybang):
            print((self,"BANG! otherbang=",otherbang,"mybang=",mybang))

    def on_bang(otherbang, mybang):
        print(("GLOBAL BANG! otherbang=",otherbang,"mybang=",mybang))

    handler1 = MyHandler()
    handler2 = MyHandler()
    eventbus = NeilEventBus()
    eventbus.ping += handler1.on_bang, 50 #pylint: disable=no-member
    eventbus.ping += handler2.on_bang, 60 #pylint: disable=no-member
    eventbus.ping += on_bang, 70 #pylint: disable=no-member
    eventbus.print_mapping()
    print("* 3 bangs:")
    eventbus.ping(25) #pylint: disable=no-member
    del handler1
    del on_bang
    print("* 1 bang:")
    eventbus.ping(25) #pylint: disable=no-member
