#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This scripts shows a compressed spectrum of an image.
# Image pipeline
reader = vtk.vtkMetaImageReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/foot/foot.mha")

viewer = vtk.vtkImageViewer2()
viewer.SetInputConnection(reader.GetOutputPort())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)

viewInt = vtk.vtkRenderWindowInteractor()
viewer.SetupInteractor(viewInt)
viewer.Render()

# This is needed if you want to interact with the image.
# viewInt.Start()
