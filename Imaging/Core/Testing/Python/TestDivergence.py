#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Divergence measures rate of change of gradient.
# Image pipeline
reader = vtk.vtkPNGReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
gradient = vtk.vtkImageGradient()
gradient.SetDimensionality(2)
gradient.SetInputConnection(reader.GetOutputPort())
derivative = vtk.vtkImageDivergence()
derivative.SetInputConnection(gradient.GetOutputPort())
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(derivative.GetOutputPort())
viewer.SetColorWindow(1000)
viewer.SetColorLevel(0)
viewer.Render()
# --- end of script --
