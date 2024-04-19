#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkDICOMImageReader
from vtkmodules.vtkImagingGeneral import vtkImageSobel2D
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script is for testing the 3D Sobel filter.
# Displays the 3 components using color.
# Image pipeline
reader = vtkDICOMImageReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/mr.001")
sobel = vtkImageSobel2D()
sobel.SetInputConnection(reader.GetOutputPort())
sobel.ReleaseDataFlagOff()
viewer = vtkImageViewer()
viewer.SetInputConnection(sobel.GetOutputPort())
viewer.SetColorWindow(400)
viewer.SetColorLevel(0)
viewer.Render()
# --- end of script --
