#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# A script to test the threshold filter.
# Values above 2000 are set to 255.
# Values below 2000 are set to 0.

# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

cast = vtkImageCast()
cast.SetOutputScalarType(VTK_SHORT)
cast.SetInput(reader.GetOutput())

thresh = vtkImageThreshold()
thresh.SetInput(cast.GetOutput())
thresh.ThresholdByUpper(2000.0)
thresh.SetInValue(0)
thresh.SetOutValue(200)
thresh.ReleaseDataFlagOff()

dist = vtkImageCityBlockDistance()
dist.SetDimensionality(2)
dist.SetInput(thresh.GetOutput())

viewer = vtkImageViewer()
viewer.SetInput(dist.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(117)
viewer.SetColorLevel(43)

viewer.Render()

windowToimage = vtkWindowToImageFilter()
windowToimage.SetInput(viewer.GetImageWindow())

pnmWriter = vtkPNMWriter()
pnmWriter.SetInput(windowToimage.GetOutput())
pnmWriter.SetFileName("TestCityBlockDistance.tcl.ppm")
#pnmWriter.Write()

# make interface
WindowLevelInterface(viewer)
