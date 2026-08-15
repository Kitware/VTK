#!/usr/bin/env python
"""A volume, and a surface framing it in the same view.

show() builds a surface representation unless told otherwise; "volume" asks for
the other kind.  A volume colors through transfer functions of its own, which it
generates from the data, so there is something to see before anything is
configured.
"""

from vtkmodules.vtkImagingCore import vtkRTAnalyticSource
from vtkmodules.vtkViewsScivis import vtkScivisView

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401
import vtkmodules.vtkRenderingVolumeOpenGL2  # noqa: F401

view = vtkScivisView(window_title="Volume", size=(1000, 700))

wavelet = vtkRTAnalyticSource(whole_extent=(-20, 20, -20, 20, -20, 20))
volume = view.show(wavelet, "volume", scalar_opacity_unit_distance=1.5)

# How it responds to light is on the representation; the transfer functions and
# anything finer are on the objects it hands out.
volume.shade = True
volume.ambient = 0.3
volume.diffuse = 0.7
volume.volume_property.interpolation_type = "linear"

# The same data as an outline, to give the volume a frame of reference.
view.show(wavelet, representation="outline", color="white")

view.ResetCamera()
view.Start()
