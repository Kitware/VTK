#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkImageReader
from vtkmodules.vtkImagingCore import vtkImagePermute
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Simple viewer for images.
# Image pipeline
reader = vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
permute = vtkImagePermute()
permute.SetInputConnection(reader.GetOutputPort())
permute.SetFilteredAxes(1,2,0)
viewer = vtkImageViewer()
viewer.SetInputConnection(permute.GetOutputPort())
viewer.SetZSlice(32)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
#viewer DebugOn
#viewer Render
#make interface
viewer.Render()
# --- end of script --
