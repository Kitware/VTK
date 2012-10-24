#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtk.vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
range = vtk.vtkImageRange3D()
range.SetInputConnection(reader.GetOutputPort())
range.SetKernelSize(5,5,5)
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(range.GetOutputPort())
viewer.SetZSlice(22)
viewer.SetColorWindow(1000)
viewer.SetColorLevel(500)
#viewer DebugOn
viewer.Render()
# --- end of script --
