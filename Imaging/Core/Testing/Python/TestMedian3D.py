#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Test the median filter.
# Image pipeline
reader = vtk.vtkPNGReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
median = vtk.vtkImageMedian3D()
median.SetInputConnection(reader.GetOutputPort())
median.SetKernelSize(7,7,1)
median.ReleaseDataFlagOff()
viewer = vtk.vtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(median.GetOutputPort())
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.Render()
# --- end of script --
