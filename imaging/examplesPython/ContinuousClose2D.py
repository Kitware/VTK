#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# Divergence measures rate of change of gradient.

# Image pipeline

reader = vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.DebugOn()

dilate = vtkImageContinuousDilate3D()
dilate.SetInput(reader.GetOutput())
dilate.SetKernelSize(11,11,1)

erode = vtkImageContinuousErode3D()
erode.SetInput(dilate.GetOutput())
erode.SetKernelSize(11,11,1)

viewer = vtkImageViewer()
viewer.SetInput(erode.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)


WindowLevelInterface(viewer)
