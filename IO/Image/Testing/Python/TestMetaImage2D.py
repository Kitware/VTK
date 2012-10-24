#!/usr/bin/env python

# This scripts shows a compressed spectrum of an image.
# Image pipeline
reader = vtk.vtkMetaImageReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/foot/foot.mha")
viewer = vtk.vtkImageViewer2()
viewer.SetInputConnection(reader.GetOutputPort())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
viewInt = vtk.vtkRenderWindowInteractor()
viewer.SetupInteractor(viewInt)
viewer.Render()
# --- end of script --
