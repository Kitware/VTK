#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *

# Image pipeline

reader = vtkPNMReader()
reader.SetFileName("../../../vtkdata/binary.pgm")

cast = vtkImageCast()
cast.SetInput(reader.GetOutput())
cast.SetOutputScalarTypeToShort()

dilate = vtkImageDilateErode3D()
dilate.SetInput(cast.GetOutput())
dilate.SetDilateValue(255)
dilate.SetErodeValue(0)
dilate.SetKernelSize(31,31,1)

erode = vtkImageDilateErode3D()
erode.SetInput(dilate.GetOutput())
erode.SetDilateValue(0)
erode.SetErodeValue(255)
erode.SetKernelSize(31,31,1)

add = vtkImageMathematics()
add.SetInput1(cast.GetOutput())
add.SetInput2(erode.GetOutput())
add.SetOperationToAdd()

viewer = vtkImageViewer()
#viewer.DebugOn()
viewer.SetInput(add.GetOutput())
viewer.SetColorWindow(512)
viewer.SetColorLevel(256)

# make interface
WindowLevelInterface(viewer)
