#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


import sys
from vtkpython import *
from WindowLevelInterface import *

reader = vtkTIFFReader()
reader.SetFileName(VTK_DATA + "/testTIFF.tif")
#reader.SetFileName(sys.argv[1])

viewer = vtkImageViewer()
#viewer.DebugOn()
viewer.SetInput(reader.GetOutput())
viewer.SetColorWindow(256)
viewer.SetColorLevel(127.5)

#make interface
WindowLevelInterface(viewer)
