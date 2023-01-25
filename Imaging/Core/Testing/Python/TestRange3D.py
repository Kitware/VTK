#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkImageReader
from vtkmodules.vtkImagingGeneral import vtkImageRange3D
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
range = vtkImageRange3D()
range.SetInputConnection(reader.GetOutputPort())
range.SetKernelSize(5,5,5)
viewer = vtkImageViewer()
viewer.SetInputConnection(range.GetOutputPort())
viewer.SetZSlice(22)
viewer.SetColorWindow(1000)
viewer.SetColorLevel(500)
#viewer DebugOn
viewer.Render()
# --- end of script --
