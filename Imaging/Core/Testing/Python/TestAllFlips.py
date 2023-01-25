#!/usr/bin/env python
from vtkmodules.vtkFiltersCore import vtkImageAppend
from vtkmodules.vtkIOImage import vtkImageReader
from vtkmodules.vtkImagingCore import (
    vtkImageCast,
    vtkImageFlip,
)
from vtkmodules.vtkInteractionImage import vtkImageViewer
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2
from vtkmodules.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtkImageReader()
reader.GetExecutive().SetReleaseDataFlag(0,0)
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
imageFloat = vtkImageCast()
imageFloat.SetInputConnection(reader.GetOutputPort())
imageFloat.SetOutputScalarTypeToFloat()
flipX = vtkImageFlip()
flipX.SetInputConnection(imageFloat.GetOutputPort())
flipX.SetFilteredAxis(0)
flipY = vtkImageFlip()
flipY.SetInputConnection(imageFloat.GetOutputPort())
flipY.SetFilteredAxis(1)
flipY.FlipAboutOriginOn()
imageAppend = vtkImageAppend()
imageAppend.AddInputConnection(imageFloat.GetOutputPort())
imageAppend.AddInputConnection(flipX.GetOutputPort())
imageAppend.AddInputConnection(flipY.GetOutputPort())
imageAppend.SetAppendAxis(0)
viewer = vtkImageViewer()
viewer.SetInputConnection(imageAppend.GetOutputPort())
viewer.SetZSlice(22)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
#make interface
viewer.Render()
# --- end of script --
