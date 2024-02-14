#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkMath,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkFiltersCore import vtkDelaunay2D
from vtkmodules.vtkFiltersGeneral import vtkShrinkPolyData
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer(background=[1,1,1])
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create some points
#
math = vtkMath()
points = vtkPoints()
i = 0
while i < 1000:
    points.InsertPoint(i,math.Random(0,1),math.Random(0,1),0.0)
    i = i + 1

profile = vtkPolyData()
profile.SetPoints(points)
# triangulate them
#
del1 = vtkDelaunay2D(bounding_triangulation=True, alpha=0.0, tolerance=0.001)
shrink = vtkShrinkPolyData()
map = vtkPolyDataMapper()
profile >> del1 >> shrink >> map
triangulation = vtkActor(mapper=map)
triangulation.GetProperty().color = [1,0,0]
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(triangulation)
renWin.SetSize(500,500)
renWin.Render()
cam1 = ren1.GetActiveCamera()
cam1.Zoom(1.5)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
