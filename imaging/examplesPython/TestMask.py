#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# A script to test the mask filter.
# removes all but a sphere of headSq.


# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,94)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

sphere = vtkImageEllipsoidSource()
sphere.SetWholeExtent(0,255,0,255,1,94)
sphere.SetCenter(128,128,46)
sphere.SetRadius(80,80,80)

mask = vtkImageMask()
mask.SetImageInput(reader.GetOutput())
mask.SetMaskInput(sphere.GetOutput())
mask.SetMaskedOutputValue(500)
mask.NotMaskOn()
mask.ReleaseDataFlagOff()

viewer = vtkImageViewer()
viewer.SetInput(mask.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
#viewer.DebugOn()

# make interface
WindowLevelInterface(viewer)
