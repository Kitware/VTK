#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# Make an image larger by repeating the data.  Tile.




# Image pipeline
reader = vtkPNMReader()
reader.ReleaseDataFlagOff()
reader.SetFileName(VTK_DATA + "/earth.ppm")

pad = vtkImageMirrorPad()
pad.SetInput(reader.GetOutput())
pad.SetOutputWholeExtent(-220,340,-220,340,0,0)

quant = vtkImageQuantizeRGBToIndex()
quant.SetInput(pad.GetOutput())
quant.SetNumberOfColors(16)
quant.Update()

map = vtkImageMapToRGBA()
map.SetInput(quant.GetOutput())
map.SetLookupTable(quant.GetLookupTable())

viewer = vtkImageViewer()
viewer.SetInput(map.GetOutput())
viewer.SetZSlice(0)
viewer.SetColorWindow(256)
viewer.SetColorLevel(127)
viewer.GetActor2D().SetDisplayPosition(220,220)
viewer.Render()

WindowLevelInterface(viewer)
