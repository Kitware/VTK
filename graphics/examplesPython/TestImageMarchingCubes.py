#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *

#catch  load vtktcl 
# get the interactor ui
#source ../../examplesTcl/vtkInt.tcl
#source ../../examplesTcl/colors.tcl
from colors import *
# Create the RenderWindow, Renderer and both Actors
#
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# create pipeline
#
v16 = vtkVolume16Reader()
v16.SetDataDimensions(128,128)
v16.GetOutput().SetOrigin(0.0,0.0,0.0)
v16.SetDataByteOrderToLittleEndian()
v16.SetFilePrefix("../../../vtkdata/headsq/half")
#v16 SetImageRange 19 24
v16.SetImageRange(1,93)
v16.SetDataSpacing(1.6,1.6,1.5)
v16.Update()

#vtkImageMarchingCubes iso
iso = vtkMarchingCubes()
iso.SetInput(v16.GetOutput())
iso.SetValue(0,1150)
#iso SetStartMethod {puts "Start Marching"}
#iso SetProgressMethod {puts "Progress ..."}
#iso SetEndMethod {puts "Finished Marching"}
#iso SetInputMemoryLimit 1000


isoMapper = vtkPolyDataMapper()
isoMapper.SetInput(iso.GetOutput())
isoMapper.ScalarVisibilityOff()

isoActor = vtkActor()
isoActor.SetMapper(isoMapper)
isoActor.GetProperty().SetColor(antique_white[0],antique_white[1],antique_white[2])

outline = vtkOutlineFilter()
outline.SetInput(v16.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.VisibilityOff()

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(isoActor)
ren.SetBackground(0.2,0.3,0.4)
renWin.SetSize(450,450)
ren.GetActiveCamera().Elevation(90)
ren.GetActiveCamera().SetViewUp(0,0,-1)
ren.GetActiveCamera().Azimuth(180)
iren.Initialize()

# render the image
#
#renWin SetFileName "mcubes.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .


iren.Start()
