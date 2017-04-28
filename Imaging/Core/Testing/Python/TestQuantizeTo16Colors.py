#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Make an image larger by repeating the data.  Tile.
# Image pipeline
reader = vtk.vtkPNMReader()
reader.ReleaseDataFlagOff()
reader.SetFileName("" + str(VTK_DATA_ROOT) + "/Data/earth.ppm")
pad = vtk.vtkImageMirrorPad()
pad.SetInputConnection(reader.GetOutputPort())
pad.SetOutputWholeExtent(-120,320,-120,320,0,0)
quant = vtk.vtkImageQuantizeRGBToIndex()
quant.SetInputConnection(pad.GetOutputPort())
quant.SetNumberOfColors(16)
quant.Update()
map = vtk.vtkImageMapToRGBA()
map.SetInputConnection(quant.GetOutputPort())
map.SetLookupTable(quant.GetLookupTable())
viewer = vtk.vtkImageViewer()
viewer.SetInputConnection(map.GetOutputPort())
viewer.SetColorWindow(256)
viewer.SetColorLevel(127)
viewer.GetActor2D().SetDisplayPosition(110,110)
viewer.Render()
# --- end of script --
