#!/usr/bin/env python
# -*- coding: utf-8 -*-

from vtkmodules.vtkCommonCore import (
    vtkMath,
    vtkPoints,
)
from vtkmodules.vtkCommonDataModel import vtkBoundingBox
from vtkmodules.vtkCommonSystem import vtkTimerLog
import sys

# Test speed, serial versus threaded, of compute bounds in
# vtkBoundingBox.

# Control model size
baseSize = 1000
threadingCrossover = 750000
timer = vtkTimerLog()
math = vtkMath()

# Uncomment if you want to use as a little interactive program
#if len(sys.argv) >= 2 :
#    res = int(sys.argv[1])
#else:
#    res = baseSize


# Compares serial to threaded computation of the bounding box.  The current
# crossover point is hardcoded in the code (vtkBoundingBox.cxx) so this test
# may need to be updated in the future. (The crossover point, expressed in
# terms of number of points, is used to switch between a serial computation
# for smaller data, versus a threaded computation for larger data, due to the
# cost of spinning up threads.)

serialPoints = vtkPoints()
serialPoints.SetDataTypeToDouble()
serialPoints.SetNumberOfPoints(threadingCrossover - baseSize)
numSerialPts = threadingCrossover - baseSize;
for i in range(0,numSerialPts):
    serialPoints.SetPoint(i, math.Random(-1.0,1.0),
                          math.Random(-1.0,1.0),
                          math.Random(-1.0,1.0))

threadedPoints = vtkPoints()
threadedPoints.SetDataTypeToDouble()
threadedPoints.SetNumberOfPoints(threadingCrossover + baseSize)
numThreadedPts = threadingCrossover - baseSize;
for i in range(0,numThreadedPts):
    threadedPoints.SetPoint(i, math.Random(-1.0,1.0),
                          math.Random(-1.0,1.0),
                          math.Random(-1.0,1.0))

serialBox = [0.0,0.0,0.0,0.0,0.0,0.0]
threadedBox = [0.0,0.0,0.0,0.0,0.0,0.0]

bbox = vtkBoundingBox()
timer.StartTimer()
bbox.ComputeBounds(serialPoints,serialBox)
timer.StopTimer()
serialTime = timer.GetElapsedTime()

timer.StartTimer()
bbox.ComputeBounds(threadedPoints,threadedBox)
timer.StopTimer()
threadedTime = timer.GetElapsedTime()

print("vtkBoundingBox::ComputeBounds():")
print("\tSerial Time: {0}".format(serialTime))
print("\tBounds: {0}".format(serialBox))
print("\tThreaded Time: {0}".format(threadedTime))
print("\tBounds: {0}".format(threadedBox))
