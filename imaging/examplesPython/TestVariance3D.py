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

var = vtkImageVariance3D()
var.SetInput(reader.GetOutput())
var.SetKernelSize(3,3,1)

viewer = vtkImageViewer()
viewer.SetInput(var.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(3000)
viewer.SetColorLevel(1000)
#viewer.DebugOn()

WindowLevelInterface(viewer)
