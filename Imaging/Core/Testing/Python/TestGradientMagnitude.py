#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtk.vtkGESignaReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/E07733S002I009.MR")
gradient = vtk.vtkImageGradientMagnitude()
gradient.SetDimensionality(2)
gradient.SetInputConnection(reader.GetOutputPort())
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(gradient.GetOutputPort())
viewer.SetColorWindow(250)
viewer.SetColorLevel(125)
viewer.Render()
# --- end of script --
