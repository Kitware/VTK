#!/usr/bin/env python
import vtk

# create a test dataset
#
math = vtk.vtkMath()
math.RandomSeed(314159)

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

# Create an initial set of points and associated dataset
points = vtk.vtkPoints()
points.SetDataTypeToDouble()
points.SetNumberOfPoints(numPts)
for i in range(0,numPts):
    points.SetPoint(i,math.Random(-1,1),math.Random(-1,1), 0.0)

polydata = vtk.vtkPolyData()
polydata.SetPoints(points)
points.ComputeBounds()

# Create points array which are positions to probe data with
# FindClosestPoint(), We also create an array to hold the results of this
# probe operation.
probePoints = vtk.vtkPoints()
probePoints.SetDataTypeToDouble()
probePoints.SetNumberOfPoints(numProbes)
for i in range (0,numProbes):
    probePoints.SetPoint(i,math.Random(-1,1),math.Random(-1,1), 0.0)
staticClosest = vtk.vtkIdList()
staticClosest.SetNumberOfIds(numProbes)

# Print initial statistics
print("Processing NumPts: {0}".format(numPts))
print("\n")

# StaticPointLocator
# Time the creation of static point locator
staticLocator = vtk.vtkStaticPointLocator2D()
staticLocator.SetDataSet(polydata)
staticLocator.SetNumberOfPointsPerBucket(5)
staticLocator.AutomaticOn()

staticTimer = vtk.vtkTimerLog()
staticTimer.StartTimer()
staticLocator.BuildLocator()
staticTimer.StopTimer()
BuildTime = staticTimer.GetElapsedTime()
print("Build Static Point Locator: {0}".format(BuildTime))

# Now probe the dataset with FindClosestPoint()
math.RandomSeed(314159)
staticTimer.StartTimer()
for i in range (0,numProbes):
    staticClosest.SetId(i, staticLocator.FindClosestPoint(probePoints.GetPoint(i)))
staticTimer.StopTimer()
staticOpTime = staticTimer.GetElapsedTime()
print("    Static Closest point probing: {0}".format(staticOpTime))
print("    Divisions: {0}".format( staticLocator.GetDivisions() ))

# Poke other methods before deleting static locator class
staticClosestN = vtk.vtkIdList()
staticLocator.FindClosestNPoints(10, probePoints.GetPoint(0), staticClosestN)

# Intersect with line
# Out of plane line intersecction
a0 = [0.5, 0.5, 1]
a1 = [0.5, 0.5, -1]
tol = 0.001
t = vtk.reference(0.0)
lineX = [0.0,0.0,0.0]
ptX = [0.0,0.0,0.0]
ptId = vtk.reference(-100)
staticLocator.IntersectWithLine(a0,a1,tol,t,lineX,ptX,ptId)
print("    Out of plane line intersection: PtId {0}".format(ptId))

# In plane line intersection
a0 = [-1.5, 0.5, 0]
a1 = [ 1.5, 0.5, 0]
tol = 0.001
t = vtk.reference(0.0)
lineX = [0.0,0.0,0.0]
ptX = [0.0,0.0,0.0]
ptId = vtk.reference(-100)
staticLocator.IntersectWithLine(a0,a1,tol,t,lineX,ptX,ptId)
print("    In plane line intersection: PtId {0}".format(ptId))

# Okay now delete class
staticTimer.StartTimer()
del staticLocator
staticTimer.StopTimer()
StaticTime2 = staticTimer.GetElapsedTime()
totalStaticTime = BuildTime + staticOpTime + StaticTime2

print("    Delete Point Locator: {0}".format(StaticTime2))
print("    Static Point Locator (Total): {0}".format(totalStaticTime))
print("\n")
