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

# create a sphere source and actor
#
sphere = vtkSphereSource()

# create the spikes using a cone source and the sphere source
#
atext = vtkVectorText()
count=3
atext.SetText("Welcome to VTK\n" + \
"An exciting new adventure\n" + \
"brought to you by over\n" + \
"%d" % count + " monkeys at work for\n" + \
"over three years.")

shrink = vtkShrinkPolyData()
shrink.SetInput(atext.GetOutput())
shrink.SetShrinkFactor(0.1)

spikeMapper = vtkPolyDataMapper()
spikeMapper.SetInput(shrink.GetOutput())
spikeActor = vtkActor()
spikeActor.SetMapper(spikeMapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(spikeActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(500,300)
cam1=ren.GetActiveCamera()
cam1.Zoom(2.4)

while count < 27:
	renWin.Render()
	count=count+1
	shrink.SetShrinkFactor(count/27.0)

	atext.SetText("Welcome to VTK\n" + \
	"An exciting new adventure\n" + \
	"brought to you by over\n" + \
	"%d" % count + " monkeys at work for\n" + \
	"over three years.")
 

# render the image
#
iren.Initialize()

iren.Start()
