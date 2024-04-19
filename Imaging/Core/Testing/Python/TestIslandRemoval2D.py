#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkPNGReader
from vtkmodules.vtkImagingCore import vtkImageThreshold
from vtkmodules.vtkImagingMorphological import vtkImageIslandRemoval2D
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# A script to test the island removal filter.
# first the image is thresholded, then small islands are removed.
# Image pipeline
reader = vtkPNGReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/fullhead15.png")
thresh = vtkImageThreshold()
thresh.SetInputConnection(reader.GetOutputPort())
thresh.ThresholdByUpper(2000.0)
thresh.SetInValue(255)
thresh.SetOutValue(0)
thresh.ReleaseDataFlagOff()
island = vtkImageIslandRemoval2D()
island.SetInputConnection(thresh.GetOutputPort())
island.SetIslandValue(255)
island.SetReplaceValue(128)
island.SetAreaThreshold(100)
island.SquareNeighborhoodOn()
viewer = vtkImageViewer()
viewer.SetInputConnection(island.GetOutputPort())
viewer.SetColorWindow(255)
viewer.SetColorLevel(127.5)
#viewer DebugOn
viewer.Render()
# --- end of script --
