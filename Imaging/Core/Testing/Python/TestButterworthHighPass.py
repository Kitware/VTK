#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script shows the result of an ideal highpass filter in frequency space.
# Image pipeline
reader = vtk.vtkPNGReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
fft = vtk.vtkImageFFT()
fft.SetDimensionality(2)
fft.SetInputConnection(reader.GetOutputPort())
#fft DebugOn
highPass = vtk.vtkImageButterworthHighPass()
highPass.SetInputConnection(fft.GetOutputPort())
highPass.SetOrder(2)
highPass.SetXCutOff(0.2)
highPass.SetYCutOff(0.1)
highPass.ReleaseDataFlagOff()
#highPass DebugOn
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(highPass.GetOutputPort())
viewer.SetColorWindow(10000)
viewer.SetColorLevel(5000)
#viewer DebugOn
viewer.Render()
# --- end of script --
