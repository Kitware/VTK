#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Simple viewer for images.
# Image pipeline
reader = vtk.vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
permute = vtk.vtkImagePermute()
permute.SetInputConnection(reader.GetOutputPort())
permute.SetFilteredAxes(1,2,0)
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(permute.GetOutputPort())
viewer.SetZSlice(32)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
#viewer DebugOn
#viewer Render
#make interface
viewer.Render()
# --- end of script --
