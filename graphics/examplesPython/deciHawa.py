#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKPatentedPython import *

#catch  load vtktcl 
# decimate hawaii dataset
#
# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl
#source ../../examplesTcl/colors.tcl
from colors import *
# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create a cyberware source
#
reader = vtkPolyDataReader()
reader.SetFileName("../../../vtkdata/honolulu.vtk")
deci = vtkDecimate()
deci.SetInput(reader.GetOutput())
deci.SetTargetReduction(0.9)
deci.SetAspectRatio(20)
deci.SetInitialError(0.0002)
deci.SetErrorIncrement(0.0005)
deci.SetMaximumIterations(6)
deci.SetInitialFeatureAngle(45)
hawaiiMapper = vtkPolyDataMapper()
hawaiiMapper.SetInput(deci.GetOutput())
hawaiiActor = vtkActor()
hawaiiActor.SetMapper(hawaiiMapper)
hawaiiActor.GetProperty().SetColor(turquoise_blue[0],turquoise_blue[1],turquoise_blue[2])
hawaiiActor.GetProperty().SetRepresentationToWireframe()

# Add the actors to the renderer, set the background and size
#
ren.AddActor(hawaiiActor)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)

# render the image
#

iren.Initialize()

# prevent the tk window from showing up then start the event loop
#wm withdraw .


iren.Start()
