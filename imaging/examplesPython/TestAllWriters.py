#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# This script reads an image and writes it in several formats

# Image pipeline

image = vtkBMPReader()
image.SetFileName(VTK_DATA + "/beach.bmp")

luminance = vtkImageLuminance()
luminance.SetInput(image.GetOutput())

tiff1 = vtkTIFFWriter()
tiff1.SetInput(image.GetOutput())
tiff1.SetFileName("tiff1.tif")

tiff2 = vtkTIFFWriter()
tiff2.SetInput(luminance.GetOutput())
tiff2.SetFileName("tiff2.tif")

bmp1 = vtkBMPWriter()
bmp1.SetInput(image.GetOutput())
bmp1.SetFileName("bmp1.bmp")

bmp2 = vtkBMPWriter()
bmp2.SetInput(luminance.GetOutput())
bmp2.SetFileName("bmp2.bmp")

tiff1.Write()
tiff2.Write()
bmp1.Write()
bmp2.Write()

viewer = vtkImageViewer()
viewer.SetInput(luminance.GetOutput())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
viewer.Render()

#make interface
WindowLevelInterface(viewer)

windowToimage = vtkWindowToImageFilter()
windowToimage.SetInput(viewer.GetImageWindow())
pnmWriter = vtkPNMWriter()
pnmWriter.SetInput(windowToimage.GetOutput())
pnmWriter.SetFileName("TestAllWriters.tcl.ppm")
#pnmWriter.Write()
