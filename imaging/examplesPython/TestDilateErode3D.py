#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# A script to test DilationErode filter
# First the image is thresholded.
# It is the dilated with a spher of radius 5.


# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

thresh = vtkImageThreshold()
thresh.SetInput(reader.GetOutput())
thresh.ThresholdByUpper(2000.0)
thresh.SetInValue(255)
thresh.SetOutValue(0)

dilate = vtkImageDilateErode3D()
dilate.SetInput(thresh.GetOutput())
dilate.SetDilateValue(255)
dilate.SetErodeValue(0)
dilate.SetKernelSize(5,5,5)
dilate.ReleaseDataFlagOff()

viewer = vtkImageViewer()
viewer.SetInput(dilate.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
#viewer.DebugOn()


# make interface
WindowLevelInterface(viewer)
