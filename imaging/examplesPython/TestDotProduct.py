#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# This script shows the magnitude of an image in frequency domain.



# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,22,22)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.DebugOn()



cast = vtkImageCast()
cast.SetInput(reader.GetOutput())
cast.SetOutputScalarType(VTK_FLOAT)

scale2 = vtkImageShiftScale()
scale2.SetInput(cast.GetOutput())
scale2.SetScale(0.05)

gradient = vtkImageGradient()
gradient.SetInput(scale2.GetOutput())
gradient.SetDimensionality(3)

pnm = vtkPNMReader()
pnm.SetFileName(VTK_DATA + "/masonry.ppm")

cast2 = vtkImageCast()
cast2.SetInput(pnm.GetOutput())
cast2.SetOutputScalarType(VTK_FLOAT)

magnitude = vtkImageDotProduct()
magnitude.SetInput1(cast2.GetOutput())
magnitude.SetInput2(gradient.GetOutput())

viewer = vtkImageViewer()
viewer.SetInput(magnitude.GetOutput())
viewer.SetColorWindow(1000)
viewer.SetColorLevel(300)
#viewer.DebugOn()

# make interface
WindowLevelInterface(viewer)
