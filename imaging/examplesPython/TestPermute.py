#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *

# Simple viewer for images.


# Image pipeline

reader = vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix("../../../vtkdata/fullHead/headsq")
reader.SetDataMask(0x7fff)

permute = vtkImagePermute()
permute.SetInput(reader.GetOutput())
permute.SetFilteredAxes(VTK_IMAGE_Y_AXIS,VTK_IMAGE_Z_AXIS,VTK_IMAGE_X_AXIS)

viewer = vtkImageViewer()
viewer.SetInput(permute.GetOutput())
viewer.SetZSlice(128)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
#viewer.DebugOn()
#viewer.Render()

#make interface
WindowLevelInterface(viewer)
