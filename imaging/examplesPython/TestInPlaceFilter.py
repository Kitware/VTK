#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# Derived from Cursor3D.  This script increases the coverage of the
# vtkImageInplaceFilter super class.


# global values
CURSOR_X = 20
CURSOR_Y = 20
CURSOR_Z = 20

IMAGE_MAG_X = 2
IMAGE_MAG_Y = 2
IMAGE_MAG_Z = 1



# pipeline stuff
reader = vtkSLCReader()
reader.SetFileName(VTK_DATA + "/poship.slc")

# make the image a little biger
magnify = vtkImageMagnify()
magnify.SetInput(reader.GetOutput())
magnify.SetMagnificationFactors(IMAGE_MAG_X,IMAGE_MAG_Y,IMAGE_MAG_Z)
magnify.ReleaseDataFlagOn()

# one filter that just passes the data trough
cursor1 = vtkImageCursor3D()
cursor1.SetInput(magnify.GetOutput())
cursor1.SetCursorPosition(CURSOR_X*IMAGE_MAG_X, \
                          CURSOR_Y*IMAGE_MAG_Y, \
                          CURSOR_Z*IMAGE_MAG_Z)
cursor1.SetCursorValue(255)
cursor1.SetCursorRadius(50*IMAGE_MAG_X)
cursor1.BypassOn()

# a second filter that does in place processing (magnify ReleaseDataFlagOn)
cursor2 = vtkImageCursor3D()
cursor2.SetInput(magnify.GetOutput())
cursor2.SetCursorPosition(CURSOR_X*IMAGE_MAG_X, \
                          CURSOR_Y*IMAGE_MAG_Y, \
                          CURSOR_Z*IMAGE_MAG_Z)
cursor2.SetCursorValue(255)
cursor2.SetCursorRadius(50*IMAGE_MAG_X)
# stream to increase coverage of in place filter.
cursor2.SetInputMemoryLimit(1)

# put thge two together in one image
append = vtkImageAppend()
append.SetAppendAxis(0)
append.AddInput(cursor1.GetOutput())
append.AddInput(cursor2.GetOutput())

viewer = vtkImageViewer()
viewer.SetInput(append.GetOutput())
viewer.SetZSlice(CURSOR_Z*IMAGE_MAG_Z)
viewer.SetColorWindow(200)
viewer.SetColorLevel(80)
#viewer.DebugOn()
viewer.Render()

viewer.SetPosition(50,50)

#make interface
WindowLevelInterface(viewer)

