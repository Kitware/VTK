#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# This Script test the euclidean to polar by coverting 2D vectors 
# from a gradient into polar, which is converted into HSV, and then to RGB.

# Image pipeline

reader = vtkImageReader()
#reader.DebugOn()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

ellipsoid = vtkImageEllipsoidSource()
ellipsoid.SetWholeExtent(0,255,0,255,0,44)
ellipsoid.SetCenter(127,127,22)
ellipsoid.SetRadius(100,100,100)
ellipsoid.SetOutValue(0)
ellipsoid.SetInValue(200)
ellipsoid.SetOutputScalarType(VTK_FLOAT)

gauss = vtkImageGaussianSource()
gauss.SetWholeExtent(0,255,0,255,0,44)
gauss.SetCenter(127,127,22)
gauss.SetStandardDeviation(50.0)
gauss.SetMaximum(8000.0)

gradient = vtkImageGradient()
gradient.SetInput(reader.GetOutput())
#gradient.SetInput(ellipsoid.GetOutput())
#gradient.SetInput(gauss.GetOutput())
gradient.SetDimensionality(2)
gradient.ReleaseDataFlagOff()

polar = vtkImageEuclideanToPolar()
polar.SetInput(gradient.GetOutput())
polar.SetThetaMaximum(255)

pad = vtkImageConstantPad()
pad.SetInput(polar.GetOutput())
pad.SetOutputNumberOfScalarComponents(3)
pad.SetConstant(200)

# permute components so saturation will be constant
permute = vtkImageExtractComponents()
permute.SetInput(pad.GetOutput())
permute.SetComponents(0,2,1)

rgb = vtkImageHSVToRGB()
rgb.SetInput(permute.GetOutput())
rgb.SetMaximum(255)

viewer = vtkImageViewer()
#viewer.DebugOn()
viewer.SetInput(rgb.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
viewer.Render()

wtoif = vtkWindowToImageFilter()
wtoif.SetInput(viewer.GetImageWindow())

psWriter = vtkPostScriptWriter()
psWriter.SetInput(wtoif.GetOutput())
psWriter.SetFileName("junk.ps")
psWriter.Write()

#make interface
WindowLevelInterface(viewer)
