#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *


# Image pipeline

reader = vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix("../../../vtkdata/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.DebugOn()

range = vtkImageRange3D()
range.SetInput(reader.GetOutput())
range.SetKernelSize(5,5,5)

viewer = vtkImageViewer()
viewer.SetInput(range.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(1000)
viewer.SetColorLevel(500)
#viewer.DebugOn()


WindowLevelInterface(viewer)
