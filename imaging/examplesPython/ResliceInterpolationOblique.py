#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from vtkpython import *
from WindowLevelInterface import *
import os

# Image pipeline

reader = vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetDataSpacing(1,1,2)
reader.SetFilePrefix(os.path.join(VTK_DATA, "fullHead/headsq"))
reader.SetDataMask(0x7fff)
reader.Update()

transform = vtkTransform()
# rotate about the center of the image
transform.Translate(+127.5,+127.5,+92.0)
transform.RotateWXYZ(10,1,1,0)
transform.Translate(-127.5,-127.5,-92.0)

reslice1 = vtkImageReslice()
reslice1.SetInput(reader.GetOutput())
reslice1.SetResliceTransform(transform)
reslice1.SetInterpolationModeToCubic()
reslice1.SetOutputSpacing(0.2,0.2,0.2)
reslice1.SetOutputOrigin(100,150,51) 
reslice1.SetOutputExtent(0,255,0,255,0,0)

reslice2 = vtkImageReslice()
reslice2.SetInput(reader.GetOutput())
reslice2.SetResliceTransform(transform)
reslice2.SetInterpolationModeToLinear()
reslice2.SetOutputSpacing(0.2,0.2,0.2)
reslice2.SetOutputOrigin(100,150,51) 
reslice2.SetOutputExtent(0,255,0,255,0,0)

reslice3 = vtkImageReslice()
reslice3.SetInput(reader.GetOutput())
reslice3.SetResliceTransform(transform)
reslice3.SetInterpolationModeToNearestNeighbor()
reslice3.SetOutputSpacing(0.2,0.2,0.2)
reslice3.SetOutputOrigin(100,150,51) 
reslice3.SetOutputExtent(0,255,0,255,0,0)

reslice4 = vtkImageReslice()
reslice4.SetInput(reader.GetOutput())
reslice4.SetResliceTransform(transform)
reslice4.SetInterpolationModeToLinear()
reslice4.SetOutputSpacing(1.0,1.0,1.0)
reslice4.SetOutputOrigin(0,0,50) 
reslice4.SetOutputExtent(0,255,0,255,0,0)

mapper1 = vtkImageMapper()
mapper1.SetInput(reslice1.GetOutput())
mapper1.SetColorWindow(2000)
mapper1.SetColorLevel(1000)
mapper1.SetZSlice(0)
#mapper1.DebugOn()

mapper2 = vtkImageMapper()
mapper2.SetInput(reslice2.GetOutput())
mapper2.SetColorWindow(2000)
mapper2.SetColorLevel(1000)
mapper2.SetZSlice(0) 
#mapper2.DebugOn()

mapper3 = vtkImageMapper()
mapper3.SetInput(reslice3.GetOutput())
mapper3.SetColorWindow(2000)
mapper3.SetColorLevel(1000)
mapper3.SetZSlice(0) 
#mapper3.DebugOn()

mapper4 = vtkImageMapper()
mapper4.SetInput(reslice4.GetOutput())
mapper4.SetColorWindow(2000)
mapper4.SetColorLevel(1000)
mapper4.SetZSlice(0) 
#mapper4.DebugOn()

actor1 = vtkActor2D()
actor1.SetMapper(mapper1)
#actor1.DebugOn()

actor2 = vtkActor2D()
actor2.SetMapper(mapper2)
#actor2.DebugOn()

actor3 = vtkActor2D()
actor3.SetMapper(mapper3)
#actor3.DebugOn()

actor4 = vtkActor2D()
actor4.SetMapper(mapper4)
#actor4.DebugOn()

imager1 = vtkImager()
imager1.AddActor2D(actor1)
imager1.SetViewport(0.5,0.0,1.0,0.5)
#imager1.DebugOn()

imager2 = vtkImager()
imager2.AddActor2D(actor2)
imager2.SetViewport(0.0,0.0,0.5,0.5)
#imager2.DebugOn()

imager3 = vtkImager()
imager3.AddActor2D(actor3)
imager3.SetViewport(0.5,0.5,1.0,1.0)
#imager3.DebugOn()

imager4 = vtkImager()
imager4.AddActor2D(actor4)
imager4.SetViewport(0.0,0.5,0.5,1.0)
#imager4.DebugOn()


imgWin = vtkImageWindow()
imgWin.AddImager(imager1)
imgWin.AddImager(imager2)
imgWin.AddImager(imager3)
imgWin.AddImager(imager4)
imgWin.SetSize(512,512)
#imgWin.DebugOn()

imgWin.Render()

import sys
sys.stdin.read(1)




