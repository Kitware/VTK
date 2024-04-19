#!/usr/bin/env python
from vtkmodules.vtkIOImage import vtkImageReader
from vtkmodules.vtkImagingCore import (
    vtkImageCast,
    vtkImageMagnify,
    vtkImageThreshold,
)
from vtkmodules.vtkImagingMath import vtkImageWeightedSum
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
reader.SetDataExtent(0, 63, 0, 63, 1, 93)
reader.SetFilePrefix(VTK_DATA_ROOT + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)

mag = vtkImageMagnify()
mag.SetInputConnection(reader.GetOutputPort())
mag.SetMagnificationFactors(4, 4, 1)

th = vtkImageThreshold()
th.SetInputConnection(mag.GetOutputPort())
th.SetReplaceIn(1)
th.SetReplaceOut(1)
th.ThresholdBetween(-1000, 1000)
th.SetOutValue(0)
th.SetInValue(2000)

cast = vtkImageCast()
cast.SetInputConnection(mag.GetOutputPort())
cast.SetOutputScalarTypeToFloat()

cast2 = vtkImageCast()
cast2.SetInputConnection(th.GetOutputPort())
cast2.SetOutputScalarTypeToFloat()

sum = vtkImageWeightedSum()
sum.AddInputConnection(cast.GetOutputPort())
sum.AddInputConnection(cast2.GetOutputPort())
sum.SetWeight(0, 10)
sum.SetWeight(1, 4)

viewer = vtkImageViewer()
viewer.SetInputConnection(sum.GetOutputPort())
viewer.SetZSlice(22)
viewer.SetColorWindow(1819)
viewer.SetColorLevel(939)
sum.SetWeight(0, 1)
# make interface
viewer.SetSize(256, 256)
viewer.Render()
