#!/usr/local/bin/python

import sys
from vtkpython import *
from WindowLevelInterface import *

reader = vtkTIFFReader()
reader.SetFileName("../../../vtkdata/testTIFF.tif")
#reader.SetFileName(sys.argv[1])

viewer = vtkImageViewer()
#viewer.DebugOn()
viewer.SetInput(reader.GetOutput())
viewer.SetColorWindow(256)
viewer.SetColorLevel(127.5)

#make interface
WindowLevelInterface(viewer)
