#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
image1 = vtk.vtkImageCanvasSource2D()
image1.SetNumberOfScalarComponents(3)
image1.SetScalarTypeToUnsignedChar()
image1.SetExtent(0,511,0,511,0,0)
image1.SetDrawColor(255,255,0)
image1.FillBox(0,511,0,511)
pad1 = vtk.vtkImageWrapPad()
pad1.SetInputConnection(image1.GetOutputPort())
pad1.SetOutputWholeExtent(0,511,0,511,0,99)
pad1.Update()
image2 = vtk.vtkImageCanvasSource2D()
image2.SetNumberOfScalarComponents(3)
image2.SetScalarTypeToUnsignedChar()
image2.SetExtent(0,511,0,511,0,0)
image2.SetDrawColor(0,255,255)
image2.FillBox(0,511,0,511)
pad2 = vtk.vtkImageWrapPad()
pad2.SetInputConnection(image2.GetOutputPort())
pad2.SetOutputWholeExtent(0,511,0,511,0,99)
pad2.Update()
checkers = vtk.vtkImageCheckerboard()
checkers.SetInput1Data(pad1.GetOutput())
checkers.SetInput2Data(pad2.GetOutput())
checkers.SetNumberOfDivisions(11,6,2)
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(checkers.GetOutputPort())
viewer.SetZSlice(49)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
viewer.Render()
# --- end of script --
