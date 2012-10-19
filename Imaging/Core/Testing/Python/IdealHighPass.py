#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script shows the result of an ideal highpass filter in  spatial domain
# Image pipeline
createReader = vtk.vtkImageReader2Factory()
reader = createReader.CreateImageReader2("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
fft = vtk.vtkImageFFT()
fft.SetInputConnection(reader.GetOutputPort())
highPass = vtk.vtkImageIdealHighPass()
highPass.SetInputConnection(fft.GetOutputPort())
highPass.SetXCutOff(0.1)
highPass.SetYCutOff(0.1)
highPass.ReleaseDataFlagOff()
rfft = vtk.vtkImageRFFT()
rfft.SetInputConnection(highPass.GetOutputPort())
real = vtk.vtkImageExtractComponents()
real.SetInputConnection(rfft.GetOutputPort())
real.SetComponents(0)
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(real.GetOutputPort())
viewer.SetColorWindow(500)
viewer.SetColorLevel(0)
viewer.Render()
reader.UnRegister(viewer) # not needed in python
# --- end of script --
