#!/usr/bin/env python
from vtkmodules.vtkImagingGeneral import (
    vtkImageGradient,
    vtkImageNormalize,
)
from vtkmodules.vtkImagingSources import vtkImageSinusoidSource
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script is for testing the normalize filter.
# Image pipeline
cos = vtkImageSinusoidSource()
cos.SetWholeExtent(0,225,0,225,0,20)
cos.SetAmplitude(250)
cos.SetDirection(1,1,1)
cos.SetPeriod(20)
cos.ReleaseDataFlagOff()
gradient = vtkImageGradient()
gradient.SetInputConnection(cos.GetOutputPort())
gradient.SetDimensionality(3)
norm = vtkImageNormalize()
norm.SetInputConnection(gradient.GetOutputPort())
viewer = vtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(norm.GetOutputPort())
viewer.SetZSlice(10)
viewer.SetColorWindow(2.0)
viewer.SetColorLevel(0)
viewer.Render()
# --- end of script --
