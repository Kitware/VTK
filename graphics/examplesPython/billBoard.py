#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKImagingPython import *

# Create a rolling billboard - requires texture support

# Get the interactor

# load in the texture map
#
pnmReader = vtkPNMReader()
pnmReader.SetFileName(VTK_DATA + "/billBoard.pgm")
atext = vtkTexture()
atext.SetInput(pnmReader.GetOutput())
atext.InterpolateOn()

# create a plane source and actor
plane = vtkPlaneSource()
plane.SetPoint1(1024,0,0)
plane.SetPoint2(0,64,0)
trans = vtkTransformTextureCoords()
trans.SetInput(plane.GetOutput())
planeMapper = vtkDataSetMapper()
planeMapper.SetInput(trans.GetOutput())
planeActor = vtkActor()
planeActor.SetMapper(planeMapper)
planeActor.SetTexture(atext)

# Create graphics stuff
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
ren.AddActor(planeActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(512,32)

# Setup camera
camera = vtkCamera()
camera.SetClippingRange(11.8369,591.843)
camera.SetFocalPoint(512,32,0)
camera.SetPosition(512,32,118.369)
camera.SetViewAngle(30)
camera.SetViewUp(0,1,0)
ren.SetActiveCamera(camera)
renWin.Render()

for i in range(0,112):
	trans.AddPosition(0.01,0,0)
	renWin.Render()
 
for i in range(0,40):
	trans.AddPosition(0,0.05,0)
	renWin.Render()
 
for i in range(0,112):
	trans.AddPosition(-0.01,0,0)
	renWin.Render()
 







iren.Start()
