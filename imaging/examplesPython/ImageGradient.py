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

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

cast = vtkImageCast()
cast.SetInput(reader.GetOutput())
cast.SetOutputScalarTypeToFloat()

magnify = vtkImageMagnify()
magnify.SetInput(cast.GetOutput())
magnify.SetMagnificationFactors(2,2,1)
magnify.InterpolateOn()

# remove high freqeuncy artifacts due to linear interpolation
smooth = vtkImageGaussianSmooth()
smooth.SetInput(magnify.GetOutput())
smooth.SetDimensionality(2)
smooth.SetStandardDeviations(1.5,1.5,0)
smooth.SetRadiusFactors(2.01,2.01,0)

gradient = vtkImageGradient()
gradient.SetInput(smooth.GetOutput())
gradient.SetDimensionality(2)

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
viewer.SetInput(rgb.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127)

#make interface
WindowLevelInterface(viewer)
