#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKPatentedPython import *

# decimate hawaii dataset
#
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
reader.SetFileName(VTK_DATA + "/honolulu.vtk")
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



iren.Start()
