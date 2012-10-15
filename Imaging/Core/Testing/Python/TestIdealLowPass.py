#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script shows the result of an ideal lowpass filter in frequency space.
# Image pipeline
reader = vtk.vtkPNGReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
fft = vtk.vtkImageFFT()
fft.SetInputConnection(reader.GetOutputPort())
#fft DebugOn
lowPass = vtk.vtkImageIdealLowPass()
lowPass.SetInputConnection(fft.GetOutputPort())
lowPass.SetXCutOff(0.2)
lowPass.SetYCutOff(0.1)
lowPass.ReleaseDataFlagOff()
#lowPass DebugOn
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(lowPass.GetOutputPort())
viewer.SetColorWindow(10000)
viewer.SetColorLevel(5000)
#viewer DebugOn
viewer.Render()
# --- end of script --
