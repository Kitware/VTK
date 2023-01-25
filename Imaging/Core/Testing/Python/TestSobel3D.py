#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkImageReader
from vtkmodules.vtkImagingGeneral import vtkImageSobel3D
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# This script is for testing the 3D Sobel filter.
# Displays the 3 components using color.
# Image pipeline
reader = vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetDataSpacing(3.2,3.2,1.5)
reader.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
sobel = vtkImageSobel3D()
sobel.SetInputConnection(reader.GetOutputPort())
sobel.ReleaseDataFlagOff()
viewer = vtkImageViewer()
#viewer DebugOn
viewer.SetInputConnection(sobel.GetOutputPort())
viewer.SetZSlice(22)
viewer.SetColorWindow(400)
viewer.SetColorLevel(0)
viewer.Render()
# --- end of script --
