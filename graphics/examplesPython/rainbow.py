#!/usr/local/bin/python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *

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
pl3d.SetXYZFileName(VTK_DATA + "/combxyz.bin")
pl3d.SetQFileName(VTK_DATA + "/combq.bin")
pl3d.SetScalarFunctionNumber(100)
pl3d.SetVectorFunctionNumber(202)
pl3d.Update()
plane = vtkStructuredGridGeometryFilter()
plane.SetInput(pl3d.GetOutput())
plane.SetExtent(1,100,1,100,7,7)
lut = vtkLookupTable()
planeMapper = vtkPolyDataMapper()
planeMapper.SetLookupTable(lut)
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
outlineProp=outlineActor.GetProperty() #eval $outlineProp SetColor $black

# different lookup tables for each figure
#

#black to white lut
#    lut SetHueRange 0 0
#    lut SetSaturationRange 0 0
#    lut SetValueRange 0.2 1.0

#red to blue lut
#    lut SetHueRange 0.0 0.667

#blue to red lut
#    lut SetHueRange 0.667 0.0

#funky constrast
lut.SetNumberOfColors(64)
lut.Build()
for i in range(0,16):
	lut.SetTableValue(i*16,red[0],red[1],red[2],1)
	lut.SetTableValue(i*16+1,green[0],green[1],green[2],1)
	lut.SetTableValue(i*16+2,blue[0],blue[1],blue[2],1)
	lut.SetTableValue(i*16+3,black[0],black[1],black[2],1)
     
#    eval lut SetTableValue 0 $coral 1
#    eval lut SetTableValue 1 $black 1
#    eval lut SetTableValue 2 $peacock 1
#    eval lut SetTableValue 3 $black 1
#    eval lut SetTableValue 4 $orchid 1
#    eval lut SetTableValue 5 $black 1
#    eval lut SetTableValue 6 $cyan 1
#    eval lut SetTableValue 7 $black 1
#    eval lut SetTableValue 8 $mint 1
#    eval lut SetTableValue 9 $black 1
#    eval lut SetTableValue 10 $tomato 1
#    eval lut SetTableValue 11 $black 1
#    eval lut SetTableValue 12 $sea_green 1
#    eval lut SetTableValue 13 $black 1
#    eval lut SetTableValue 14 $plum 1
#    eval lut SetTableValue 15 $black 1

# Add the actors to the renderer, set the background and size
#
ren.AddActor(outlineActor)
ren.AddActor(planeActor)
#ren1 SetBackground 1 1 1
ren.SetBackground(0.1,0.2,0.4)
renWin.SetSize(500,500)
renWin.DoubleBufferOn()
iren.Initialize()

cam1=ren.GetActiveCamera()
cam1.SetClippingRange(3.95297,50)
cam1.SetFocalPoint(8.88908,0.595038,29.3342)
cam1.SetPosition(-12.3332,31.7479,41.2387)
cam1.SetViewUp(0.060772,-0.319905,0.945498)

# render the image
#

renWin.Render()





iren.Start()
