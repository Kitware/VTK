#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkImageReader
from vtkmodules.vtkImagingMorphological import (
    vtkImageContinuousDilate3D,
    vtkImageContinuousErode3D,
)
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
reader.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
dilate = vtkImageContinuousDilate3D()
dilate.SetInputConnection(reader.GetOutputPort())
dilate.SetKernelSize(11,11,1)
erode = vtkImageContinuousErode3D()
erode.SetInputConnection(dilate.GetOutputPort())
erode.SetKernelSize(11,11,1)
viewer = vtkImageViewer()
viewer.SetInputConnection(erode.GetOutputPort())
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.Render()
# --- end of script --
