#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Tst the OpenClose3D filter.
# Image pipeline
reader = vtk.vtkPNGReader()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
thresh = vtk.vtkImageThreshold()
thresh.SetInputConnection(reader.GetOutputPort())
thresh.SetOutputScalarTypeToUnsignedChar()
thresh.ThresholdByUpper(2000.0)
thresh.SetInValue(255)
thresh.SetOutValue(0)
thresh.ReleaseDataFlagOff()
my_close = vtk.vtkImageOpenClose3D()
my_close.SetInputConnection(thresh.GetOutputPort())
my_close.SetOpenValue(0)
my_close.SetCloseValue(255)
my_close.SetKernelSize(5,5,3)
my_close.ReleaseDataFlagOff()
# for coverage (we could compare results to see if they are correct).
my_close.DebugOn()
my_close.DebugOff()
my_close.GetOutput()
my_close.GetCloseValue()
my_close.GetOpenValue()
#my_close AddObserver ProgressEvent {set pro [my_close GetProgress]; puts "Completed $pro"; flush stdout}
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(my_close.GetOutputPort())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
viewer.Render()
# --- end of script --
