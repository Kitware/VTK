#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# A script to test the mask filter.
#  replaces a circle with a color


# Image pipeline
reader = vtkPNMReader()
reader.ReleaseDataFlagOff()
reader.SetFileName(VTK_DATA + "/earth.ppm")

sphere = vtkImageEllipsoidSource()
sphere.SetWholeExtent(0,511,0,255,0,0)
sphere.SetCenter(128,128,0)
sphere.SetRadius(80,80,1)

mask = vtkImageMask()
mask.SetImageInput(reader.GetOutput())
mask.SetMaskInput(sphere.GetOutput())
mask.SetMaskedOutputValue(100,128,200)
mask.NotMaskOn()
mask.ReleaseDataFlagOff()

sphere2 = vtkImageEllipsoidSource()
sphere2.SetWholeExtent(0,511,0,255,0,0)
sphere2.SetCenter(328,128,0)
sphere2.SetRadius(80,50,1)

# Test the wrapping of the output masked value
mask2 = vtkImageMask()
mask2.SetImageInput(mask.GetOutput())
mask2.SetMaskInput(sphere2.GetOutput())
mask2.SetMaskedOutputValue(100)
mask2.NotMaskOn()
mask2.ReleaseDataFlagOff()

viewer = vtkImageViewer()
viewer.SetInput(mask2.GetOutput())
viewer.SetColorWindow(255)
viewer.SetColorLevel(128)
#viewer.DebugOn()

# make interface
WindowLevelInterface(viewer)
