#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *


# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create room profile
#
points = vtkPoints()
points.InsertPoint(0,1,0,0)
points.InsertPoint(1,0,0,0)
points.InsertPoint(2,0,10,0)
points.InsertPoint(3,12,10,0)
points.InsertPoint(4,12,0,0)
points.InsertPoint(5,3,0,0)

lines = vtkCellArray()
lines.InsertNextCell(6) #number.of.points
lines.InsertCellPoint(0)
lines.InsertCellPoint(1)
lines.InsertCellPoint(2)
lines.InsertCellPoint(3)
lines.InsertCellPoint(4)
lines.InsertCellPoint(5)

profile = vtkPolyData()
profile.SetPoints(points)
profile.SetLines(lines)

# extrude profile to make wall
#
extrude = vtkLinearExtrusionFilter()
extrude.SetInput(profile)
extrude.SetScaleFactor(8)
extrude.SetVector(0,0,1)
extrude.CappingOff()
    
map = vtkPolyDataMapper()
map.SetInput(extrude.GetOutput())

wall = vtkActor()
wall.SetMapper(map)
wall.GetProperty().SetColor(0.3800,0.7000,0.1600)
#[wall GetProperty] BackfaceCullingOff
#[wall GetProperty] FrontfaceCullingOn

# Add the actors to the renderer, set the background and size
#
ren.AddActor(wall)
ren.SetBackground(1,1,1)

renWin.SetSize(500,500)

ren.GetActiveCamera().SetViewUp(-0.181457,0.289647,0.939776)
ren.GetActiveCamera().SetPosition(23.3125,-28.2001,17.5756)
ren.GetActiveCamera().SetFocalPoint(6,5,4)
ren.GetActiveCamera().SetViewAngle(30)

renWin.Render()
# render the image
#
iren.Initialize()




iren.Start()
