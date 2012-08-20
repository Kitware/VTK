#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script subtracts the 2D laplacian from an image to enhance the edges.
# Image pipeline
reader = vtk.vtkPNGReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
cast = vtk.vtkImageCast()
cast.SetInputConnection(reader.GetOutputPort())
cast.SetOutputScalarTypeToDouble()
cast.Update()
lap = vtk.vtkImageLaplacian()
lap.SetInputConnection(cast.GetOutputPort())
lap.SetDimensionality(2)
lap.Update()
subtract = vtk.vtkImageMathematics()
subtract.SetOperationToSubtract()
subtract.SetInput1Data(cast.GetOutput())
subtract.SetInput2Data(lap.GetOutput())
subtract.ReleaseDataFlagOff()
#subtract BypassOn
viewer = vtk.vtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(subtract.GetOutputPort())
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.Render()
# --- end of script --
