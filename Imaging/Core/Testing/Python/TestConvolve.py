#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Show the constant kernel.  Smooth an impulse function.
s1 = vtk.vtkImageCanvasSource2D()
s1.SetScalarTypeToFloat()
s1.SetExtent(0,255,0,255,0,0)
s1.SetDrawColor(0)
s1.FillBox(0,255,0,255)
s1.SetDrawColor(1.0)
s1.FillBox(75,175,75,175)
convolve = vtk.vtkImageConvolve()
convolve.SetInputConnection(s1.GetOutputPort())
convolve.SetKernel5x5([1,1,1,1,1,5,4,3,2,1,5,4,3,2,1,5,4,3,2,1,1,1,1,1,1])
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(convolve.GetOutputPort())
viewer.SetColorWindow(18)
viewer.SetColorLevel(9)
viewer.Render()
# --- end of script --
