#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *
import os

# In this example, an image is centered at (0,0,0) before a
# rotation is applied to ensure that the rotation occurs about
# the center of the image.

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetDataSpacing(1.0,1.0,2.0)
reader.SetFilePrefix(os.path.join(VTK_DATA,"fullHead/headsq"))
reader.SetDataMask(0x7fff)

# first center the image at (0,0,0)
information = vtkImageChangeInformation()
information.SetInput(reader.GetOutput())
information.CenterImageOn()

# apply a rotation about (0,0,0)
trans = vtkTransform()
trans.RotateZ(30)

reslice = vtkImageReslice()
reslice.SetInput(information.GetOutput())
reslice.SetResliceTransform(trans)
reslice.SetInterpolationModeToCubic()

# reset the image back to the way it was (you don't have
# to do this, it is just put in as an example)
information2 = vtkImageChangeInformation()
information2.SetInput(reslice.GetOutput())
information2.SetInformationInput(reader.GetOutput())

viewer = vtkImageViewer()
viewer.SetInput(information2.GetOutput())
viewer.GetImageWindow().DoubleBufferOn()
viewer.SetZSlice(14)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
#viewer.Render()

#make interface
WindowLevelInterface(viewer)


