#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *


reader = vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

LUT = vtkLookupTable()
LUT.SetTableRange(0,1800)
LUT.SetSaturationRange(1,1)
LUT.SetHueRange(0,1)
LUT.SetValueRange(1,1)
LUT.SetAlphaRange(0,0)
LUT.Build()

mapToRGBA = vtkImageMapToColors()
mapToRGBA.SetInput(reader.GetOutput())
mapToRGBA.SetOutputFormatToRGBA()
mapToRGBA.SetLookupTable(LUT)

# set the window/level to 255.0/127.5 to view full range
viewer = vtkImageViewer()
viewer.SetInput(mapToRGBA.GetOutput())
viewer.SetColorWindow(255.0)
viewer.SetColorLevel(127.5)
viewer.SetZSlice(50)

viewer.Render()

#make interface
WindowLevelInterface(viewer)

windowToimage = vtkWindowToImageFilter()
windowToimage.SetInput(viewer.GetImageWindow())

pnmWriter = vtkPNMWriter()
pnmWriter.SetInput(windowToimage.GetOutput())
pnmWriter.SetFileName("TestMapToRGBA.tcl.ppm")
#pnmWriter.Write()

