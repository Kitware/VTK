#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *


# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix("../../../vtkdata/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader DebugOn

diffusion = vtkImageAnisotropicDiffusion2D()
diffusion.SetInput(reader.GetOutput())
diffusion.SetDiffusionFactor(1.0)
diffusion.SetDiffusionThreshold(200.0)
diffusion.SetNumberOfIterations(5)
#diffusion.DebugOn()

viewer = vtkImageViewer()
#viewer.DebugOn()
viewer.SetInput(diffusion.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(3000)
viewer.SetColorLevel(1500)


WindowLevelInterface(viewer)
