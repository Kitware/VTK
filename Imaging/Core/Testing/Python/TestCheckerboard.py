#!/usr/bin/env python
from vtkmodules.vtkImagingCore import vtkImageWrapPad
from vtkmodules.vtkImagingGeneral import vtkImageCheckerboard
from vtkmodules.vtkImagingSources import vtkImageCanvasSource2D
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
image1 = vtkImageCanvasSource2D()
image1.SetNumberOfScalarComponents(3)
image1.SetScalarTypeToUnsignedChar()
image1.SetExtent(0,511,0,511,0,0)
image1.SetDrawColor(255,255,0)
image1.FillBox(0,511,0,511)
pad1 = vtkImageWrapPad()
pad1.SetInputConnection(image1.GetOutputPort())
pad1.SetOutputWholeExtent(0,511,0,511,0,10)
pad1.Update()
image2 = vtkImageCanvasSource2D()
image2.SetNumberOfScalarComponents(3)
image2.SetScalarTypeToUnsignedChar()
image2.SetExtent(0,511,0,511,0,0)
image2.SetDrawColor(0,255,255)
image2.FillBox(0,511,0,511)
pad2 = vtkImageWrapPad()
pad2.SetInputConnection(image2.GetOutputPort())
pad2.SetOutputWholeExtent(0,511,0,511,0,10)
pad2.Update()
checkers = vtkImageCheckerboard()
checkers.SetInput1Data(pad1.GetOutput())
checkers.SetInput2Data(pad2.GetOutput())
checkers.SetNumberOfDivisions(11,6,0)
viewer = vtkImageViewer()
viewer.SetInputConnection(checkers.GetOutputPort())
viewer.SetZSlice(9)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
viewer.Render()
# --- end of script --
