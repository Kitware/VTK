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
sphere.SetPhiResolution(25)
sphere.SetThetaResolution(25)
plane = vtkPlane()
plane.SetOrigin(0.25,0,0)
plane.SetNormal(-1,-1,0)
clipper = vtkClipPolyData()
clipper.SetInput(sphere.GetOutput())
clipper.SetClipFunction(plane)
clipper.GenerateClipScalarsOn()
clipper.SetValue(0.0)
clipMapper = vtkPolyDataMapper()
clipMapper.SetInput(clipper.GetOutput())
clipMapper.ScalarVisibilityOff()
backProp = vtkProperty()
backProp.SetDiffuseColor(tomato)
clipActor = vtkActor()
clipActor.SetMapper(clipMapper)
clipActor.GetProperty().SetColor(peacock[0],peacock[1],peacock[2])
clipActor.SetBackfaceProperty(backProp)

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
ren.GetActiveCamera().Azimuth(30)
ren.GetActiveCamera().Elevation(30)
ren.GetActiveCamera().Dolly(1.2)
ren.ResetCameraClippingRange()

renWin.SetSize(400,400)
iren.Initialize()

# render the image
#
renWin.SetFileName("clipSphere.ppm")



iren.Start()
