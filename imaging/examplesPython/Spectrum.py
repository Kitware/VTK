#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *

# This scripts shows a compressed spectrum of an image.


# Image pipeline

reader = vtkImageReader()
reader.GetOutput().ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix("../../../vtkdata/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.DebugOn()

fft = vtkImageFFT()
fft.SetFilteredAxes(VTK_IMAGE_X_AXIS,VTK_IMAGE_Y_AXIS)
fft.SetInput(reader.GetOutput())
fft.ReleaseDataFlagOff()
#fft.DebugOn()

magnitude = vtkImageMagnitude()
magnitude.SetInput(fft.GetOutput())
magnitude.ReleaseDataFlagOff()

center = vtkImageFourierCenter()
center.SetInput(magnitude.GetOutput())
center.SetFilteredAxes(VTK_IMAGE_X_AXIS,VTK_IMAGE_Y_AXIS)

compress = vtkImageLogarithmicScale()
compress.SetInput(center.GetOutput())
compress.SetConstant(15)

viewer = vtkImageViewer()
viewer.SetInput(compress.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(150)
viewer.SetColorLevel(170)

WindowLevelInterface(viewer)
