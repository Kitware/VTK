#!/usr/bin/env python
"""Coloring by an array, and the scalar bar that describes it.

A scalar bar belongs to the scene rather than to any one representation: both
surfaces below draw "Elevation", so there is one bar spanning both of their
ranges rather than two disagreeing about the same quantity.
"""

from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkFiltersCore import vtkElevationFilter
from vtkmodules.vtkFiltersSources import vtkSphereSource
from vtkmodules.vtkViewsScivis import vtkScivisView

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401

view = vtkScivisView(window_title="Coloring", size=(1000, 700))

# Two spheres of different sizes, measured against the same scale, so the small
# one covers only the middle of the range the big one covers all of.
for center, radius in [((-3, 0, 0), 1.0), ((3, 0, 0), 2.5)]:
    sphere = vtkSphereSource(center=center, radius=radius,
                             theta_resolution=48, phi_resolution=48)
    elevation = vtkElevationFilter(low_point=(0, -2.5, 0), high_point=(0, 2.5, 0))
    representation = view.show(sphere >> elevation)
    representation.ColorByPointArray("Elevation")

    elevations = (sphere >> elevation)().point_data["Elevation"]
    print(f"  sphere r={radius}: Elevation over {tuple(round(v, 2) for v in elevations.range)}")

# The color map an array is drawn through comes from the view's manager, which
# is a mapping of array name to map.  Register one to choose it; setting a map
# on a representation does not stick, because the view offers the shared one
# again on every render.
table = vtkLookupTable(number_of_table_values=16, hue_range=(0.667, 0.0))
table.Build()
view.lookup_table_manager["Elevation"] = table

# How the bars are drawn belongs to the set, so it reaches bars made later too.
view.scalar_bars.label_font_size = 14
view.scalar_bars.title_font_size = 18
view.scalar_bars.draggable = True

view.ResetCamera()
view.Render()

print(f"{len(view.scalar_bars)} bar(s):")
for bar in view.scalar_bars:
    low, high = bar.lookup_table.range
    print(f"  {bar.title}: [{low:.2f}, {high:.2f}] spanning both spheres")

view.Start()
