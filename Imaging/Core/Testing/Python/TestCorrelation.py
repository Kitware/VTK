#!/usr/bin/env python
from vtkmodules.vtkImagingGeneral import vtkImageCorrelation
from vtkmodules.vtkImagingSources import vtkImageCanvasSource2D
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Show the constant kernel.  Smooth an impulse function.
s1 = vtkImageCanvasSource2D()
s1.SetScalarTypeToFloat()
s1.SetExtent(0,255,0,255,0,0)
s1.SetDrawColor(0)
s1.FillBox(0,255,0,255)
s1.SetDrawColor(2.0)
s1.FillTriangle(10,100,190,150,40,250)
s1.Update()
s2 = vtkImageCanvasSource2D()
s2.SetScalarTypeToFloat()
s2.SetExtent(0,31,0,31,0,0)
s2.SetDrawColor(0.0)
s2.FillBox(0,31,0,31)
s2.SetDrawColor(2.0)
s2.FillTriangle(10,1,25,10,1,5)
s2.Update()
convolve = vtkImageCorrelation()
convolve.SetDimensionality(2)
convolve.SetInput1Data(s1.GetOutput())
convolve.SetInput2Data(s2.GetOutput())
viewer = vtkImageViewer()
viewer.SetInputConnection(convolve.GetOutputPort())
viewer.SetColorWindow(256)
viewer.SetColorLevel(127.5)
viewer.Render()
# --- end of script --
