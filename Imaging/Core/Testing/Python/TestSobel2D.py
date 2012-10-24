#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script is for testing the 3D Sobel filter.
# Displays the 3 components using color.
# Image pipeline
reader = vtk.vtkDICOMImageReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/mr.001")
sobel = vtk.vtkImageSobel2D()
sobel.SetInputConnection(reader.GetOutputPort())
sobel.ReleaseDataFlagOff()
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(sobel.GetOutputPort())
viewer.SetColorWindow(400)
viewer.SetColorLevel(0)
viewer.Render()
# --- end of script --
