#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkPNGReader
from vtkmodules.vtkImagingCore import (
    vtkImageBinaryThreshold,
    vtkImageCast,
)
from vtkmodules.vtkImagingGeneral import vtkImageEuclideanDistance
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script shows how to use vtkImageEuclideanDistance
# Image pipeline
reader = vtkPNGReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/fullhead15.png")
cast = vtkImageCast()
cast.SetOutputScalarTypeToShort()
cast.SetInputConnection(reader.GetOutputPort())
thresh = vtkImageBinaryThreshold()
thresh.SetInputConnection(cast.GetOutputPort())
thresh.SetThresholdFunction(vtkImageBinaryThreshold.THRESHOLD_UPPER)
thresh.SetLowerThreshold(2000.0)
thresh.SetInValue(0)
thresh.SetOutValue(200)
thresh.ReplaceInOn()
thresh.ReplaceOutOn()
thresh.ReleaseDataFlagOff()
dist = vtkImageEuclideanDistance()
dist.SetInputConnection(thresh.GetOutputPort())
dist.SetAlgorithmToSaitoCached()
viewer = vtkImageViewer()
viewer.SetInputConnection(dist.GetOutputPort())
viewer.SetColorWindow(117)
viewer.SetColorLevel(43)
viewer.Render()
#skipping source
# --- end of script --
