#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#create triangle strip - won't see anything with backface culling on


# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create triangle strip
#
points = vtkPoints()
points.InsertPoint(0,0.0,0.0,0.0)
points.InsertPoint(1,0.0,1.0,0.0)
points.InsertPoint(2,1.0,0.0,0.0)
points.InsertPoint(3,1.0,1.0,0.0)
points.InsertPoint(4,2.0,0.0,0.0)
points.InsertPoint(5,2.0,1.0,0.0)
points.InsertPoint(6,3.0,0.0,0.0)
points.InsertPoint(7,3.0,1.0,0.0)
strips = vtkCellArray()
strips.InsertNextCell(8) #number.of.points
strips.InsertCellPoint(0)
strips.InsertCellPoint(1)
strips.InsertCellPoint(2)
strips.InsertCellPoint(3)
strips.InsertCellPoint(4)
strips.InsertCellPoint(5)
strips.InsertCellPoint(6)
strips.InsertCellPoint(7)
profile = vtkPolyData()
profile.SetPoints(points)
profile.SetStrips(strips)

map = vtkPolyDataMapper()
map.SetInput(profile)

strip = vtkActor()
strip.SetMapper(map)
strip.GetProperty().SetColor(0.3800,0.7000,0.1600)
strip.GetProperty().BackfaceCullingOff()

# Add the actors to the renderer, set the background and size
#
ren.AddActor(strip)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)
renWin.Render()


# render the image
#




iren.Start()
