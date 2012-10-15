#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script is for testing the normalize filter.
# Image pipeline
cos = vtk.vtkImageSinusoidSource()
cos.SetWholeExtent(0,225,0,225,0,20)
cos.SetAmplitude(250)
cos.SetDirection(1,1,1)
cos.SetPeriod(20)
cos.ReleaseDataFlagOff()
gradient = vtk.vtkImageGradient()
gradient.SetInputConnection(cos.GetOutputPort())
gradient.SetDimensionality(3)
norm = vtk.vtkImageNormalize()
norm.SetInputConnection(gradient.GetOutputPort())
viewer = vtk.vtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(norm.GetOutputPort())
viewer.SetZSlice(10)
viewer.SetColorWindow(2.0)
viewer.SetColorLevel(0)
viewer.Render()
# --- end of script --
