#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkPNGReader
from vtkmodules.vtkImagingGeneral import vtkImageGradient
from vtkmodules.vtkImagingMath import vtkImageDivergence
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Divergence measures rate of change of gradient.
# Image pipeline
reader = vtkPNGReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/fullhead15.png")
gradient = vtkImageGradient()
gradient.SetDimensionality(2)
gradient.SetInputConnection(reader.GetOutputPort())
derivative = vtkImageDivergence()
derivative.SetInputConnection(gradient.GetOutputPort())
viewer = vtkImageViewer()
viewer.SetInputConnection(derivative.GetOutputPort())
viewer.SetColorWindow(1000)
viewer.SetColorLevel(0)
viewer.Render()
# --- end of script --
