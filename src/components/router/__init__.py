from .attributes_dialog import AttributesDialog
from .parameter_dialog import ParameterDialog, ParameterDialogManager
from .preset_dialog import PresetDialog, PresetDialogManager
from .route_panel import RoutePanel
from .volume_slider import VolumeSlider
from .route_view import RouteView

__all__ = [
    'ParameterDialog',
    'ParameterDialogManager',
    'PresetDialog',
    'PresetDialogManager',
    'AttributesDialog',
    'RoutePanel',
    'VolumeSlider',
    'RouteView',
]

__neil__ = dict(
    classes = [
        ParameterDialog,
        ParameterDialogManager,
        PresetDialogManager,
        AttributesDialog,
        RoutePanel,
        RouteView,
    ],
)