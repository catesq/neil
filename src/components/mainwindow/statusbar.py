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
        self.set_margin_left(48)

        self.detail_col = 3
        self.bpm_col = 0
        self.controls_col = 1
        self.status_col = 5

        self.sizes = sizes

        self.attach(Gtk.HBox(), self.detail_col, 0, 2, 1)
        self.attach(Gtk.HBox(), self.detail_col, 1, 2, 1)
        self.attach(Gtk.HBox(), self.bpm_col, 0, 1, 2)
        self.attach(Gtk.HBox(), self.controls_col, 0, 2, 2)
        self.attach(Gtk.HBox(), self.status_col, 0, 1, 1)
        self.attach(Gtk.HBox(), self.status_col, 1, 1, 1)

        self.set_row_homogeneous(True)
        self.set_column_homogeneous(True)

        ## expand and centralize middle column
        self.set_hexpand(True)
        self.get_child_at(self.controls_col, 0).set_hexpand(True)
        self.get_child_at(self.bpm_col, 0).set_halign(Gtk.Align.START)
        self.get_child_at(self.controls_col, 0).set_halign(Gtk.Align.CENTER)
        
        self.get_child_at(self.detail_col, 0).set_halign(Gtk.Align.START)
        self.get_child_at(self.status_col, 0).set_halign(Gtk.Align.START)
        self.get_child_at(self.detail_col, 1).set_halign(Gtk.Align.START)
        self.get_child_at(self.status_col, 1).set_halign(Gtk.Align.START)

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
        

    def set_left(self, widget_upper = None, widget_lower = None):
        self.set_cell(widget_upper, self.detail_col, 0, False, self.sizes.min_left)
        self.set_cell(widget_lower, self.detail_col, 1, False, self.sizes.min_left)
        
    def clear_cell_at(self, x, y) -> Gtk.HBox:
        cell = self.get_child_at(x, y)
        cell.foreach(lambda widget: cell.remove(widget))
        return cell
    
    def clear_left(self):
        self.clear_cell_at(self.detail_col, 0)

    def clear_right(self):
        self.clear_cell_at(self.status_col, 0)

    def clear_both_sides(self):
        self.clear_left()
        self.clear_right()

    def set_play_controls(self, widget):
        self.set_cell(widget, self.controls_col, 0, True, self.sizes.min_center)

    def set_play_info(self, widget):
        self.set_cell(widget, self.bpm_col, 0, False, self.sizes.min_center)


    def set_right(self, widget_upper = None, widget_lower = None):
        self.set_cell(widget_upper, self.status_col, 0, False, self.sizes.min_right)
        self.set_cell(widget_lower, self.status_col, 1, False, self.sizes.min_right)



__neil__ = dict(
    classes = [
        StatusBar,
    ],
)

