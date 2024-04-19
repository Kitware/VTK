#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkPNGReader
from vtkmodules.vtkImagingCore import (
    vtkImageChangeInformation,
    vtkImageClip,
)
from vtkmodules.vtkImagingGeneral import vtkImageGradientMagnitude
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtkPNGReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/fullhead15.png")
clip = vtkImageClip()
clip.SetInputConnection(reader.GetOutputPort())
clip.SetOutputWholeExtent(80,230,80,230,0,0)
clip.ClipDataOff()
gradient = vtkImageGradientMagnitude()
gradient.SetDimensionality(2)
gradient.SetInputConnection(clip.GetOutputPort())
gradient.HandleBoundariesOff()
slide = vtkImageChangeInformation()
slide.SetInputConnection(gradient.GetOutputPort())
slide.SetExtentTranslation(-100,-100,0)
viewer = vtkImageViewer()
viewer.SetInputConnection(slide.GetOutputPort())
viewer.SetColorWindow(-1000)
viewer.SetColorLevel(500)
viewer.SetSize(150,150)
viewer.Render()
#skipping source
# --- end of script --
