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

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetDataVOI(100,255,100,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)
#reader.ReleaseDataFlagOff()
#reader.DebugOn()

pad = vtkImageMirrorPad()
pad.SetInput(reader.GetOutput())
pad.SetOutputWholeExtent(-220,340,-220,340,0,92)
pad.SetOutputNumberOfScalarComponents(3)

viewer = vtkImageViewer()
viewer.SetInput(pad.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)
viewer.GetActor2D().SetDisplayPosition(220,220)

#make interface
WindowLevelInterface(viewer)
