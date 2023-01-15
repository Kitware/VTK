#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkPNGReader
from vtkmodules.vtkImagingCore import vtkImageCast
from vtkmodules.vtkImagingGeneral import vtkImageLaplacian
from vtkmodules.vtkImagingMath import vtkImageMathematics
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script subtracts the 2D laplacian from an image to enhance the edges.
# Image pipeline
reader = vtkPNGReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/fullhead15.png")
cast = vtkImageCast()
cast.SetInputConnection(reader.GetOutputPort())
cast.SetOutputScalarTypeToDouble()
cast.Update()
lap = vtkImageLaplacian()
lap.SetInputConnection(cast.GetOutputPort())
lap.SetDimensionality(2)
lap.Update()
subtract = vtkImageMathematics()
subtract.SetOperationToSubtract()
subtract.SetInput1Data(cast.GetOutput())
subtract.SetInput2Data(lap.GetOutput())
subtract.ReleaseDataFlagOff()
#subtract BypassOn
viewer = vtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(subtract.GetOutputPort())
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.Render()
# --- end of script --
