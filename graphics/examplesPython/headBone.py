#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *
from libVTKPatentedPython import *

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
lgt = vtkLight()

# create pipeline
#
v16 = vtkVolume16Reader()
v16.SetDataDimensions(128,128)
v16.GetOutput().SetOrigin(0.0,0.0,0.0)
v16.SetDataByteOrderToLittleEndian()
v16.SetFilePrefix("../../../vtkdata/headsq/half")
v16.SetImageRange(1,93)
v16.SetDataSpacing(1.6,1.6,1.5)
iso = vtkMarchingCubes()
iso.SetInput(v16.GetOutput())
iso.SetValue(0,1150)
isoMapper = vtkPolyDataMapper()
isoMapper.SetInput(iso.GetOutput())
isoMapper.ScalarVisibilityOff()
isoActor = vtkActor()
isoActor.SetMapper(isoMapper)
isoProp=isoActor.GetProperty()
isoProp.SetColor(antique_white[0],antique_white[1],antique_white[2])

outline = vtkOutlineFilter()
outline.SetInput(v16.GetOutput())
outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())
outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineProp=outlineActor.GetProperty() #eval $outlineProp SetColor 0 0 0

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(isoActor)
ren.SetBackground(1,1,1)
ren.AddLight(lgt)
renWin.SetSize(500,500)
ren.SetBackground(0.1,0.2,0.4)

cam1=ren.GetActiveCamera()
cam1.Elevation(90)
cam1.SetViewUp(0,0,-1)
cam1.Zoom(1.3)
lgt.SetPosition(cam1.GetPosition())
lgt.SetFocalPoint(cam1.GetFocalPoint())

# render the image
#

renWin.Render()
#renWin SetFileName "headBone.tcl.ppm"
#renWin SaveImageAsPPM

# prevent the tk window from showing up then start the event loop
#wm withdraw .


iren.Start()
