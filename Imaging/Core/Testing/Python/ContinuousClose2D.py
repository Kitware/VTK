#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtk.vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
dilate = vtk.vtkImageContinuousDilate3D()
dilate.SetInputConnection(reader.GetOutputPort())
dilate.SetKernelSize(11,11,1)
erode = vtk.vtkImageContinuousErode3D()
erode.SetInputConnection(dilate.GetOutputPort())
erode.SetKernelSize(11,11,1)
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(erode.GetOutputPort())
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.Render()
# --- end of script --
