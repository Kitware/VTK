#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtk.vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetDataSpacing(3.2,3.2,1.5)
reader.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
var = vtk.vtkImageVariance3D()
var.SetInputConnection(reader.GetOutputPort())
var.SetKernelSize(3,3,1)
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(var.GetOutputPort())
viewer.SetZSlice(22)
viewer.SetColorWindow(3000)
viewer.SetColorLevel(1000)
#viewer DebugOn
viewer.Render()
# --- end of script --
