#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkImageReader
from vtkmodules.vtkImagingCore import vtkImageResample
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Doubles the number of images (x dimension).
# Image pipeline
reader = vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetDataSpacing(3.2,3.2,1.5)
reader.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
magnify = vtkImageResample()
magnify.SetDimensionality(3)
magnify.SetInputConnection(reader.GetOutputPort())
magnify.SetAxisOutputSpacing(0,0.52)
magnify.SetAxisOutputSpacing(1,2.2)
magnify.SetAxisOutputSpacing(2,0.8)
magnify.ReleaseDataFlagOff()
viewer = vtkImageViewer()
viewer.SetInputConnection(magnify.GetOutputPort())
viewer.SetZSlice(30)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.Render()
# --- end of script --
