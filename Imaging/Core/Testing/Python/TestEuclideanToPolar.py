#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This Script test the euclidean to polar by coverting 2D vectors
# from a gradient into polar, which is converted into HSV, and then to RGB.
# Image pipeline
gauss = vtk.vtkImageGaussianSource()
gauss.SetWholeExtent(0,255,0,255,0,44)
gauss.SetCenter(127,127,22)
gauss.SetStandardDeviation(50.0)
gauss.SetMaximum(10000.0)
gradient = vtk.vtkImageGradient()
gradient.SetInputConnection(gauss.GetOutputPort())
gradient.SetDimensionality(2)
polar = vtk.vtkImageEuclideanToPolar()
polar.SetInputConnection(gradient.GetOutputPort())
polar.SetThetaMaximum(255)
viewer = vtk.vtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(polar.GetOutputPort())
viewer.SetZSlice(22)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
#make interface
viewer.Render()
# --- end of script --
