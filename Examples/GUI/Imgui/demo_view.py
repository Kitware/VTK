# /// script
# requires-python = ">=3.10"
# dependencies = [
#     # VTK::ViewsScivis is not part of a released wheel yet.
#     "vtk>=9.7",
# ]
# ///
"""Demo of vtkStandardRenderView and vtkSurfaceRepresentation."""

from vtkmodules.vtkFiltersSources import vtkConeSource, vtkSphereSource, vtkCylinderSource
from vtkmodules.vtkViewsScivis import vtkStandardRenderView

# Registers the OpenGL implementations of the rendering classes.  Without it the
# view builds a base vtkRenderWindow and nothing is drawn.
import vtkmodules.vtkRenderingOpenGL2  # noqa: F401

# Create a view
view = vtkStandardRenderView(window_title="StandardRenderView Demo")
view.size = (1200, 800)

# show() creates a surface representation for the source, applies the given
# properties, adds it to the view, and returns it.  Colors accept snake_case
# names from vtkmodules.util.colors or an (r, g, b) tuple.
view.show(vtkSphereSource(center=(-2, 0, 0), theta_resolution=32, phi_resolution=32),
          color="steel_blue", specular=0.4, specular_power=30)

view.show(vtkConeSource(resolution=32),
          color="tomato", representation="surfacewithedges", edge_color="black")

view.show(vtkCylinderSource(center=(2, 0, 0), resolution=24),
          color="sea_green", opacity=0.6, representation="wireframe", line_width=2)

# Enable the light kit for professional lighting
view.use_light_kit = True
view.key_light_intensity = 0.8
view.key_light_warmth = 0.6
view.fill_light_warmth = 0.4
view.key_to_fill_ratio = 3.0
view.key_light_angle = (50, 10)

# Start interactive rendering
view.Start()
