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

thresh = vtkImageThreshold()
thresh.SetInput(reader.GetOutput())
thresh.ThresholdBetween(500.0,2200.0)
#thresh.SetInValue(0)
thresh.SetOutValue(500)
thresh.ReleaseDataFlagOff()

viewer = vtkImageViewer()
viewer.SetInput(thresh.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
#viewer.DebugOn()

# make interface
WindowLevelInterface(viewer)
