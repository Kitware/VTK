#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

sr = vtkSTLReader()
sr.SetFileName("../../../vtkdata/42400-IDGH.stl")

stlMapper = vtkPolyDataMapper()
stlMapper.SetInput(sr.GetOutput())
stlActor = vtkLODActor()
stlActor.SetMapper(stlMapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(stlActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(500,500)

# render the image
#
cam1=ren.GetActiveCamera()
cam1.Zoom(1.4)
iren.Initialize()
#renWin SetFileName "stl.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .
iren.Start()
