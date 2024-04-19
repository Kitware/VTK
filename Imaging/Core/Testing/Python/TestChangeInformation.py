#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkPNGReader
from vtkmodules.vtkImagingCore import (
    vtkImageChangeInformation,
    vtkImageReslice,
)
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# In this example, an image is centered at (0,0,0) before a
# rotation is applied to ensure that the rotation occurs about
# the center of the image.
reader = vtkPNGReader()
reader.SetDataSpacing(0.8,0.8,1.5)
reader.SetFileName(VTK_DATA_ROOT + "/Data/fullhead15.png")
# first center the image at (0,0,0)
information = vtkImageChangeInformation()
information.SetInputConnection(reader.GetOutputPort())
information.CenterImageOn()
reslice = vtkImageReslice()
reslice.SetInputConnection(information.GetOutputPort())
reslice.SetResliceAxesDirectionCosines([0.866025,-0.5,0,0.5,0.866025,0,0,0,1])
reslice.SetInterpolationModeToCubic()
# reset the image back to the way it was (you don't have
# to do this, it is just put in as an example)
information2 = vtkImageChangeInformation()
information2.SetInputConnection(reslice.GetOutputPort())
reader.Update()
information2.SetInformationInputData(reader.GetOutput())
viewer = vtkImageViewer()
viewer.SetInputConnection(information2.GetOutputPort())
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.Render()
# --- end of script --
