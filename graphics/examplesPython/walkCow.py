#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

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
cow = vtkBYUReader()
cow.SetGeometryFileName(VTK_DATA + "/Viewpoint/cow.g")

cowMapper = vtkPolyDataMapper()
cowMapper.SetInput(cow.GetOutput())
cowActor = vtkActor()
cowActor.SetMapper(cowMapper)
cowActor.GetProperty().SetDiffuseColor(0.9608,0.8706,0.7020)

cowAxesSource = vtkAxes()
cowAxesSource.SetScaleFactor(10)
cowAxesSource.SetOrigin(0,0,0)

cowAxesMapper = vtkPolyDataMapper()
cowAxesMapper.SetInput(cowAxesSource.GetOutput())
 
cowAxes = vtkActor()
cowAxes.SetMapper(cowAxesMapper)

ren.AddActor(cowAxes)
cowAxes.VisibilityOff()

# Add the actors to the renderer, set the background and size
#
ren.AddActor(cowActor)
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(640,480)
ren.GetActiveCamera().Azimuth(0)
ren.GetActiveCamera().Dolly(1.4)
ren.ResetCameraClippingRange()
iren.Initialize()
cowAxes.VisibilityOn()
renWin.Render()

# render the image
#


#
cowTransform = vtkTransform()

def walk():
#	cowActor.SetOrientation(0,0,0)
#	cowActor.SetOrigin(0,0,0)
#	cowActor.SetPosition(0,0,0)
#	renWin.Render()
#	renWin.Render()
#	renWin.EraseOff()
	renWin.EraseOn()
	for i in range(1,6+1):
#		cowActor RotateY 60
		cowTransform.Identity()
		cowTransform.RotateY(i*60)
		cowTransform.Translate(0,0,5)
#		cowActor.SetUserMatrix(cowTransform.GetMatrix())
		renWin.Render()
#		renWin.Render()
     
	renWin.EraseOn()
 


def walk2():
	cowActor.SetOrientation(0,0,0)
	renWin.Render()
	renWin.Render()
	renWin.EraseOff()
	cowActor.SetOrigin(0,0,-5)
	cowActor.SetPosition(0,0,5)
	cowTransform.Identity()
	cowActor.SetUserMatrix(cowTransform.GetMatrix())
	for i in range(1,6+1):
		cowActor.RotateY(60)
		renWin.Render()
		renWin.Render()
   
	renWin.EraseOn()
 

def walk3():
	cowActor.SetOrientation(0,0,0)
	renWin.Render()
	renWin.Render()
	renWin.EraseOff()
	cowActor.SetOrigin(0,0,-5)
	cowActor.SetPosition(0,0,0)
	cowTransform.Identity()
	cowActor.SetUserMatrix(cowTransform.GetMatrix())
	for i in range(1,6+1):
		cowActor.RotateY(60)
		renWin.Render()
		renWin.Render()
   
	renWin.EraseOn()
 

def walk4():
	cowActor.SetOrientation(0,0,0)
	renWin.Render()
	renWin.Render()
	renWin.EraseOff()
	cowActor.SetOrigin(6.11414,1.27386,.015175)
	cowActor.SetOrigin(0,0,0)
	cowActor.SetPosition(0,0,0)
	cowTransform.Identity()
	cowActor.SetUserMatrix(cowTransform.GetMatrix())
	for i in range(1,6+1):
		cowActor.RotateWXYZ(60,2.19574,-1.42455,-.0331036)
		renWin.Render()
		renWin.Render()
   
	renWin.EraseOn()
 


walk()
#walk2()
#walk3()
#walk4()
#renWin.EraseOff()
renWin.EraseOn()


iren.Start()
