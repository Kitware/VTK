#!/usr/local/bin/python

from vtkpython import *
from WindowLevelInterface import *

# A script to test the Arithmetic filter.
# An image is smoothed then sbutracted from the original image.
# The result is a high-pass filter.



# Image pipeline

reader = vtkImageReader()
#reader.DebugOn()
reader.GetOutput().ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix("../../../vtkdata/fullHead/headsq")
reader.SetDataMask(0x7fff)

cast = vtkImageCast()
cast.SetInput(reader.GetOutput())
cast.SetOutputScalarTypeToFloat()

shiftScale = vtkImageShiftScale()
shiftScale.SetInput(cast.GetOutput())
shiftScale.SetShift(1.0)

log = vtkImageMathematics()
log.SetOperationToLog()
log.SetInput1(shiftScale.GetOutput())
log.ReleaseDataFlagOff()

viewer = vtkImageViewer()
#viewer.DebugOn()
viewer.SetInput(log.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(4)
viewer.SetColorLevel(6)
viewer.Render()


# make interface
WindowLevelInterface(viewer)
