#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# Test the vtkImageCast filter.
# Cast the shorts to unsinged chars.  This will cause overflow artifacts
# because the data does not fit into 8 bits.


# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.ReleaseDataFlagOff()
#reader.DebugOn()

cast = vtkImageCast()
cast.SetInput(reader.GetOutput())
cast.SetOutputScalarType(VTK_UNSIGNED_CHAR)

viewer = vtkImageViewer()
viewer.SetInput(cast.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(200)
viewer.SetColorLevel(60)
#viewer.DebugOn()
viewer.Render()


# make interface
WindowLevelInterface(viewer)
