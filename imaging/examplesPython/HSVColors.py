#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# This example uses a programable source to create a ramp.
# This is converted into a volume (256x256x256) with 3 components.
# Each axis ramps independently.
# It is then converted into a color volume with Hue, 
# Saturation and Value ramping

rampSource = vtkProgrammableSource()

# Generate a single ramp value
def ramp():
  newScalars = vtkScalars()
  newScalars.SetNumberOfScalars(256)

  #.Compute.points.and.scalars()
  for idx in range(0,256):
    newScalars.SetScalar(idx,idx)

  rampSource.GetStructuredPointsOutput().SetDimensions(256,1,1)
  rampSource.GetStructuredPointsOutput().GetPointData().SetScalars(newScalars)

  newScalars.Delete() #reference.counting - it's ok 
 
rampSource.SetExecuteMethod(ramp)

# use pad filter to create a volume
pad = vtkImageWrapPad()
pad.SetInput(rampSource.GetStructuredPointsOutput())
pad.SetOutputWholeExtent(0,255,0,255,0,255)
pad.ReleaseDataFlagOn()


# hack work around of bug
copy1 = vtkImageShiftScale()
copy1.SetInput(pad.GetOutput())
copy2 = vtkImageShiftScale()
copy2.SetInput(pad.GetOutput())
copy3 = vtkImageShiftScale()
copy3.SetInput(pad.GetOutput())



# create HSV components
perm1 = vtkImagePermute()
perm1.SetInput(copy1.GetOutput())
#perm1.SetInput(pad.GetOutput())
perm1.SetFilteredAxes(VTK_IMAGE_Y_AXIS,VTK_IMAGE_Z_AXIS,VTK_IMAGE_X_AXIS)
perm1.ReleaseDataFlagOn()
 
perm2 = vtkImagePermute()
perm2.SetInput(copy2.GetOutput())
#perm2.SetInput(pad.GetOutput())
perm2.SetFilteredAxes(VTK_IMAGE_Z_AXIS,VTK_IMAGE_X_AXIS,VTK_IMAGE_Y_AXIS)
perm2.ReleaseDataFlagOn()

append1 = vtkImageAppendComponents()
append1.SetInput1(copy3.GetOutput())
#append1.SetInput1(pad.GetOutput())
append1.SetInput2(perm1.GetOutput())
append1.ReleaseDataFlagOn()

append2 = vtkImageAppendComponents()
append2.SetInput1(append1.GetOutput())
append2.SetInput2(perm2.GetOutput())


rgb = vtkImageHSVToRGB()
rgb.SetInput(append2.GetOutput())
rgb.SetMaximum(255)

viewer = vtkImageViewer()
viewer.SetInput(rgb.GetOutput())
viewer.SetZSlice(128)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)

# make interface
WindowLevelInterface(viewer)
