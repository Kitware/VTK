#!/usr/local/bin/python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

from colors import *
# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create lines
reader = vtkPolyDataReader()
reader.SetFileName(VTK_DATA + "/hello.vtk")
lineMapper = vtkPolyDataMapper()
lineMapper.SetInput(reader.GetOutput())
lineActor = vtkActor()
lineActor.SetMapper(lineMapper)
lineActor.GetProperty().SetColor(red[0],red[1],red[2])

# create implicit model
imp = vtkImplicitModeller()
imp.SetInput(reader.GetOutput())
imp.SetSampleDimensions(110,40,20)
imp.SetMaximumDistance(0.25)
imp.SetModelBounds(-1.0,10.0,-1.0,3.0,-1.0,1.0)
contour = vtkContourFilter()
contour.SetInput(imp.GetOutput())
contour.SetValue(0,0.25)
impMapper = vtkPolyDataMapper()
impMapper.SetInput(contour.GetOutput())
impMapper.ScalarVisibilityOff()
impActor = vtkActor()
impActor.SetMapper(impMapper)
impActor.GetProperty().SetColor(peacock[0],peacock[1],peacock[2])
impActor.GetProperty().SetOpacity(0.5)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(lineActor)
ren.AddActor(impActor)
ren.SetBackground(1,1,1)
renWin.SetSize(600,250)

camera = vtkCamera()
camera.SetClippingRange(1.81325,90.6627)
camera.SetFocalPoint(4.5,1,0)
camera.SetPosition(4.5,1.0,6.73257)
camera.SetViewUp(0,1,0)
camera.Zoom(0.8)
ren.SetActiveCamera(camera)

iren.Initialize()

# render the image
#



iren.Start()
