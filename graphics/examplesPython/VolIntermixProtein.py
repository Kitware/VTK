#!/usr/local/bin/python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from libVTKCommonPython import *
from libVTKGraphicsPython import *



reader = vtkSLCReader()
reader.SetFileName(VTK_DATA + "/poship.slc")

reader2 = vtkSLCReader()
reader2.SetFileName(VTK_DATA + "/neghip.slc")

opacityTransferFunction = vtkPiecewiseFunction()
opacityTransferFunction.AddPoint(20,0.0)
opacityTransferFunction.AddPoint(255,0.3)

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

volumeProperty = vtkVolumeProperty()
volumeProperty.SetColor(colorTransferFunction)
volumeProperty.SetScalarOpacity(opacityTransferFunction)
volumeProperty.SetInterpolationTypeToLinear()
volumeProperty.ShadeOff()

compositeFunction = vtkVolumeRayCastCompositeFunction()

volumeMapper = vtkVolumeRayCastMapper()
volumeMapper.SetInput(reader.GetOutput())
volumeMapper.SetVolumeRayCastFunction(compositeFunction)
volumeMapper.SetSampleDistance(0.25)

volume = vtkVolume()
volume.SetMapper(volumeMapper)
volume.SetProperty(volumeProperty)

contour = vtkContourFilter()
contour.SetInput(reader2.GetOutput())
contour.SetValue(0,128.0)

neghip_mapper = vtkPolyDataMapper()
neghip_mapper.SetInput(contour.GetOutput())
neghip_mapper.ScalarVisibilityOff()

neghip = vtkActor()
neghip.SetMapper(neghip_mapper)
neghip.GetProperty().SetColor(0.8,0.2,0.8)
neghip.GetProperty().SetAmbient(0.1)
neghip.GetProperty().SetDiffuse(0.6)
neghip.GetProperty().SetSpecular(0.4)

# Okay now the graphics stuff
ren = vtkRenderer()
ren.SetBackground(0.1,0.2,0.4)
renWin = vtkRenderWindow()
renWin.AddRenderer(ren)
renWin.SetSize(256,256)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren.AddActor(neghip)
ren.SetBackground(0,0,0)

ren.GetActiveCamera().SetPosition(162.764,30.8946,116.029)
ren.GetActiveCamera().SetFocalPoint(32.868,31.5566,31.9246)
ren.GetActiveCamera().SetViewUp(-0.00727828,0.999791,0.0191114)
ren.GetActiveCamera().SetClippingRange(15.4748,773.74)

ren.AddVolume(volume)
#renWin.SetSize(200,200)
renWin.Render()

iren.SetDesiredUpdateRate(1)
iren.Initialize()




iren.Start()
