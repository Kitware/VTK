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

disk = vtkDiskSource()
disk.SetInnerRadius(1.0)
disk.SetOuterRadius(2.0)
disk.SetRadialResolution(1)
disk.SetCircumferentialResolution(20)

diskMapper = vtkPolyDataMapper()
diskMapper.SetInput(disk.GetOutput())
diskActor = vtkActor()
diskActor.SetMapper(diskMapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(diskActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(450,450)

# Get handles to some useful objects
#
iren.Initialize()
renWin.Render()

#renWin SetFileName Disk.tcl.ppm
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .



iren.Start()
