#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl
#source ../../examplesTcl/colors.tcl
from colors import *# create planes

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
pl3d = vtkPLOT3DReader()
pl3d.SetXYZFileName("../../../vtkdata/combxyz.bin")
pl3d.SetQFileName("../../../vtkdata/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
plane = vtkStructuredGridGeometryFilter()
plane.SetInput(pl3d.GetOutput())
plane.SetExtent(0,100,0,100,0,0)
planeMapper = vtkPolyDataMapper()
planeMapper.SetInput(plane.GetOutput())
planeMapper.SetScalarRange(0.197813,0.710419)
planeActor = vtkActor()
planeActor.SetMapper(planeMapper)

outline = vtkStructuredGridOutlineFilter()
outline.SetInput(pl3d.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(black[0],black[1],black[2])

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(planeActor)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)
iren.Initialize()

cam1=ren.GetActiveCamera()
cam1.SetClippingRange(3.95297,50)
cam1.SetFocalPoint(8.88908,0.595038,29.3342)
cam1.SetPosition(-12.3332,31.7479,41.2387)
cam1.ComputeViewPlaneNormal()
cam1.SetViewUp(0.060772,-0.319905,0.945498)

# render the image
#

for j in range(0,3):
	for i in range(0,25):
		plane.SetExtent(0,100,0,100,i,i)
		renWin.Render()
     
 

#renWin SetFileName "movePlane.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .



iren.Start()
