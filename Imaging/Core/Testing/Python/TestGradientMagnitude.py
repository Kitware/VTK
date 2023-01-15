#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkGESignaReader
from vtkmodules.vtkImagingGeneral import vtkImageGradientMagnitude
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtkGESignaReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/E07733S002I009.MR")
gradient = vtkImageGradientMagnitude()
gradient.SetDimensionality(2)
gradient.SetInputConnection(reader.GetOutputPort())
viewer = vtkImageViewer()
viewer.SetInputConnection(gradient.GetOutputPort())
viewer.SetColorWindow(250)
viewer.SetColorLevel(125)
viewer.Render()
# --- end of script --
