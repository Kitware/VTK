#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script shows the magnitude of an image in frequency domain.
# Image pipeline
reader = vtk.vtkPNGReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
cast = vtk.vtkImageCast()
cast.SetInputConnection(reader.GetOutputPort())
cast.SetOutputScalarTypeToFloat()
scale2 = vtk.vtkImageShiftScale()
scale2.SetInputConnection(cast.GetOutputPort())
scale2.SetScale(0.05)
gradient = vtk.vtkImageGradient()
gradient.SetInputConnection(scale2.GetOutputPort())
gradient.SetDimensionality(3)
gradient.Update()
pnm = vtk.vtkBMPReader()
pnm.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/masonry.bmp")
cast2 = vtk.vtkImageCast()
cast2.SetInputConnection(pnm.GetOutputPort())
cast2.SetOutputScalarTypeToDouble()
cast2.Update()
magnitude = vtk.vtkImageDotProduct()
magnitude.SetInput1Data(cast2.GetOutput())
magnitude.SetInput2Data(gradient.GetOutput())
#vtkImageViewer viewer
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(magnitude.GetOutputPort())
viewer.SetColorWindow(1000)
viewer.SetColorLevel(300)
#viewer DebugOn
viewer.Render()
# --- end of script --
