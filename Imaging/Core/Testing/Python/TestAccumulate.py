#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkPNGReader
from vtkmodules.vtkImagingCore import (
    vtkImageAppendComponents,
    vtkImageClip,
)
from vtkmodules.vtkImagingGeneral import vtkImageGaussianSmooth
from vtkmodules.vtkImagingStatistics import vtkImageAccumulate
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtkPNGReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/fullhead15.png")
smooth = vtkImageGaussianSmooth()
smooth.SetDimensionality(2)
smooth.SetStandardDeviations(1,1)
smooth.SetInputConnection(reader.GetOutputPort())
imageAppend = vtkImageAppendComponents()
imageAppend.AddInputConnection(reader.GetOutputPort())
imageAppend.AddInputConnection(smooth.GetOutputPort())
clip = vtkImageClip()
clip.SetInputConnection(imageAppend.GetOutputPort())
clip.SetOutputWholeExtent(0,255,0,255,20,22)
accum = vtkImageAccumulate()
accum.SetInputConnection(clip.GetOutputPort())
accum.SetComponentExtent(0,255,0,255,0,0)
accum.SetComponentSpacing(12,12,0.0)
viewer = vtkImageViewer()
viewer.SetInputConnection(accum.GetOutputPort())
viewer.SetColorWindow(4)
viewer.SetColorLevel(2)
viewer.Render()
# --- end of script --
