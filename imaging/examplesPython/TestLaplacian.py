#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *

# Divergence measures rate of change of gradient.


# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix("../../../vtkdata/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.DebugOn()

laplacian = vtkImageLaplacian()
laplacian.SetDimensionality(3)
laplacian.SetInput(reader.GetOutput())

viewer = vtkImageViewer()
viewer.SetInput(laplacian.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(0)
#viewer.DebugOn()


WindowLevelInterface(viewer)
