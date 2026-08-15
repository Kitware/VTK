#!/usr/bin/env python
"""Things a view shows that have no data behind them.

A text overlay and a set of grid axes are representations like any other: they
are added, hidden and removed the same way.  Neither has an input, which is what
vtkScivisRepresentation exists for -- being showable costs two methods, so an
annotation does not have to carry a pipeline, a color map and a selection it
would never use.
"""

from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkViewsScivis import (
    vtkGridAxesRepresentation,
    vtkScivisView,
    vtkTextOverlayRepresentation,
)

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401

view = vtkScivisView(window_title="Annotations", size=(1000, 700))
sphere = vtkSphereSource(radius=3.0, theta_resolution=48, phi_resolution=48)
view.show(sphere, color="steel_blue", specular=0.3)

# A title, in display coordinates from the lower left of the window.
view += vtkTextOverlayRepresentation(
    text="Sphere, r = 3", position=(20, 20), font_size=24, color="white")

# Axes that follow the scene.  There are no bounds to set: the view says when
# the extent of what it is drawing changes and these resize themselves, which is
# why growing the sphere below moves them without anything being told twice.
axes = vtkGridAxesRepresentation(
    padding=0.05, x_title="x (m)", y_title="y (m)", z_title="z (m)", label_font_size=14)
view += axes

view.ResetCamera()
view.Render()
print("axes around", [round(b, 2) for b in axes.grid_axes_actor.grid_bounds])

sphere.radius = 6.0
view.Render()
print("after the sphere grows:", [round(b, 2) for b in axes.grid_axes_actor.grid_bounds])

view.ResetCamera()
view.Start()
