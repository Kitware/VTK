# /// script
# requires-python = ">=3.10"
# dependencies = [
#     # VTK::ViewsRendering is not part of a released wheel yet.
#     "vtk>=9.7",
# ]
# ///
"""Volume rendering with vtkStandardRenderView and vtkVolumeRepresentation.

``show()`` builds a surface representation by default; passing ``"volume"``
builds a vtkVolumeRepresentation for the same input instead.  Both kinds of
representation live in the same view, so the volume below is framed by an
outline of the very same data.

The volume representation generates its color and opacity transfer functions
from the scalar range of the input.  Supply your own with
``SetColorTransferFunction()`` / ``SetScalarOpacity()``, and give them back
with ``ResetColorTransferFunction()`` / ``ResetScalarOpacity()``.
"""

from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.vtkViewsRendering import vtkStandardRenderView

# Registers the OpenGL implementations of the rendering classes, including the
# volume mappers.  Without these the view builds a base vtkRenderWindow and
# nothing is drawn.
import vtkmodules.vtkRenderingOpenGL2  # noqa: F401
import vtkmodules.vtkRenderingVolumeOpenGL2  # noqa: F401

view = vtkStandardRenderView(window_title="Volume Rendering Demo")
view.size = (900, 700)

wavelet = vtkRTAnalyticSource(whole_extent=(-25, 25, -25, 25, -25, 25))

# The volume, with transfer functions generated from the data.
view.show(wavelet, "volume", scalar_opacity_unit_distance=1.5)

# The same data as an outline, to give the volume a frame of reference.
view.show(wavelet, representation="outline", color="white")

view.ResetCamera()
view.Start()
