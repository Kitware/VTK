#!/usr/bin/env python
from vtkmodules.vtkImagingCore import (
    vtkImageConstantPad,
    vtkImageExtractComponents,
)
from vtkmodules.vtkImagingFourier import (
    vtkImageFFT,
    vtkImageRFFT,
)
from vtkmodules.vtkImagingMath import vtkImageMathematics
from vtkmodules.vtkImagingSources import vtkImageCanvasSource2D
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Performs a correlation in frequency domain.
s1 = vtkImageCanvasSource2D()
s1.SetScalarTypeToFloat()
s1.SetExtent(0,255,0,255,0,0)
s1.SetDrawColor(0)
s1.FillBox(0,255,0,255)
s1.SetDrawColor(2.0)
s1.FillTriangle(10,100,190,150,40,250)
s2 = vtkImageCanvasSource2D()
s2.SetScalarTypeToFloat()
s2.SetExtent(0,31,0,31,0,0)
s2.SetDrawColor(0.0)
s2.FillBox(0,31,0,31)
s2.SetDrawColor(2.0)
s2.FillTriangle(10,1,25,10,1,5)
fft1 = vtkImageFFT()
fft1.SetDimensionality(2)
fft1.SetInputConnection(s1.GetOutputPort())
fft1.ReleaseDataFlagOff()
fft1.Update()
# Pad kernel out to same size as image.
pad2 = vtkImageConstantPad()
pad2.SetInputConnection(s2.GetOutputPort())
pad2.SetOutputWholeExtent(0,255,0,255,0,0)
fft2 = vtkImageFFT()
fft2.SetDimensionality(2)
fft2.SetInputConnection(pad2.GetOutputPort())
fft2.ReleaseDataFlagOff()
fft2.Update()
# conjugate is necessary for correlation (not convolution)
conj = vtkImageMathematics()
conj.SetOperationToConjugate()
conj.SetInput1Data(fft2.GetOutput())
conj.Update()
# Correlation is multiplication in frequency space.
mult = vtkImageMathematics()
mult.SetOperationToComplexMultiply()
mult.SetInput1Data(fft1.GetOutput())
mult.SetInput2Data(conj.GetOutput())
rfft = vtkImageRFFT()
rfft.SetDimensionality(2)
rfft.SetInputConnection(mult.GetOutputPort())
real = vtkImageExtractComponents()
real.SetInputConnection(rfft.GetOutputPort())
real.SetComponents(0)
viewer = vtkImageViewer()
viewer.SetInputConnection(real.GetOutputPort())
viewer.SetColorWindow(256)
viewer.SetColorLevel(127.5)
# make interface
#skipping source
# --- end of script --
