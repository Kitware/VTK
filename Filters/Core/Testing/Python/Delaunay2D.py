#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Create the RenderWindow, Renderer and both Actors
#
ren1 = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren1)
iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)
# create some points
#
math = vtk.vtkMath()
points = vtk.vtkPoints()
i = 0
while i < 1000:
    points.InsertPoint(i,math.Random(0,1),math.Random(0,1),0.0)
    i = i + 1

profile = vtk.vtkPolyData()
profile.SetPoints(points)
# triangulate them
#
del1 = vtk.vtkDelaunay2D()
del1.SetInputData(profile)
del1.BoundingTriangulationOn()
del1.SetTolerance(0.001)
del1.SetAlpha(0.0)
shrink = vtk.vtkShrinkPolyData()
shrink.SetInputConnection(del1.GetOutputPort())
map = vtk.vtkPolyDataMapper()
map.SetInputConnection(shrink.GetOutputPort())
triangulation = vtk.vtkActor()
triangulation.SetMapper(map)
triangulation.GetProperty().SetColor(1,0,0)
# Add the actors to the renderer, set the background and size
#
ren1.AddActor(triangulation)
ren1.SetBackground(1,1,1)
renWin.SetSize(500,500)
renWin.Render()
cam1 = ren1.GetActiveCamera()
cam1.Zoom(1.5)
renWin.Render()
# prevent the tk window from showing up then start the event loop
# --- end of script --
