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

# create some points
#
math = vtkMath()
points = vtkFloatPoints()
for i in range(0,50):
	points.InsertPoint(i,math.Random(0,1),math.Random(0,1),math.Random(0,1))
 

scalars = vtkFloatScalars()
for i in range(0,50):
	scalars.InsertScalar(i,math.Random(0,1))
 

profile = vtkPolyData()
profile.SetPoints(points)
profile.GetPointData().SetScalars(scalars)

# triangulate them
#
shepard = vtkShepardMethod()
shepard.SetInput(profile)
shepard.SetModelBounds(0,1,0,1,.1,.5)
#    shepard SetMaximumDistance .1
shepard.SetNullValue(1)
shepard.SetSampleDimensions(20,20,20)
shepard.Update()
    
map = vtkDataSetMapper()
map.SetInput(shepard.GetOutput())

block = vtkActor()
block.SetMapper(map)
block.GetProperty().SetColor(1,0,0)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(block)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)

cam1=ren.GetActiveCamera()
cam1.Azimuth(160)
cam1.Elevation(30)
cam1.Zoom(1.5)

renWin.Render()

# render the image
#

renWin.Render()




iren.Start()
