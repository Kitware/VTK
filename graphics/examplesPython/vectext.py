#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
# this is a tcl version of the Mace example
# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl

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

#renWin SetFileName "vectext.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .
iren.Start()
