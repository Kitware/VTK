#!/usr/bin/env python
import os
try:
  VTK_DATA = os.environ['VTK_DATA']
except KeyError:
  VTK_DATA = '../../../vtkdata/'


from vtkpython import *
from WindowLevelInterface import *

# Threshold a volume and write it to disk.
# It then reads the new data set from disk and displays it.
# Dont forget to delete the test files after the script is finished.


# Image pipeline

reader = vtkImageReader()
reader.SetDataByteOrderToLittleEndian()
reader.SetDataExtent(0,255,0,255,1,33)
reader.SetFilePrefix(VTK_DATA + "/fullHead/headsq")
reader.SetDataMask(0x7fff)

thresh = vtkImageThreshold()
thresh.SetInput(reader.GetOutput())
thresh.ThresholdByUpper(1000.0)
thresh.SetInValue(0.0)
thresh.SetOutValue(250.0)
thresh.ReplaceOutOn()
thresh.SetOutputScalarTypeToUnsignedChar()

writer = vtkImageWriter()
writer.SetInput(thresh.GetOutput())
writer.SetFileName("garf.xxx")
writer.SetFileName("test.xxx")
writer.SetFileDimensionality(3)
writer.Write()

reader2 = vtkImageReader()
reader2.SetDataScalarTypeToUnsignedChar()
reader2.ReleaseDataFlagOff()
reader2.SetDataExtent(0,255,0,255,1,33)
reader2.SetFileName("garf.xxx")
reader2.SetFileName("test.xxx")
reader2.SetFileDimensionality(3)

viewer = vtkImageViewer()
viewer.SetInput(reader2.GetOutput())
viewer.SetZSlice(22)
viewer.SetColorWindow(300)
viewer.SetColorLevel(150)

# make interface
WindowLevelInterface(viewer)
