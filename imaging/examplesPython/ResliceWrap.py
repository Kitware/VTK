#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *
import os

# Simple viewer for images.


# Image pipeline

reader = vtkPNMReader()
reader.SetFileName(os.path.join(VTK_DATA,"masonry.ppm"))
reader.SetDataExtent(0,255,0,255,0,0)
reader.SetDataSpacing(1,1,1)
reader.SetDataOrigin(0,0,0)
reader.UpdateWholeExtent()

transform = vtkTransform()
transform.RotateZ(45)
transform.Scale(1.414,1.414,1.414)

reslice = vtkImageReslice()
reslice.SetInput(reader.GetOutput())
reslice.SetResliceTransform(transform)
reslice.InterpolateOn()
reslice.SetInterpolationModeToCubic()
reslice.WrapOn()
reslice.AutoCropOutputOn()

viewer = vtkImageViewer()
viewer.SetInput(reslice.GetOutput())
viewer.GetImageWindow().DoubleBufferOn()
viewer.SetZSlice(0)
viewer.SetColorWindow(255.0)
viewer.SetColorLevel(127.5)
#viewer.Render()

#make interface
WindowLevelInterface(viewer)



