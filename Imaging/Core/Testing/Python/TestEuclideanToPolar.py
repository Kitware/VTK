#!/usr/bin/env python
from vtkmodules.vtkImagingGeneral import (
    vtkImageEuclideanToPolar,
    vtkImageGradient,
)
from vtkmodules.vtkImagingSources import vtkImageGaussianSource
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This Script test the euclidean to polar by converting 2D vectors
# from a gradient into polar, which is converted into HSV, and then to RGB.
# Image pipeline
gauss = vtkImageGaussianSource()
gauss.SetWholeExtent(0,255,0,255,0,44)
gauss.SetCenter(127,127,22)
gauss.SetStandardDeviation(50.0)
gauss.SetMaximum(10000.0)
gradient = vtkImageGradient()
gradient.SetInputConnection(gauss.GetOutputPort())
gradient.SetDimensionality(2)
polar = vtkImageEuclideanToPolar()
polar.SetInputConnection(gradient.GetOutputPort())
polar.SetThetaMaximum(255)
viewer = vtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(polar.GetOutputPort())
viewer.SetZSlice(22)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
#make interface
viewer.Render()
# --- end of script --
