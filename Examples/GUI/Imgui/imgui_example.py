from vtk_viewer import VtkViewer

from vtkmodules.vtkFiltersSources import vtkConeSource
from vtkmodules.vtkRenderingCore import vtkActor, vtkPolyDataMapper

viewer = VtkViewer()

# Pythonic VTK: connect the pipeline with >> and pass the mapper as a kwarg.
mapper = vtkPolyDataMapper()
vtkConeSource() >> mapper
actor = vtkActor(mapper=mapper)

viewer.renderer.AddActor(actor)

# Start the app! (Uses default fullscreen layout)
viewer.run(title="Simple Cone Viewer")
