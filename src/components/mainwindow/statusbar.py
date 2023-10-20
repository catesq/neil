import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk

from neil.common import MARGIN0

## the statusbar is a grid with three columns and two rows
# the central column has the play/pause/... buttons on the top row and the bpm/play position on the bottom row
#the left and right column are 


class StatusBar(Gtk.Grid):
    __neil__ = dict(
        id = 'neil.core.statusbar',
        singleton = True,
        categories = [  ],
    )

    class Sizes:
        def __init__(self, min_left=150, min_center=250, min_right=150):
            self.min_left = min_left
            self.min_center = min_center
            self.min_right = min_right

    def __init__(self, sizes = Sizes()):
        Gtk.Grid.__init__(self)
        self.sizes = sizes

        self.attach(Gtk.HBox(), 0, 0, 1, 1)
        self.attach(Gtk.HBox(), 0, 1, 1, 1)
        self.attach(Gtk.HBox(), 1, 0, 2, 2)
        self.attach(Gtk.HBox(), 3, 0, 2, 2)
        self.attach(Gtk.HBox(), 5, 0, 1, 1)
        self.attach(Gtk.HBox(), 5, 1, 1, 1)

        self.set_row_homogeneous(True)
        self.set_column_homogeneous(True)

        for x in range(3):
            for y in range(2):
                ## pad the child at grid pos x,y
                cell = self.get_child_at(x, y)

        ## expand and centralize middle column
        self.get_child_at(1, 0).set_hexpand(True)
        self.get_child_at(1, 0).set_halign(Gtk.Align.CENTER)
        self.get_child_at(3, 0).set_halign(Gtk.Align.CENTER)

        self.set_column_spacing(MARGIN0)
        self.set_row_spacing(MARGIN0)


    def set_cell(self, widget, x, y, expand, min_width = 0):
        if not widget:
            return
            
        cell = self.clear_cell_at(x, y)
        cell.pack_start(widget, expand, expand, 0)

        # if expand:
            # cell.set_hexpand(True)

        widget.show_all()
        self.show_all()
            # if min_width > 0:
                # cell.set_size_request(min_width, -1)
        

    def set_left(self, widget_upper = None, widget_lower = None):
        self.set_cell(widget_upper, 0, 0, True, self.sizes.min_left)
        self.set_cell(widget_lower, 0, 1, True, self.sizes.min_left)
        
    def clear_cell_at(self, x, y) -> Gtk.HBox:
        cell = self.get_child_at(x, y)
        cell.foreach(lambda widget: cell.remove(widget))
        return cell
    
    def clear_left(self):
        self.clear_cell_at(0, 0)

    def clear_right(self):
        self.clear_cell_at(5, 0)

    def clear_both_sides(self):
        self.clear_left()
        self.clear_right()

    def set_play_controls(self, widget):
        self.set_cell(widget, 3, 0, True, self.sizes.min_center)


    def set_play_info(self, widget):
        self.set_cell(widget, 1, 0, True, self.sizes.min_center)


    def set_right(self, widget_upper = None, widget_lower = None):
        self.set_cell(widget_upper, 5, 0, True, self.sizes.min_right)
        self.set_cell(widget_lower, 5, 1, True, self.sizes.min_right)



__neil__ = dict(
    classes = [
        StatusBar,
    ],
)

