#!/usr/bin/env python
from vtkmodules.vtkCommonDataModel import vtkSphere
from vtkmodules.vtkIOImage import vtkPNGReader
from vtkmodules.vtkImagingCore import vtkImageShiftScale
from vtkmodules.vtkImagingStencil import (
    vtkImageStencil,
    vtkImplicitFunctionToImageStencil,
)
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# A script to test the stencil filter.
# removes all but a sphere.
reader = vtkPNGReader()
reader.SetDataSpacing(0.8,0.8,1.5)
reader.SetDataOrigin(0.0,0.0,0.0)
reader.SetFileName(VTK_DATA_ROOT + "/Data/fullhead15.png")
reader.Update()
sphere = vtkSphere()
sphere.SetCenter(128,128,0)
sphere.SetRadius(80)
functionToStencil = vtkImplicitFunctionToImageStencil()
functionToStencil.SetInput(sphere)
functionToStencil.SetInformationInput(reader.GetOutput())
functionToStencil.Update()
# test making a copying of the stencil (for coverage)
stencilOriginal = functionToStencil.GetOutput()
stencilCopy = stencilOriginal.NewInstance()
stencilCopy.DeepCopy(functionToStencil.GetOutput())
shiftScale = vtkImageShiftScale()
shiftScale.SetInputConnection(reader.GetOutputPort())
shiftScale.SetScale(0.2)
shiftScale.Update()
stencil = vtkImageStencil()
stencil.SetInputConnection(reader.GetOutputPort())
stencil.SetBackgroundInputData(shiftScale.GetOutput())
stencil.SetStencilData(stencilCopy)
stencilCopy.UnRegister(stencil) # not needed in python
viewer = vtkImageViewer()
viewer.SetInputConnection(stencil.GetOutputPort())
viewer.SetZSlice(0)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.Render()
# --- end of script --
