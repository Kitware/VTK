#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Shift and scale an image (in that order)
# This filter is useful for converting to a lower precision data type.
reader = vtk.vtkImageReader()
reader.GetExecutive().SetReleaseDataFlag(0,0)
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
shiftScale = vtk.vtkImageShiftScale()
shiftScale.SetInputConnection(reader.GetOutputPort())
shiftScale.SetShift(0)
shiftScale.SetScale(0.5)
shiftScale.SetOutputScalarTypeToDouble()
shiftScale2 = vtk.vtkImageShiftScale()
shiftScale2.SetInputConnection(shiftScale.GetOutputPort())
shiftScale2.SetShift(0)
shiftScale2.SetScale(2.0)
mag = vtk.vtkImageMagnify()
mag.SetInputConnection(shiftScale2.GetOutputPort())
mag.SetMagnificationFactors(4,4,1)
mag.InterpolateOff()
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(mag.GetOutputPort())
viewer.SetColorWindow(1024)
viewer.SetColorLevel(1024)
#make interface
#skipping source
#vtkPNMWriter w
#w SetFileName "D:/vtknew/vtk/graphics/examplesTcl/mace2.ppm"
#w SetInputConnection [shiftScale GetOutputPort]
#w Write
# --- end of script --
