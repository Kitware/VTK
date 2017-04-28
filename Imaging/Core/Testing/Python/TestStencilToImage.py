#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# A script to test converting a stencil to a binary image
sphere = vtk.vtkSphere()
sphere.SetCenter(128,128,0)
sphere.SetRadius(80)
functionToStencil = vtk.vtkImplicitFunctionToImageStencil()
functionToStencil.SetInput(sphere)
functionToStencil.SetOutputOrigin(0,0,0)
functionToStencil.SetOutputSpacing(1,1,1)
functionToStencil.SetOutputWholeExtent(0,255,0,255,0,0)
stencilToImage = vtk.vtkImageStencilToImage()
stencilToImage.SetInputConnection(functionToStencil.GetOutputPort())
stencilToImage.SetOutsideValue(0)
stencilToImage.SetInsideValue(255)
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(stencilToImage.GetOutputPort())
viewer.SetZSlice(0)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
viewer.Render()
# --- end of script --
