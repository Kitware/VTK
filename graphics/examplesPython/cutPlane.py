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

# create a plane 
#
plane = vtkPlaneSource()
plane.SetResolution(5,5)

planeTris = vtkTriangleFilter()
planeTris.SetInput(plane.GetOutput())

halfPlane = vtkPlane()
halfPlane.SetOrigin(-.13,-.03,0)
halfPlane.SetNormal(1,.2,0)

planeCutter = vtkCutter()
planeCutter.SetCutFunction(halfPlane)
planeCutter.SetInput(planeTris.GetOutput())
planeCutter.SetValue(0,0)

planeMapper = vtkPolyDataMapper()
planeMapper.SetInput(planeCutter.GetOutput())
planeMapper.ScalarVisibilityOff()

planeActor = vtkActor()
planeActor.SetMapper(planeMapper)
planeActor.GetProperty().SetDiffuseColor(0,0,0)
planeActor.GetProperty().SetRepresentationToWireframe()
# Add the actors to the renderer, set the background and size
#
ren.AddActor(planeActor)
ren.SetBackground(1,1,1)
renWin.SetSize(300,300)



# render the image
#
iren.Initialize()



iren.Start()
