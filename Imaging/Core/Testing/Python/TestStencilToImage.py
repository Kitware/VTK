#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkSphere
from vtkmodules.vtkImagingStencil import (
    vtkImageStencilToImage,
    vtkImplicitFunctionToImageStencil,
)
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# A script to test converting a stencil to a binary image
sphere = vtkSphere()
sphere.SetCenter(128,128,0)
sphere.SetRadius(80)
functionToStencil = vtkImplicitFunctionToImageStencil()
functionToStencil.SetInput(sphere)
functionToStencil.SetOutputOrigin(0,0,0)
functionToStencil.SetOutputSpacing(1,1,1)
functionToStencil.SetOutputWholeExtent(0,255,0,255,0,0)
stencilToImage = vtkImageStencilToImage()
stencilToImage.SetInputConnection(functionToStencil.GetOutputPort())
stencilToImage.SetOutsideValue(0)
stencilToImage.SetInsideValue(255)
viewer = vtkImageViewer()
viewer.SetInputConnection(stencilToImage.GetOutputPort())
viewer.SetZSlice(0)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
viewer.Render()
# --- end of script --
