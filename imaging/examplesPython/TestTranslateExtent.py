#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# Simple viewer for images.


# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.DebugOn()
#reader.Update()

translate = vtkImageTranslateExtent()
translate.SetInput(reader.GetOutput())
translate.SetTranslation(10,10,10)




viewer = vtkImageViewer()
viewer.SetInput(translate.GetOutput())
viewer.SetZSlice(14)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.Render()

#make interface
WindowLevelInterface(viewer)
