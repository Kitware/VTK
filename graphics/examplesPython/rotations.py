#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
# Generate implicit model of a sphere
#
#source ../../examplesTcl/vtkInt.tcl
#source ../../examplesTcl/colors.tcl
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
cone.SetGeometryFileName("../../../vtkdata/Viewpoint/cow.g")

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
iren.Initialize()
coneAxes.VisibilityOn()
renWin.Render()

# render the image
#


# prevent the tk window from showing up then start the event loop
#wm withdraw .

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
#renWin SaveImageAsPPM

RotateY()
renWin.SetFileName("rotY.ppm")
#renWin SaveImageAsPPM

RotateZ()
renWin.SetFileName("rotZ.ppm")
#renWin SaveImageAsPPM

RotateXY()
renWin.EraseOff()
renWin.SetFileName("rotXY.ppm")
#renWin SaveImageAsPPM

#renWin SetFileName "rotations.tcl.ppm"
#renWin SaveImageAsPPM
iren.Start()
