#!/usr/bin/env python
from vtkmodules.vtkIOImage import (
    vtkBMPReader,
    vtkPNGReader,
)
from vtkmodules.vtkImagingCore import (
    vtkImageCast,
    vtkImageShiftScale,
)
from vtkmodules.vtkImagingGeneral import vtkImageGradient
from vtkmodules.vtkImagingMath import vtkImageDotProduct
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script shows the magnitude of an image in frequency domain.
# Image pipeline
reader = vtkPNGReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/fullhead15.png")
cast = vtkImageCast()
cast.SetInputConnection(reader.GetOutputPort())
cast.SetOutputScalarTypeToFloat()
scale2 = vtkImageShiftScale()
scale2.SetInputConnection(cast.GetOutputPort())
scale2.SetScale(0.05)
gradient = vtkImageGradient()
gradient.SetInputConnection(scale2.GetOutputPort())
gradient.SetDimensionality(3)
gradient.Update()
pnm = vtkBMPReader()
pnm.SetFileName(VTK_DATA_ROOT + "/Data/masonry.bmp")
cast2 = vtkImageCast()
cast2.SetInputConnection(pnm.GetOutputPort())
cast2.SetOutputScalarTypeToDouble()
cast2.Update()
magnitude = vtkImageDotProduct()
magnitude.SetInput1Data(cast2.GetOutput())
magnitude.SetInput2Data(gradient.GetOutput())
#vtkImageViewer viewer
viewer = vtkImageViewer()
viewer.SetInputConnection(magnitude.GetOutputPort())
viewer.SetColorWindow(1000)
viewer.SetColorLevel(300)
#viewer DebugOn
viewer.Render()
# --- end of script --
