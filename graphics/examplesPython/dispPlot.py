#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
# this is a tcl version of plate vibration
# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl

# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# read a vtk file
#
plate = vtkPolyDataReader()
plate.SetFileName("../../../vtkdata/plate.vtk")
plate.SetVectorsName("mode8")
warp = vtkWarpVector()
warp.SetInput(plate.GetOutput())
warp.SetScaleFactor(0.5)
normals = vtkPolyDataNormals()
normals.SetInput(warp.GetPolyDataOutput())
color = vtkVectorDot()
color.SetInput(normals.GetOutput())
lut = vtkLookupTable()
lut.SetNumberOfColors(256)
lut.Build()
for i in range(0,128):
	lut.SetTableValue(i,(128.0-i)/128.0,(128.0-i)/128.0,(128.0-i)/128.0,1)
     
for i in range(128,256):
	lut.SetTableValue(i,(i-128.0)/128.0,(i-128.0)/128.0,(i-128.0)/128.0,1)
     

plateMapper = vtkDataSetMapper()
plateMapper.SetInput(color.GetOutput())
plateMapper.SetLookupTable(lut)
plateMapper.SetScalarRange(-1,1)
plateActor = vtkActor()
plateActor.SetMapper(plateMapper)

# Add the actors to the renderer, set the background and size
#
ren.AddActor(plateActor)
ren.SetBackground(1,1,1)
renWin.SetSize(500,500)

# render the image
#
iren.Initialize()

#renWin SetFileName "dispPlot.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .


iren.Start()
