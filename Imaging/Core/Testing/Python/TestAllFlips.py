#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Image pipeline
reader = vtk.vtkImageReader()
reader.GetExecutive().SetReleaseDataFlag(0,0)
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
imageFloat = vtk.vtkImageCast()
imageFloat.SetInputConnection(reader.GetOutputPort())
imageFloat.SetOutputScalarTypeToFloat()
flipX = vtk.vtkImageFlip()
flipX.SetInputConnection(imageFloat.GetOutputPort())
flipX.SetFilteredAxis(0)
flipY = vtk.vtkImageFlip()
flipY.SetInputConnection(imageFloat.GetOutputPort())
flipY.SetFilteredAxis(1)
flipY.FlipAboutOriginOn()
imageAppend = vtk.vtkImageAppend()
imageAppend.AddInputConnection(imageFloat.GetOutputPort())
imageAppend.AddInputConnection(flipX.GetOutputPort())
imageAppend.AddInputConnection(flipY.GetOutputPort())
imageAppend.SetAppendAxis(0)
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(imageAppend.GetOutputPort())
viewer.SetZSlice(22)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
#make interface
viewer.Render()
# --- end of script --
