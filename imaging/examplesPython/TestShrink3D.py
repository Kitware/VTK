#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# Halves the size of the image in the x, Y and Z dimensions.



# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

shrink = vtkImageShrink3D()
shrink.SetInput(reader.GetOutput())
shrink.SetShrinkFactors(2,2,2)
#shrink.Update()
shrink.SetNumberOfThreads(1)

viewer = vtkImageViewer()
viewer.SetInput(shrink.GetOutput())
viewer.SetZSlice(11)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)


# make interface
WindowLevelInterface(viewer)
