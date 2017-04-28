#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This scripts shows a compressed spectrum of an image.
# Image pipeline
reader = vtk.vtkPNGReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
fft = vtk.vtkImageFFT()
fft.SetInputConnection(reader.GetOutputPort())
fft.ReleaseDataFlagOff()
#fft DebugOn
magnitude = vtk.vtkImageMagnitude()
magnitude.SetInputConnection(fft.GetOutputPort())
magnitude.ReleaseDataFlagOff()
center = vtk.vtkImageFourierCenter()
center.SetInputConnection(magnitude.GetOutputPort())
compress = vtk.vtkImageLogarithmicScale()
compress.SetInputConnection(center.GetOutputPort())
compress.SetConstant(15)
viewer = vtk.vtkImageViewer2()
viewer.SetInputConnection(compress.GetOutputPort())
viewer.SetColorWindow(150)
viewer.SetColorLevel(170)
viewInt = vtk.vtkRenderWindowInteractor()
viewer.SetupInteractor(viewInt)
viewer.Render()
# --- end of script --
