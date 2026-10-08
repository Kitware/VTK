#!/usr/bin/env python
"""Getting a scene out of a view, as a picture or as geometry.

Both are the exporter, reached as view.exporter.  How a capture comes out --
how large, and whether its background is transparent -- is a setting on it
rather than an argument repeated at every call.
"""

import os
import tempfile

from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkViewsScivis import vtkScivisView

import vtkmodules.vtkRenderingOpenGL2  # noqa: F401

view = vtkScivisView(window_title="Export", size=(800, 600),
                     exporter={"magnification": 2})
view.show(vtkConeSource(resolution=48), color="tomato", specular=0.4)
view.ResetCamera()

directory = tempfile.mkdtemp()

# The format comes from the extension, so choosing one is choosing a name.
picture = os.path.join(directory, "cone.png")
view.exporter.SaveScreenshot(picture)
print(f"wrote {picture} ({os.path.getsize(picture)} bytes, twice the window size)")

scene = os.path.join(directory, "cone.gltf")
view.exporter.ExportScene(scene)
print(f"wrote {scene} ({os.path.getsize(scene)} bytes)")

# A transparent capture turns the background off for as long as it takes to read
# the window, and puts it back afterwards.
view.exporter.transparent_background = True
image = view.exporter.CaptureImage()
print("captured", image.dimensions[:2], "with", image.number_of_scalar_components, "components")

view.Start()
