#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

reader = vtk.vtkBMPReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/masonry.bmp")
# set the window/level
viewer = vtk.vtkImageViewer2()
viewer.SetInputConnection(reader.GetOutputPort())
viewer.SetColorWindow(100.0)
viewer.SetColorLevel(127.5)
viewer.Render()
# --- end of script --
