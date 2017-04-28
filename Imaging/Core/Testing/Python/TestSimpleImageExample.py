#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtk.vtkPNGReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
gradient = vtk.vtkSimpleImageFilterExample()
gradient.SetInputConnection(reader.GetOutputPort())
viewer = vtk.vtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(gradient.GetOutputPort())
viewer.SetColorWindow(1000)
viewer.SetColorLevel(500)
viewer.Render()
# --- end of script --
