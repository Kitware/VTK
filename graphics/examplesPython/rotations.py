#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

# Generate implicit model of a sphere
#
from colors import *
# Create renderer stuff
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
cone = vtkBYUReader()
cone.SetGeometryFileName(VTK_DATA + "/Viewpoint/cow.g")

coneMapper = vtkPolyDataMapper()
coneMapper.SetInput(cone.GetOutput())
coneActor = vtkActor()
coneActor.SetMapper(coneMapper)
coneActor.GetProperty().SetDiffuseColor(0.9608,0.8706,0.7020)

coneAxesSource = vtkAxes()
coneAxesSource.SetScaleFactor(10)
coneAxesSource.SetOrigin(0,0,0)

coneAxesMapper = vtkPolyDataMapper()
coneAxesMapper.SetInput(coneAxesSource.GetOutput())
 
coneAxes = vtkActor()
coneAxes.SetMapper(coneAxesMapper)

ren.AddActor(coneAxes)
coneAxes.VisibilityOff()

# Add the actors to the renderer, set the background and size
#
ren.AddActor(coneActor)
ren.SetBackground(0.1,0.2,0.4)
#renWin SetSize 1280 1024
renWin.SetSize(640,480)
ren.GetActiveCamera().Azimuth(0)
ren.GetActiveCamera().Dolly(1.4)
ren.ResetCameraClippingRange()
iren.Initialize()
coneAxes.VisibilityOn()
renWin.Render()

# render the image
#



#
def RotateX():
	coneActor.SetOrientation(0,0,0)
	renWin.Render()
	renWin.Render()
	renWin.EraseOff()
	for i in range(1,6+1):
		coneActor.RotateX(60)
		renWin.Render()
		renWin.Render()
     
	renWin.EraseOn()
 
def RotateY():
	coneActor.SetOrientation(0,0,0)
	renWin.Render()
	renWin.Render()
	renWin.EraseOff()
	for i in range(1,6+1):
		coneActor.RotateY(60)
		renWin.Render()
		renWin.Render()
     
	renWin.EraseOn()
 
def RotateZ():
	coneActor.SetOrientation(0,0,0)
	renWin.Render()
	renWin.Render()
	renWin.EraseOff()
	for i in range(1,6+1):
		coneActor.RotateZ(60)
		renWin.Render()
		renWin.Render()
     
	renWin.EraseOn()
 
def RotateXY():
	coneActor.SetOrientation(0,0,0)
	coneActor.RotateX(60)
	renWin.Render()
	renWin.Render()
	renWin.EraseOff()
	for i in range(1,6+1):
		coneActor.RotateY(60)
		renWin.Render()
		renWin.Render()
     
	renWin.EraseOn()
 

RotateX()
renWin.SetFileName("rotX.ppm")

RotateY()
renWin.SetFileName("rotY.ppm")

RotateZ()
renWin.SetFileName("rotZ.ppm")

RotateXY()
renWin.EraseOff()
renWin.SetFileName("rotXY.ppm")

iren.Start()
