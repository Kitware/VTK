#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
# this is a tcl version of the Mace example
# include get the vtk interactor ui
#source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

cone = vtkCylinderSource()
#vtkDiskSource cone
#cone SetInnerRadius 1.0
#cone SetOuterRadius 1.0
coneMapper = vtkPolyDataMapper()
coneMapper.SetInput(cone.GetOutput())
coneActor = vtkActor()
coneActor.SetMapper(coneMapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(coneActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(450,450)

# Get handles to some useful objects
#
iren.Initialize()
renWin.Render()

#renWin SetFileName Cylinder.tcl.ppm
#renWin SaveImageAsPPM

coneProp=coneActor.GetProperty()
# prevent the tk window from showing up then start the event loop
#wm withdraw .



iren.Start()
