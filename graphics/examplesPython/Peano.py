#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create bottle profile
#
points = vtkPoints()
points.InsertNextPoint(0,0,0)
points.InsertNextPoint(1,0,0)
points.InsertNextPoint(1,1,0)
points.InsertNextPoint(0,1,0)
points.InsertNextPoint(0,1,1)
points.InsertNextPoint(1,1,1)
points.InsertNextPoint(1,0,1)
points.InsertNextPoint(0,0,1)
points.InsertNextPoint(0,0,2)
points.InsertNextPoint(0,1,2)
points.InsertNextPoint(0,1,3)
points.InsertNextPoint(0,0,3)
points.InsertNextPoint(1,0,3)
points.InsertNextPoint(1,1,3)
points.InsertNextPoint(1,1,2)
points.InsertNextPoint(1,0,2)
points.InsertNextPoint(2,0,2)
points.InsertNextPoint(2,1,2)
points.InsertNextPoint(2,1,3)
points.InsertNextPoint(2,0,3)
points.InsertNextPoint(3,0,3)
points.InsertNextPoint(3,1,3)
points.InsertNextPoint(3,1,2)
points.InsertNextPoint(3,0,2)
points.InsertNextPoint(3,0,1)
points.InsertNextPoint(3,0,0)
points.InsertNextPoint(2,0,0)
points.InsertNextPoint(2,0,1)
points.InsertNextPoint(2,1,1)
points.InsertNextPoint(2,1,0)
points.InsertNextPoint(3,1,0)
points.InsertNextPoint(3,1,1)
points.InsertNextPoint(3,2,1)
points.InsertNextPoint(3,2,0)
points.InsertNextPoint(2,2,0)
points.InsertNextPoint(2,2,1)
points.InsertNextPoint(2,3,1)
points.InsertNextPoint(2,3,0)
points.InsertNextPoint(3,3,0)
points.InsertNextPoint(3,3,1)
points.InsertNextPoint(3,3,2)
points.InsertNextPoint(3,2,2)
points.InsertNextPoint(3,2,3)
points.InsertNextPoint(3,3,3)
points.InsertNextPoint(2,3,3)
points.InsertNextPoint(2,2,3)
points.InsertNextPoint(2,2,2)
points.InsertNextPoint(2,3,2)
points.InsertNextPoint(1,3,2)
points.InsertNextPoint(1,2,2)
points.InsertNextPoint(1,2,3)
points.InsertNextPoint(1,3,3)
points.InsertNextPoint(0,3,3)
points.InsertNextPoint(0,2,3)
points.InsertNextPoint(0,2,2)
points.InsertNextPoint(0,3,2)
points.InsertNextPoint(0,3,1)
points.InsertNextPoint(1,3,1)
points.InsertNextPoint(1,2,1)
points.InsertNextPoint(0,2,1)
points.InsertNextPoint(0,2,0)
points.InsertNextPoint(1,2,0)
points.InsertNextPoint(1,3,0)
points.InsertNextPoint(0,3,0)

lines = vtkCellArray()
lines.InsertNextCell(64) #number.of.points
for i in range(0,64):
	lines.InsertCellPoint(i)
     

curve = vtkPolyData()
curve.SetPoints(points)
curve.SetLines(lines)

tube = vtkTubeFilter()
tube.SetInput(curve)
tube.SetNumberOfSides(6)
tube.SetRadius(0.05)

map = vtkPolyDataMapper()
map.SetInput(tube.GetOutput())

peanoCurve = vtkActor()
peanoCurve.SetMapper(map)
peanoCurve.GetProperty().SetColor(0.3800,0.7000,0.1600)
peanoCurve.GetProperty().BackfaceCullingOn()

# Add the actors to the renderer, set the background and size
#
ren.AddActor(peanoCurve)
ren.SetBackground(1,1,1)

renWin.SetSize(500,500)
renWin.Render()
ren.GetActiveCamera().Zoom(1.4)
renWin.Render()

renWin.Render()

iren.Start()
