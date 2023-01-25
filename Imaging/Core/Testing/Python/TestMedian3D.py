#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkPNGReader
from vtkmodules.vtkImagingGeneral import vtkImageMedian3D
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Test the median filter.
# Image pipeline
reader = vtkPNGReader()
reader.SetFileName(VTK_DATA_ROOT + "/Data/fullhead15.png")
median = vtkImageMedian3D()
median.SetInputConnection(reader.GetOutputPort())
median.SetKernelSize(7,7,1)
median.ReleaseDataFlagOff()
viewer = vtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(median.GetOutputPort())
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.Render()
# --- end of script --
