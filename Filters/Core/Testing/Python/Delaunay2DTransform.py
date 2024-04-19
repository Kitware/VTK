#!/usr/bin/env python
import math
from vtkmodules.vtkCommonCore import (
    vtkFloatArray,
    vtkMath,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkCommonTransforms import vtkTransform
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
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create some points on a sphere such that the data is not in the form
# of z = f(x,y)
#
math1 = vtkMath()
points = vtkPoints()
vectors = vtkFloatArray()
vectors.SetNumberOfComponents(3)
i = 0
while i < 100:
    theta = math1.Random(0.31415, 2.8)
    phi = math1.Random(0.31415, 2.8)
    points.InsertPoint(i, math.cos(theta) * math.sin(phi),
      math.sin(theta) * math.sin(phi), math.cos(phi))
    vectors.InsertTuple3(i, math.cos(theta) * math.sin(phi),
      math.sin(theta) * math.sin(phi), math.cos(phi))
    i = i + 1

profile = vtkPolyData()
profile.SetPoints(points)
profile.GetPointData().SetVectors(vectors)

# build a transform that rotates this data into z = f(x,y)
#
transform = vtkTransform()
transform.RotateX(90)

# triangulate the data using the specified transform
#
del1 = vtkDelaunay2D()
del1.SetInputData(profile)
del1.SetTransform(transform)
del1.BoundingTriangulationOff()
del1.SetTolerance(0.001)
del1.SetAlpha(0.0)

shrink = vtkShrinkPolyData()
shrink.SetInputConnection(del1.GetOutputPort())

map = vtkPolyDataMapper()
map.SetInputConnection(shrink.GetOutputPort())

triangulation = vtkActor()
triangulation.SetMapper(map)
triangulation.GetProperty().SetColor(1, 0, 0)
triangulation.GetProperty().BackfaceCullingOn()

# Add the actors to the renderer, set the background and size
#
ren1.AddActor(triangulation)
ren1.SetBackground(1, 1, 1)

renWin.SetSize(300, 300)
renWin.Render()

cam1 = ren1.GetActiveCamera()
cam1.Zoom(1.5)
cam1.Azimuth(90)
cam1.Elevation(30)
cam1.Azimuth(-60)

# render the image
#
renWin.Render()

#iren.Start()
