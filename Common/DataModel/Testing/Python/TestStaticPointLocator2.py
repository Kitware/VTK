#!/usr/bin/env python
import vtk

# Test how the locator bins are generated
# Test how the vtkBoundingBox class computes binning divisions

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

# Create an initial set of points and associated dataset
points = vtk.vtkPoints()
points.SetDataTypeToDouble()
points.SetNumberOfPoints(numPts)
for i in range(0,numPts):
    points.SetPoint(i,math.Random(-0.25,0.25),math.Random(-0.5,0.5),math.Random(-1,1))

polydata = vtk.vtkPolyData()
polydata.SetPoints(points)

# Print initial statistics
print("Processing NumPts: {0}".format(numPts))

# Time the creation and building of the static point locator
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

# Test the generation of the binning divisions when degenerate conditions occur
print("\nTesting degenerate points specification")
locator.AutomaticOn()
points.SetNumberOfPoints(10)
points.SetPoint(0, 0,0,0)
points.SetPoint(1, 0,0,1)
points.SetPoint(2, 0,0,2)
points.SetPoint(3, 0,0,3)
points.SetPoint(4, 0,0,4)
points.SetPoint(5, 0,0,5)
points.SetPoint(6, 0,0,6)
points.SetPoint(7, 0,0,7)
points.SetPoint(8, 0,0,8)
points.SetPoint(9, 0,0,9)
locator.Modified()
locator.BuildLocator()
print("Divisions: {0}".format( locator.GetDivisions() ))
ndivs = locator.GetDivisions()
if ndivs[0] != 1 or ndivs[1] != 1 or ndivs[2] != 2:
    error = 1

# Test the generation of the binning divisions when degenerate conditions occur
print("\nTesting manual divisions, degenerate points specification")
locator.AutomaticOff()
locator.SetDivisions(0,2,10)
locator.Modified()
locator.BuildLocator()
print("Divisions: {0}".format( locator.GetDivisions() ))
ndivs = locator.GetDivisions()
if ndivs[0] != 1 or ndivs[1] != 2 or ndivs[2] != 10:
    error = 1

# Test the vtkBoundingBox code
targetBins = 2500
print("\nTesting vtkBoundingBox w/ {} target bins".format(targetBins))
divs = [1,1,1]
bounds = [0,0,0,0,0,0]
bds = [0,0,0,0,0,0]
bbox = vtk.vtkBoundingBox()

# Degenerate
bbox.SetBounds(0,0,1,1,2,2)
bbox.ComputeDivisions(targetBins,bounds,divs)
bbox.GetBounds(bds)
print("BBox bounds: ({},{}, {},{}, {},{})".format(bds[0],bds[1],bds[2],bds[3],bds[4],bds[5]))
print("   Adjusted bounds: ({},{}, {},{}, {},{})".format(bounds[0],bounds[1],bounds[2],bounds[3],bounds[4],bounds[5]))
print("   Divisions: ({},{},{})".format(divs[0],divs[1],divs[2]))
if divs[0] != 1 or divs[1] != 1 or divs[2] != 1:
    error = 1

# Line
bbox.SetBounds(0,0,1,1,5,10)
bbox.ComputeDivisions(targetBins,bounds,divs)
bbox.GetBounds(bds)
print("BBox bounds: ({},{}, {},{}, {},{})".format(bds[0],bds[1],bds[2],bds[3],bds[4],bds[5]))
print("   Adjusted bounds: ({},{}, {},{}, {},{})".format(bounds[0],bounds[1],bounds[2],bounds[3],bounds[4],bounds[5]))
print("   Divisions: ({},{},{})".format(divs[0],divs[1],divs[2]))
if divs[0] != 1 or divs[1] != 1 or divs[2] != targetBins:
    error = 1

# Plane
bbox.SetBounds(-1,4,1,1,5,10)
bbox.ComputeDivisions(targetBins,bounds,divs)
bbox.GetBounds(bds)
print("BBox bounds: ({},{}, {},{}, {},{})".format(bds[0],bds[1],bds[2],bds[3],bds[4],bds[5]))
print("   Adjusted bounds: ({},{}, {},{}, {},{})".format(bounds[0],bounds[1],bounds[2],bounds[3],bounds[4],bounds[5]))
print("   Divisions: ({},{},{})".format(divs[0],divs[1],divs[2]))
if divs[0] != targetBins**(1/2.0) or divs[1] != 1 or divs[2] != targetBins**(1/2.0):
    error = 1

# Volume
bbox.SetBounds(-6,4,1,2,5,10)
bbox.ComputeDivisions(targetBins,bounds,divs)
bbox.GetBounds(bds)
print("BBox bounds: ({},{}, {},{}, {},{})".format(bds[0],bds[1],bds[2],bds[3],bds[4],bds[5]))
print("   Adjusted bounds: ({},{}, {},{}, {},{})".format(bounds[0],bounds[1],bounds[2],bounds[3],bounds[4],bounds[5]))
print("   Divisions: ({},{},{})".format(divs[0],divs[1],divs[2]))
if divs[0] != 36 or divs[1] != 3 or divs[2] != 18:
    error = 1

assert error == 0
