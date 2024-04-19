#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkImageReader2Factory
from vtkmodules.vtkImagingCore import vtkImageExtractComponents
from vtkmodules.vtkImagingFourier import (
    vtkImageFFT,
    vtkImageIdealHighPass,
    vtkImageRFFT,
)
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script shows the result of an ideal highpass filter in spatial domain
# Image pipeline
createReader = vtkImageReader2Factory()
reader = createReader.CreateImageReader2(VTK_DATA_ROOT + "/Data/fullhead15.png")
reader.SetFileName(VTK_DATA_ROOT + "/Data/fullhead15.png")
fft = vtkImageFFT()
fft.SetInputConnection(reader.GetOutputPort())
highPass = vtkImageIdealHighPass()
highPass.SetInputConnection(fft.GetOutputPort())
highPass.SetXCutOff(0.1)
highPass.SetYCutOff(0.1)
highPass.ReleaseDataFlagOff()
rfft = vtkImageRFFT()
rfft.SetInputConnection(highPass.GetOutputPort())
real = vtkImageExtractComponents()
real.SetInputConnection(rfft.GetOutputPort())
real.SetComponents(0)
viewer = vtkImageViewer()
viewer.SetInputConnection(real.GetOutputPort())
viewer.SetColorWindow(500)
viewer.SetColorLevel(0)
viewer.Render()
reader.UnRegister(viewer) # not needed in python
# --- end of script --
