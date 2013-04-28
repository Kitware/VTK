#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# do alpha-blending of two images, but also clip with stencil
reader1 = vtk.vtkBMPReader()
reader1.SetFileName(VTK_DATA_ROOT + "/Data/masonry.bmp")

reader2 = vtk.vtkPNMReader()
reader2.SetFileName(VTK_DATA_ROOT + "/Data/B.pgm")

table = vtk.vtkLookupTable()
table.SetTableRange(0, 127)
table.SetValueRange(0.0, 1.0)
table.SetSaturationRange(0.0, 0.0)
table.SetHueRange(0.0, 0.0)
table.SetAlphaRange(0.9, 0.0)
table.Build()

translate = vtk.vtkImageTranslateExtent()
translate.SetInputConnection(reader2.GetOutputPort())
translate.SetTranslation(60, 60, 0)

sphere = vtk.vtkSphere()
sphere.SetCenter(121, 131, 0)
sphere.SetRadius(70)

functionToStencil = vtk.vtkImplicitFunctionToImageStencil()
functionToStencil.SetInput(sphere)
functionToStencil.GetExecutive().SetUpdateExtent(0, 0, 255, 0, 255, 0, 0)

blend = vtk.vtkImageBlend()
blend.SetInputConnection(reader1.GetOutputPort())
blend.AddInputConnection(translate.GetOutputPort())

# excercise the ReplaceNthInputConnection method
blend.ReplaceNthInputConnection(1, reader1.GetOutputPort())
blend.ReplaceNthInputConnection(1, translate.GetOutputPort())
blend.SetOpacity(1, 0.8)
blend.SetStencilConnection(functionToStencil.GetOutputPort())

viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(blend.GetOutputPort())
viewer.SetColorWindow(255.0)
viewer.SetColorLevel(127.5)
viewer.SetZSlice(0)
viewer.Render()
