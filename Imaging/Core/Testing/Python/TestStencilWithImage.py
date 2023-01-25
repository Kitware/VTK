#!/usr/bin/env python
from vtkmodules.vtkIOImage import (
    vtkBMPReader,
    vtkPNMReader,
)
from vtkmodules.vtkImagingCore import vtkImageTranslateExtent
from vtkmodules.vtkImagingStencil import (
    vtkImageStencil,
    vtkImageToImageStencil,
)
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# A script to test the stencil filter by using one image
# to stencil another
# Image pipeline
reader1 = vtkBMPReader()
reader1.SetFileName(VTK_DATA_ROOT + "/Data/masonry.bmp")
reader2 = vtkPNMReader()
reader2.SetFileName(VTK_DATA_ROOT + "/Data/B.pgm")
translate = vtkImageTranslateExtent()
translate.SetInputConnection(reader2.GetOutputPort())
translate.SetTranslation(60,60,0)
imageToStencil = vtkImageToImageStencil()
imageToStencil.SetInputConnection(translate.GetOutputPort())
imageToStencil.ThresholdBetween(0,127)
# silly stuff to increase coverage
imageToStencil.SetUpperThreshold(imageToStencil.GetUpperThreshold())
imageToStencil.SetLowerThreshold(imageToStencil.GetLowerThreshold())
stencil = vtkImageStencil()
stencil.SetInputConnection(reader1.GetOutputPort())
stencil.SetBackgroundValue(0)
stencil.ReverseStencilOn()
stencil.SetStencilConnection(imageToStencil.GetOutputPort())
viewer = vtkImageViewer()
viewer.SetInputConnection(stencil.GetOutputPort())
viewer.SetColorWindow(255.0)
viewer.SetColorLevel(127.5)
viewer.SetZSlice(0)
viewer.Render()
# --- end of script --
