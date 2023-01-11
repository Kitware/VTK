#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkLookupTable
from vtkmodules.vtkCommonDataModel import vtkSphere
from vtkmodules.vtkIOImage import (
    vtkBMPReader,
    vtkPNMReader,
)
from vtkmodules.vtkImagingCore import (
    vtkImageBlend,
    vtkImageTranslateExtent,
)
from vtkmodules.vtkImagingStencil import vtkImplicitFunctionToImageStencil
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# do alpha-blending of two images, but also clip with stencil
reader1 = vtkBMPReader()
reader1.SetFileName(VTK_DATA_ROOT + "/Data/masonry.bmp")

reader2 = vtkPNMReader()
reader2.SetFileName(VTK_DATA_ROOT + "/Data/B.pgm")

table = vtkLookupTable()
table.SetTableRange(0, 127)
table.SetValueRange(0.0, 1.0)
table.SetSaturationRange(0.0, 0.0)
table.SetHueRange(0.0, 0.0)
table.SetAlphaRange(0.9, 0.0)
table.Build()

translate = vtkImageTranslateExtent()
translate.SetInputConnection(reader2.GetOutputPort())
translate.SetTranslation(60, 60, 0)

sphere = vtkSphere()
sphere.SetCenter(121, 131, 0)
sphere.SetRadius(70)

functionToStencil = vtkImplicitFunctionToImageStencil()
functionToStencil.SetInput(sphere)

blend = vtkImageBlend()
blend.SetInputConnection(reader1.GetOutputPort())
blend.AddInputConnection(translate.GetOutputPort())

# exercise the ReplaceNthInputConnection method
blend.ReplaceNthInputConnection(1, reader1.GetOutputPort())
blend.ReplaceNthInputConnection(1, translate.GetOutputPort())
blend.SetOpacity(1, 0.8)
blend.SetStencilConnection(functionToStencil.GetOutputPort())

viewer = vtkImageViewer()
viewer.SetInputConnection(blend.GetOutputPort())
viewer.SetColorWindow(255.0)
viewer.SetColorLevel(127.5)
viewer.SetZSlice(0)
viewer.Render()
