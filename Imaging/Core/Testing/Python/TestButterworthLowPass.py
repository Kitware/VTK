#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkPNGReader
from vtkmodules.vtkImagingFourier import (
    vtkImageButterworthLowPass,
    vtkImageFFT,
)
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script shows the result of an ideal lowpass filter in frequency space.
# Image pipeline
reader = vtkPNGReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/fullhead15.png")
fft = vtkImageFFT()
fft.SetDimensionality(2)
fft.SetInputConnection(reader.GetOutputPort())
#fft DebugOn
lowPass = vtkImageButterworthLowPass()
lowPass.SetInputConnection(fft.GetOutputPort())
lowPass.SetOrder(2)
lowPass.SetXCutOff(0.2)
lowPass.SetYCutOff(0.1)
lowPass.ReleaseDataFlagOff()
#lowPass DebugOn
viewer = vtkImageViewer()
viewer.SetInputConnection(lowPass.GetOutputPort())
viewer.SetColorWindow(10000)
viewer.SetColorLevel(5000)
#viewer DebugOn
viewer.Render()
# --- end of script --
