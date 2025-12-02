#!/usr/bin/env python
from vtkmodules.vtkCommonCore import (
    vtkMath,
    vtkPoints
    )
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkCommonDataModel import (
    vtkDist2TupleArray,
    vtkPolyData,
    vtkStaticPointLocator2D
    )
from vtkmodules.vtkFiltersMeshing import (
    vtkAnnularBinIterator
    )

import sys

# Control problem size and set debugging parameters. For VTK
# testing (ctest), a default value is used. Otherwise, users can
# manually run the test with a specified number of points.
NPts = 1000
if len(sys.argv) > 1:
    try:
        NPts = int(sys.argv[1])
    except ValueError:
        NPts = 1000

PointsPerBucket = 1

# create some points - required double type
#
math = vtkMath()
math.RandomSeed(31415)
points = vtkPoints()
points.SetDataTypeToDouble()
i = 0
while i < NPts:
    points.InsertPoint(i,math.Random(0,1),math.Random(0,1),0.0)
    i = i + 1

pd = vtkPolyData()
pd.SetPoints(points)

# Create a locator
locator = vtkStaticPointLocator2D()
locator.SetDataSet(pd);
locator.BuildLocator();

# Now create an iterator
neiPtId = 100
x = [0.4,0.425,0]
results = vtkDist2TupleArray()

annularIter = vtkAnnularBinIterator(locator)
print(annularIter)
annularIter.Begin(neiPtId,x,results)
print(results)

# --- end of script --
