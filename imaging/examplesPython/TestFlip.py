import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'

from vtkpython import *
from WindowLevelInterface import *

# Image pipeline

reader = vtkImageReader()
reader.ReleaseDataFlagOff()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,93)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

flip = vtkImageFlip()
flip.SetInput(reader.GetOutput())
flip.SetFilteredAxes(VTK_IMAGE_X_AXIS) 
#flip.BypassOn()
#flip.PreserveImageExtentOn()

viewer = vtkImageViewer()
viewer.SetInput(flip.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(2000)
viewer.SetColorLevel(1000)

#make interface
WindowLevelInterface(viewer)







