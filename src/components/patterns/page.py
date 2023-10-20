# This Python file uses the following encoding: utf-8

# if __name__ == "__main__":
#     pass
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

class NeilNotebookPage(Gtk.VBox):
    _notebook = None # notebook and index assigned in the notebook framepanel in mainwindow.py
    _index = None
    def is_current_page(self):
        return self._notebook and self._notebook.get_current_page() == self._index
