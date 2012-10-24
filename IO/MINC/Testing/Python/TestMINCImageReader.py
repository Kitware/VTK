#!/usr/bin/env python
import vtk

# Image pipeline
reader = vtk.vtkMINCImageReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/t3_grid_0.mnc")

viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(reader.GetOutputPort())
viewer.SetColorWindow(65535)
viewer.SetColorLevel(0)

#make interface
viewer.Render()
