#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


import signal
from vtkpython import *



# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.DebugOn()
reader.Update()

magnify = vtkImageMagnify()
magnify.SetInput(reader.GetOutput())
magnify.SetMagnificationFactors(2,2,1)
#magnify.InterpolateOn()

mapper1 = vtkImageMapper()
mapper1.SetInput(magnify.GetOutput())
mapper1.SetColorWindow(2000)
mapper1.SetColorLevel(1000)
mapper1.SetZSlice(20)
#mapper1.DebugOn()

mapper2 = vtkImageMapper()
mapper2.SetInput(reader.GetOutput())
mapper2.SetColorWindow(2000)
mapper2.SetColorLevel(1000)
mapper2.SetZSlice(50)
#mapper2.DebugOn()

mapper3 = vtkImageMapper()
mapper3.SetInput(reader.GetOutput())
mapper3.SetColorWindow(2000)
mapper3.SetColorLevel(1000)
mapper3.SetZSlice(70)
#mapper3.DebugOn()

mapper4 = vtkImageMapper()
mapper4.SetInput(reader.GetOutput())
mapper4.SetColorWindow(2000)
mapper4.SetColorLevel(1000)
mapper4.SetZSlice(90)
#mapper4.DebugOn()

mapper5 = vtkImageMapper()
mapper5.SetInput(reader.GetOutput())
mapper5.SetColorWindow(2000)
mapper5.SetColorLevel(1000)
mapper5.SetZSlice(90)
#mapper5.DebugOn()

mapper6 = vtkImageMapper()
mapper6.SetInput(reader.GetOutput())
mapper6.SetColorWindow(2000)
mapper6.SetColorLevel(1000)
mapper6.SetZSlice(90)
#mapper6.DebugOn()

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

actor5 = vtkActor2D()
actor5.SetMapper(mapper5)
#actor5.DebugOn()

actor6 = vtkActor2D()
actor6.SetMapper(mapper6)
#actor6.DebugOn()

imager1 = vtkImager()
imager1.AddActor2D(actor1)
#imager1.SetViewport(0.0,0.66,0.33,1.0)
imager1.SetViewport(0.0,0.33,0.66,1.0)
#imager1.DebugOn()

imager2 = vtkImager()
imager2.AddActor2D(actor2)
#imager2.SetViewport(0.0,0.33,0.0,0.33)
imager2.SetViewport(0.0,0.0,0.33,0.33)
#imager2.DebugOn()

imager3 = vtkImager()
imager3.AddActor2D(actor3)
#imager3.SetViewport(0.33,0.66,0.0,0.33)
imager3.SetViewport(0.33,0.0,0.66,0.33)
#imager3.DebugOn()

imager4 = vtkImager()
imager4.AddActor2D(actor4)
#imager4.SetViewport(0.66,1.0,0.0,0.33)
imager4.SetViewport(0.66,0.0,1.0,0.33)
#imager4.DebugOn()

imager5 = vtkImager()
imager5.AddActor2D(actor5)
#imager5 SetViewport(0.66,1.0,0.33,0.66)
imager5.SetViewport(0.66,0.33,1.0,0.66)
#imager5.DebugOn()

imager6 = vtkImager()
imager6.AddActor2D(actor6)
#imager6.SetViewport(0.66,1.0,0.66,1.0)
imager6.SetViewport(0.66,0.66,1.0,1.0)
#imager6.DebugOn()



imgWin = vtkImageWindow()
imgWin.AddImager(imager1)
imgWin.AddImager(imager2)
imgWin.AddImager(imager3)
imgWin.AddImager(imager4)
imgWin.AddImager(imager5)
imgWin.AddImager(imager6)
imgWin.SetSize(512,512)
#imgWin.DebugOn()

imgWin.Render()

signal.pause()
