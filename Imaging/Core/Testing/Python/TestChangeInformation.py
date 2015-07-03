#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# In this example, an image is centered at (0,0,0) before a
# rotation is applied to ensure that the rotation occurs about
# the center of the image.
reader = vtk.vtkPNGReader()
reader.SetDataSpacing(0.8,0.8,1.5)
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/fullhead15.png")
# first center the image at (0,0,0)
information = vtk.vtkImageChangeInformation()
information.SetInputConnection(reader.GetOutputPort())
information.CenterImageOn()
reslice = vtk.vtkImageReslice()
reslice.SetInputConnection(information.GetOutputPort())
reslice.SetResliceAxesDirectionCosines([0.866025,-0.5,0,0.5,0.866025,0,0,0,1])
reslice.SetInterpolationModeToCubic()
# reset the image back to the way it was (you don't have
# to do this, it is just put in as an example)
information2 = vtk.vtkImageChangeInformation()
information2.SetInputConnection(reslice.GetOutputPort())
reader.Update()
information2.SetInformationInputData(reader.GetOutput())
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(information2.GetOutputPort())
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.Render()
# --- end of script --
