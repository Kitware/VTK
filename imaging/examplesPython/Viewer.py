#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *

# Simple viewer for images.



# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix("../../../vtkdata/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.DebugOn()
#reader.Update()


viewer = vtkImageViewer()
viewer.SetInput(reader.GetOutput())
viewer.SetZSlice(14)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
#viewer.DebugOn()
viewer.Render()

viewer.SetPosition(50,50)

#make interface
WindowLevelInterface(viewer)
