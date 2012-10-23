#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
createReader = vtk.vtkImageReader2Factory()
reader = createReader.CreateImageReader2("" + str(VTK_DATA_ROOT) + "/Data/beach.jpg")
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/beach.jpg")
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(reader.GetOutputPort())
viewer.SetColorWindow(256)
viewer.SetColorLevel(127.5)
#make interface
viewer.Render()
reader.UnRegister(viewer) # not needed in python
# --- end of script --
