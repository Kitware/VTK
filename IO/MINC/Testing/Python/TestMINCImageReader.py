#!/usr/bin/env python

# Image pipeline
createReader = vtk.vtkImageReader2Factory()
reader = createReader.CreateImageReader2("" + str(VTK_DATA_ROOT) + "/Data/t3_grid_0.mnc")
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/t3_grid_0.mnc")
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(reader.GetOutputPort())
viewer.SetColorWindow(65535)
viewer.SetColorLevel(0)
#make interface
viewer.Render()
reader.UnRegister(viewer) # not needed in python
# --- end of script --
