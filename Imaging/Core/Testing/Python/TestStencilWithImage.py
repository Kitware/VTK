#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# A script to test the stencil filter by using one image
# to stencil another
# Image pipeline
reader1 = vtk.vtkBMPReader()
reader1.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/masonry.bmp")
reader2 = vtk.vtkPNMReader()
reader2.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/B.pgm")
translate = vtk.vtkImageTranslateExtent()
translate.SetInputConnection(reader2.GetOutputPort())
translate.SetTranslation(60,60,0)
imageToStencil = vtk.vtkImageToImageStencil()
imageToStencil.SetInputConnection(translate.GetOutputPort())
imageToStencil.ThresholdBetween(0,127)
# silly stuff to increase coverage
imageToStencil.SetUpperThreshold(imageToStencil.GetUpperThreshold())
imageToStencil.SetLowerThreshold(imageToStencil.GetLowerThreshold())
stencil = vtk.vtkImageStencil()
stencil.SetInputConnection(reader1.GetOutputPort())
stencil.SetBackgroundValue(0)
stencil.ReverseStencilOn()
stencil.SetStencilConnection(imageToStencil.GetOutputPort())
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(stencil.GetOutputPort())
viewer.SetColorWindow(255.0)
viewer.SetColorLevel(127.5)
viewer.SetZSlice(0)
viewer.Render()
# --- end of script --
