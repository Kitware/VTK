#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#
# Demonstrate the use of clipping on polygonal data
#
from colors import *
# create pipeline
#
sphere = vtkSphereSource()
sphere.SetRadius(1)
sphere.SetPhiResolution(50)
sphere.SetThetaResolution(50)
#vtkPlaneSource sphere
#    sphere SetXResolution 10
#    sphere SetYResolution 25
#vtkConeSource sphere
#    sphere SetResolution 10

plane = vtkPlane()
plane.SetOrigin(0.25,0,0)
plane.SetNormal(-1,-1,0)
iwf = vtkImplicitWindowFunction()
iwf.SetImplicitFunction(plane)
iwf.SetWindowRange(-.2,.2)
iwf.SetWindowValues(0,1)
clipper = vtkClipPolyData()
clipper.SetInput(sphere.GetOutput())
clipper.SetClipFunction(iwf)
clipper.SetValue(0.0)
clipper.GenerateClipScalarsOn()
clipMapper = vtkDataSetMapper()
clipMapper.SetInput(clipper.GetOutput())
clipMapper.ScalarVisibilityOff()
clipActor = vtkActor()
clipActor.SetMapper(clipMapper)
clipActor.GetProperty().SetColor(peacock[0],peacock[1],peacock[2])

# Create graphics stuff
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(clipActor)
ren.SetBackground(1,1,1)
renWin.SetSize(400,400)
iren.Initialize()

# render the image
#
renWin.SetFileName("clipSphere2.ppm")



iren.Start()
