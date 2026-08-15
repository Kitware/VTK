#!/usr/bin/env python
"""The smallest thing worth drawing: a surface, colored, lit and framed.

A view is a renderer, a render window, an interactor and the wiring between
them, already built.  Properties can be given to the constructor or assigned
afterwards, and colors accept a name from vtkmodules.util.colors.
"""

from vtkmodules.vtkFiltersSources import vtkConeSource, vtkCylinderSource, vtkSphereSource
from vtkmodules.vtkViewsScivis import vtkScivisView

# Registers the OpenGL implementations.  Without it the view builds a base
# vtkRenderWindow and nothing is drawn.
import vtkmodules.vtkRenderingOpenGL2  # noqa: F401

view = vtkScivisView(window_title="Surfaces", size=(1000, 700), use_light_kit=True)

# show() makes a representation for the source, applies the properties, adds it
# to the view, and hands it back.
view.show(vtkSphereSource(center=(-2, 0, 0), theta_resolution=32, phi_resolution=32),
          color="steel_blue", specular=0.4, specular_power=30)

view.show(vtkConeSource(resolution=32),
          color="tomato", representation="surfacewithedges", edge_color="black")

cylinder = view.show(vtkCylinderSource(center=(2, 0, 0), resolution=24),
                     color="sea_green", representation="wireframe", line_width=2)

# Anything the view does not carry itself belongs to the object that owns it,
# which it hands out rather than mirrors.
view.light_kit.key_light_intensity = 0.8
view.renderer.SetTwoSidedLighting(True)

# The camera can be put on a standard direction, or reached for anything finer.
view.ViewIsometric()
print(f"{len(view)} representations, camera at {tuple(round(c, 2) for c in view.camera.position)}")

view.Start()
