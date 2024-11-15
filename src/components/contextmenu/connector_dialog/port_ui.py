import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

import zzub

# a toggle switch which display port name and changes colour when clicked
# can't use a gtk.label but they inherit background color from enclosing widget
class BoxyLabel(Gtk.EventBox):
    def __init__(self, text):
        Gtk.EventBox.__init__(self, expand=True)
        self.label = Gtk.Label(text)
        self.add(self.label)


    def set_selected_value(self, value):
        if value:
            self.get_style_context().add_class("selected")
        else:
            self.get_style_context().remove_class("selected")





# used to populate a grid with two columns and very variable number of rows
# it uses/updates a row_counts property on the grid to
# the inital row_offset is read from the grid.row_counts property
# is_target is used as the column index
# the build_rows function adds rows - starting from row_offset - and returns the number of new rows added
class AbstractPortGroup:
    def __init__(self, grid, is_target, port_type, name, *args):
        self.row_offset = grid.row_counts[is_target]
        self.full_css_name = name.replace(" ", "_")          # eg 'audio_in', 'midi_out'
        self.short_css_name = name[0:name.find(" ")]         # eg 'audio', 'midi'

        self.port_type = port_type
        self.is_target = is_target
        self.selected = 0
        self.handler = None

        self.row_count = self.build_rows(grid, self.is_target, self.row_offset, *args)

        grid.row_counts[is_target] = self.row_offset + self.row_count


    def build_rows(self, grid, column_id, first_row, *args):
        return 0


    def set_handler(self, handler):
        self.handler = handler


    def add_base_css(self, widget):
        context = widget.get_style_context()
        context.add_class(self.full_css_name)
        context.add_class(self.short_css_name)


    # the way the audio port group works is sensitive. do not reorder this function
    # the toggle_port function of audio group failed when:
    # an audio chan was selected, then a param channel, then the *same* audio channel reselected
    # as self.selected was toggled to 0 instead of 1 - audios channel had to be re-selected twice 

    # so set self.selected to 0 in audiogroup.clear() - called by the handler, dialog.item_selected -
    # then set self.selected to value after the handler call
    def set_selected_value(self, value):
        if self.handler:
            self.handler(self.port_type, self.is_target, value)

        self.selected = value   


    def clear(self, value):
        pass


    def highlight(self, value):
        pass




class DummyGroup():
    def __init__(self, is_target, port_type):
        self.port_type = port_type
        self.is_target = is_target


    def set_handler(self, handler):
        pass


    def clear(self, value):
        pass


    def highlight(self, value):
        pass


    def set_selected_value(self, value):
        pass





class TypedPorts(AbstractPortGroup):
    def __init__(self, grid, is_target, css_name, ports, port_type):
        AbstractPortGroup.__init__(self, grid, is_target, port_type, css_name, ports)


    def build_rows(self, grid, column_id, first_row, ports):
        self.labels = []

        for index, port in enumerate(ports):
            label = BoxyLabel(port.get_name())
            label.connect("button-release-event", self.toggle_port, index)
            grid.attach(label, column_id, first_row + index, 1, 1)
            self.add_base_css(label)
            self.labels.append(label)

        return len(ports)


    def toggle_port(self, widget, event, index):
        self.set_selected_value(index)


    def clear(self, value):
        if value < len(self.labels):
            self.labels[value].get_style_context().remove_class("selected")


    def highlight(self, value):
        if value < len(self.labels):
            self.labels[value].get_style_context().add_class("selected")





# int used as a bitflag of which channels are selected
class AudioPorts(AbstractPortGroup):
    def __init__(self, grid, is_target, name, port_count):
        AbstractPortGroup.__init__(self, grid, is_target, zzub.zzub_port_type_audio, name, port_count)


    def build_rows(self, grid, column_id, first_row, port_count):
        if port_count == 1:
            channels = self.build_channel_list(["mono"])
        elif port_count == 2:
            channels = self.build_channel_list(["L", "R"])
        else:
            channels = self.build_channel_list(range(max(port_count, 4)))

        self.channels = channels
        grid.attach(channels, column_id, first_row, 1, 1)

        return 1


    # return number of columns created
    def build_channel_list(self, titles) -> Gtk.Box:
        hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)

        for index, title in enumerate(titles):
            label = BoxyLabel(title)
            label.connect("button-release-event", self.toggle_channel, index)
            hbox.pack_start(label, True, True, 0)
            self.add_base_css(label)

        return hbox


    # the "L" or "R" button was clicked. toggle: channel on <-> channel off
    def toggle_channel(self, widget, event, channel):
        self.set_selected_value(self.selected ^ (1 << channel))


    def clear(self, value):
        self.selected = 0
        for label in self.channels.get_children():
            label.get_style_context().remove_class("selected")


    def highlight(self, value):
        for index, label in enumerate(self.channels.get_children()):
            if value & (1 << index):
                label.get_style_context().add_class("selected")
