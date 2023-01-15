#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkPNGReader
from vtkmodules.vtkImagingFourier import (
    vtkImageFFT,
    vtkImageFourierCenter,
)
from vtkmodules.vtkImagingMath import (
    vtkImageLogarithmicScale,
    vtkImageMagnitude,
)
from vtkmodules.vtkInteractionImage import vtkImageViewer2
from vtkmodules.vtkRenderingCore import vtkRenderWindowInteractor
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This scripts shows a compressed spectrum of an image.
# Image pipeline
reader = vtkPNGReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/fullhead15.png")
fft = vtkImageFFT()
fft.SetInputConnection(reader.GetOutputPort())
fft.ReleaseDataFlagOff()
#fft DebugOn
magnitude = vtkImageMagnitude()
magnitude.SetInputConnection(fft.GetOutputPort())
magnitude.ReleaseDataFlagOff()
center = vtkImageFourierCenter()
center.SetInputConnection(magnitude.GetOutputPort())
compress = vtkImageLogarithmicScale()
compress.SetInputConnection(center.GetOutputPort())
compress.SetConstant(15)
viewer = vtkImageViewer2()
viewer.SetInputConnection(compress.GetOutputPort())
viewer.SetColorWindow(150)
viewer.SetColorLevel(170)
viewInt = vtkRenderWindowInteractor()
viewer.SetupInteractor(viewInt)
viewer.Render()
# --- end of script --
