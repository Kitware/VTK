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
reader.SetDataScalarType(VTK_SHORT)
#reader.DebugOn()

thresh = vtkImageThreshold()
thresh.SetInput(reader.GetOutput())
thresh.ThresholdByUpper(2000.0)
thresh.SetInValue(255)
thresh.SetOutValue(0)

viewer = vtkImageViewer()
viewer.SetInput(thresh.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
#viewer.DebugOn()

# make interface
WindowLevelInterface(viewer)
