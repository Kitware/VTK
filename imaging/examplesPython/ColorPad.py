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
pad.SetOutputWholeExtent(-220,340,-220,340,0,92)

viewer = vtkImageViewer()
viewer.SetInput(pad.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(255)
viewer.SetColorLevel(127)
viewer.GetActor2D().SetDisplayPosition(220,220)

#make interface
WindowLevelInterface(viewer)
