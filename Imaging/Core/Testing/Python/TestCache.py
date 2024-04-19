#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkImageReader
from vtkmodules.vtkImagingCore import vtkImageCacheFilter
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetDataSpacing(3.2,3.2,1.5)
reader.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
cache = vtkImageCacheFilter()
cache.SetInputConnection(reader.GetOutputPort())
cache.SetCacheSize(20)
viewer = vtkImageViewer()
viewer.SetInputConnection(cache.GetOutputPort())
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.SetPosition(50,50)
i = 0
while i < 5:
    j = 10
    while j < 30:
        viewer.SetZSlice(j)
        viewer.Render()
        j = j + 1

    i = i + 1

#make interface
viewer.Render()
# --- end of script --
