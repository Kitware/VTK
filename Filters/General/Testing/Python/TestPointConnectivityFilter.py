#!/usr/bin/env python
import math
from vtkmodules.vtkCommonCore import (
    vtkMath,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkFiltersCore import vtkDelaunay2D
from vtkmodules.vtkFiltersGeneral import vtkPointConnectivityFilter
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkCamera,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
import vtkmodules.test.Testing
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

npts = 1000

# Create a pipeline with variable point connectivity
math = vtkMath()
points = vtkPoints()
i = 0
while i < npts:
    points.InsertPoint(i,math.Random(0,1),math.Random(0,1),0.0)
    i = i + 1

profile = vtkPolyData()
profile.SetPoints(points)
# triangulate them
#
del1 = vtkDelaunay2D()
del1.SetInputData(profile)
del1.BoundingTriangulationOff()
del1.SetTolerance(0.001)
del1.SetAlpha(0.0)

conn = vtkPointConnectivityFilter()
conn.SetInputConnection(del1.GetOutputPort())
conn.Update()

mapper = vtkPolyDataMapper()
mapper.SetInputConnection(conn.GetOutputPort())
mapper.SetScalarModeToUsePointFieldData()
mapper.SelectColorArray("Point Connectivity Count")
mapper.SetScalarRange(conn.GetOutput().GetPointData().GetArray("Point Connectivity Count").GetRange())
print ("Point connectivity range: ", mapper.GetScalarRange())

actor = vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(1,1,1)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren1)

ren1.AddActor(actor)
camera = vtkCamera()
camera.SetFocalPoint(0,0,0)
camera.SetPosition(0,0,1)
ren1.SetActiveCamera(camera)

ren1.SetBackground(0, 0, 0)

renWin.SetSize(250,250)

# render and interact with data

iRen = vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin);
ren1.ResetCamera()
renWin.Render()

iRen.Initialize()
#iRen.Start()
