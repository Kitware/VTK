#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *


# Simple volume rendering example.
reader = vtkSLCReader()
reader.SetFileName(VTK_DATA + "/poship.slc")

# Create transfer functions for opacity and color
opacityTransferFunction = vtkPiecewiseFunction()
opacityTransferFunction.AddPoint(20,0.0)
opacityTransferFunction.AddPoint(255,0.2)

colorTransferFunction = vtkColorTransferFunction()
colorTransferFunction.AddRedPoint(0.0,0.0)
colorTransferFunction.AddRedPoint(64.0,1.0)
colorTransferFunction.AddRedPoint(128.0,0.0)
colorTransferFunction.AddRedPoint(255.0,0.0)
colorTransferFunction.AddBluePoint(0.0,0.0)
colorTransferFunction.AddBluePoint(64.0,0.0)
colorTransferFunction.AddBluePoint(128.0,1.0)
colorTransferFunction.AddBluePoint(192.0,0.0)
colorTransferFunction.AddBluePoint(255.0,0.0)
colorTransferFunction.AddGreenPoint(0.0,0.0)
colorTransferFunction.AddGreenPoint(128.0,0.0)
colorTransferFunction.AddGreenPoint(192.0,1.0)
colorTransferFunction.AddGreenPoint(255.0,0.2)

# Create properties, mappers, volume actors, and ray cast function
volumeProperty = vtkVolumeProperty()
volumeProperty.SetColor(colorTransferFunction)
volumeProperty.SetScalarOpacity(opacityTransferFunction)

compositeFunction = vtkVolumeRayCastCompositeFunction()

volumeMapper = vtkVolumeRayCastMapper()
volumeMapper.SetInput(reader.GetOutput())
volumeMapper.SetVolumeRayCastFunction(compositeFunction)

volume = vtkVolume()
volume.SetMapper(volumeMapper)
volume.SetProperty(volumeProperty)

# Create outline
outline = vtkOutlineFilter()
outline.SetInput(reader.GetOutput())

outlineMapper = vtkPolyDataMapper()
outlineMapper.SetInput(outline.GetOutput())

outlineActor = vtkActor()
outlineActor.SetMapper(outlineMapper)
outlineActor.GetProperty().SetColor(1,1,1)

# Okay now the graphics stuff
ren = vtkRenderer()
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(256,256)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

#ren1 AddActor outlineActor
ren.AddVolume(volume)
ren.SetBackground(0.1,0.2,0.4)
renWin.Render()

def TkCheckAbort():
	foo=renWin.GetEventPending()
	if foo != 0:
		renWin.SetAbortRender(1)
 
renWin.SetAbortCheckMethod(TkCheckAbort)

iren.Initialize()



iren.Start()
