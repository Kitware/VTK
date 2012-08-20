#!/usr/bin/env python
import vtk
from vtk.test import Testing
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Make an image larger by repeating the data.  Tile.
# Image pipeline
reader = vtk.vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,63,0,63,1,93)
reader.SetDataSpacing(3.2,3.2,1.5)
reader.SetFilePrefix("" + str(VTK_DATA_ROOT) + "/Data/headsq/quarter")
reader.SetDataMask(0x7fff)
pad = vtk.vtkImageWrapPad()
pad.SetInputConnection(reader.GetOutputPort())
pad.SetOutputWholeExtent(-100,155,-100,170,0,92)
pad.ReleaseDataFlagOff()
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(pad.GetOutputPort())
viewer.SetZSlice(22)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.GetActor2D().SetDisplayPosition(150,150)
viewer.Render()
# --- end of script --
