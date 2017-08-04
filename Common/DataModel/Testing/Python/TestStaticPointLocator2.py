#!/usr/bin/env python
import vtk

# create a test dataset
#
math = vtk.vtkMath()

# Note: the bigger the data the better vtkStaticPointLocator performs
#testSize = "large"
#testSize = "medium"
testSize = "small"

if testSize == "large":
    numPts = 100000000
    numProbes = 1000000
elif testSize == "medium":
    numPts = 2000000
    numProbes = 50000
else:
    numPts = 20000
    numProbes = 5000

maxNumBuckets = 2500

# Create an initial set of points and asssociated datatset
points = vtk.vtkPoints()
points.SetDataTypeToDouble()
points.SetNumberOfPoints(numPts)
for i in range(0,numPts):
    points.SetPoint(i,math.Random(-0.25,0.25),math.Random(-0.5,0.5),math.Random(-1,1))

polydata = vtk.vtkPolyData()
polydata.SetPoints(points)

# Print initial statistics
print("Processing NumPts: {0}".format(numPts))

# Time the creation and building of the incremental point locator
locator = vtk.vtkStaticPointLocator()
locator.SetDataSet(polydata)
locator.SetNumberOfPointsPerBucket(5)
locator.AutomaticOn()
locator.SetMaxNumberOfBuckets(maxNumBuckets)

locator.BuildLocator()

print("Divisions: {0}".format( locator.GetDivisions() ))

error = 0
numBuckets = locator.GetNumberOfBuckets()
print("Number of Buckets: {0}".format(numBuckets))
if numBuckets > maxNumBuckets:
    error = 1

assert error == 0
