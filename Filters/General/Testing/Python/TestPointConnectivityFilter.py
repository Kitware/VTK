#!/usr/bin/env python
import math
import vtk
import vtk.test.Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

npts = 1000

# Create a pipeline with variable point connectivity
math = vtk.vtkMath()
points = vtk.vtkPoints()
i = 0
while i < npts:
    points.InsertPoint(i,math.Random(0,1),math.Random(0,1),0.0)
    i = i + 1

profile = vtk.vtkPolyData()
profile.SetPoints(points)
# triangulate them
#
del1 = vtk.vtkDelaunay2D()
del1.SetInputData(profile)
del1.BoundingTriangulationOff()
del1.SetTolerance(0.001)
del1.SetAlpha(0.0)

conn = vtk.vtkPointConnectivityFilter()
conn.SetInputConnection(del1.GetOutputPort())
conn.Update()

mapper = vtk.vtkPolyDataMapper()
mapper.SetInputConnection(conn.GetOutputPort())
mapper.SetScalarModeToUsePointFieldData()
mapper.SelectColorArray("Point Connectivity Count")
mapper.SetScalarRange(conn.GetOutput().GetPointData().GetArray("Point Connectivity Count").GetRange())
print ("Point connectivity range: ", mapper.GetScalarRange())

actor = vtk.vtkActor()
actor.SetMapper(mapper)
actor.GetProperty().SetColor(1,1,1)

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.AddRenderer(ren1)

ren1.AddActor(actor)
camera = vtk.vtkCamera()
camera.SetFocalPoint(0,0,0)
camera.SetPosition(0,0,1)
ren1.SetActiveCamera(camera)

ren1.SetBackground(0, 0, 0)

renWin.SetSize(250,250)

# render and interact with data

iRen = vtk.vtkRenderWindowInteractor()
iRen.SetRenderWindow(renWin);
ren1.ResetCamera()
renWin.Render()

iRen.Initialize()
#iRen.Start()
