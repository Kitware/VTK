#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *


# Image pipeline

reader = vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

smooth = vtkImageGaussianSmooth()
smooth.SetDimensionality(2)
smooth.SetStandardDeviations(1,1)
smooth.SetInput(reader.GetOutput())

append = vtkImageAppendComponents()
append.SetInput1(reader.GetOutput())
append.SetInput2(smooth.GetOutput())

clip = vtkImageClip()
clip.SetInput(append.GetOutput())
clip.SetOutputWholeExtent(0,255,0,255,20,22)

accum = vtkImageAccumulate()
accum.SetInput(clip.GetOutput())
accum.SetComponentExtent(0,512,0,512,0,0)
accum.SetComponentSpacing(6,6,0.0)


viewer = vtkImageViewer()
viewer.SetInput(accum.GetOutput())
#viewer.SetZSlice(22)
viewer.SetColorWindow(4)
viewer.SetColorLevel(2)


WindowLevelInterface(viewer)
