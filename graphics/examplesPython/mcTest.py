#!/usr/local/bin/python

from libVTKCommonPython import *
from libVTKGraphicsPython import *
import time

#catch  load vtktcl 
# Test marching cubes speed
#
v16 = vtkVolume16Reader()
v16.SetDataDimensions(64,64)
v16.GetOutput().SetOrigin(0.0,0.0,0.0)
v16.SetDataByteOrderToLittleEndian()
v16.SetFilePrefix("../../../vtkdata/headsq/quarter")
v16.SetImageRange(1,93)
v16.SetDataSpacing(3.2,3.2,1.5)
v16.Update()

start = time.time()
iso = vtkContourFilter()
iso.SetInput(v16.GetOutput())
iso.SetValue(0,1150)
iso.Update()
print "%6.2f seconds" % (time.time() - start)

#exit()
#iren.Start()
