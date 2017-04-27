#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# this test is designed to check the operation of the 8bit
# export of BMPs
# Image pipeline
reader = vtk.vtkBMPReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/masonry.bmp")
reader.SetAllow8BitBMP(1)
map = vtk.vtkImageMapToColors()
map.SetInputConnection(reader.GetOutputPort())
map.SetLookupTable(reader.GetLookupTable())
map.SetOutputFormatToRGB()
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(map.GetOutputPort())
viewer.SetColorWindow(256)
viewer.SetColorLevel(127.5)
#make interface
viewer.Render()
# --- end of script --
