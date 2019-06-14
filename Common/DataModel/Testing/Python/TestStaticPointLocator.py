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

# Create an initial set of points and associated dataset
points = vtk.vtkPoints()
points.SetDataTypeToDouble()
points.SetNumberOfPoints(numPts)
for i in range(0,numPts):
    points.SetPoint(i,math.Random(-1,1),math.Random(-1,1),math.Random(-1,1))

polydata = vtk.vtkPolyData()
polydata.SetPoints(points)
points.ComputeBounds()

# Create points array which are positions to probe data with
# FindClosestPoint(), We also create an array to hold the results of this
# probe operation.
probePoints = vtk.vtkPoints()
probePoints.SetDataTypeToDouble()
probePoints.SetNumberOfPoints(numProbes)
math.RandomSeed(314159)
for i in range (0,numProbes):
    probePoints.SetPoint(i,math.Random(-1,1),math.Random(-1,1),math.Random(-1,1))
closest = vtk.vtkIdList()
closest.SetNumberOfIds(numProbes)
staticClosest = vtk.vtkIdList()
staticClosest.SetNumberOfIds(numProbes)

# Print initial statistics
print("Processing NumPts: {0}".format(numPts))
print("\n")

# Time the creation and building of the incremental point locator
locator = vtk.vtkPointLocator()
locator.SetDataSet(polydata)
locator.SetNumberOfPointsPerBucket(5)
locator.AutomaticOn()

timer = vtk.vtkTimerLog()
timer.StartTimer()
locator.BuildLocator()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Build Point Locator: {0}".format(time))

# Probe the dataset with FindClosestPoint() and time it
timer.StartTimer()
for i in range (0,numProbes):
    closest.SetId(i, locator.FindClosestPoint(probePoints.GetPoint(i)))
timer.StopTimer()
opTime = timer.GetElapsedTime()
print("    Closest point probing: {0}".format(opTime))
print("    Divisions: {0}".format( locator.GetDivisions() ))

# Poke other methods before deleting locator class
closestN = vtk.vtkIdList()
locator.FindClosestNPoints(10, probePoints.GetPoint(0), closestN)

# Time the deletion of the locator. The incremental locator is quite slow due
# to fragmented memory.
timer.StartTimer()
del locator
timer.StopTimer()
time2 = timer.GetElapsedTime()
totalTime = time + opTime + time2
print("    Delete Point Locator: {0}".format(time2))
print("    Point Locator (Total): {0}".format(totalTime))
print("\n")

# StaticPointLocator
# Time the creation of static point locator
staticLocator = vtk.vtkStaticPointLocator()
staticLocator.SetDataSet(polydata)
staticLocator.SetNumberOfPointsPerBucket(5)
staticLocator.AutomaticOn()

staticTimer = vtk.vtkTimerLog()
staticTimer.StartTimer()
staticLocator.BuildLocator()
staticTimer.StopTimer()
StaticTime = staticTimer.GetElapsedTime()
print("Build Static Point Locator: {0}".format(StaticTime))

# Now probe the dataset with FindClosestPoint()
math.RandomSeed(314159)
staticTimer.StartTimer()
for i in range (0,numProbes):
    staticClosest.SetId(i, staticLocator.FindClosestPoint(probePoints.GetPoint(i)))
staticTimer.StopTimer()
staticOpTime = staticTimer.GetElapsedTime()
print("    Static Closest point probing: {0}".format(staticOpTime))
print("    Divisions: {0}".format( staticLocator.GetDivisions() ))

# Check that closest point operation gives the same answer and the
# incremental point locator. Note that it is possible to realize different
# results because buckets (and hence points) are processed in a different
# order, and if the distance apart is the same then the order decides which
# point is selected (from FindClosestPoint()). For small random datasets this
# is unlikely to happen.
error = 0
x = [0,0,0]
y = [0,0,0]
p = [0,0,0]
math = vtk.vtkMath()
for i in range (0,numProbes):
    staticId = staticClosest.GetId(i)
    closestId = closest.GetId(i)
    if closestId != staticId:
        probePoints.GetPoint(i,p)
        points.GetPoint(staticId,x)
        points.GetPoint(closestId,y)
        dx2 = math.Distance2BetweenPoints(x,p)
        dy2 = math.Distance2BetweenPoints(y,p)
        if dx2 != dy2:
            error = 1

# Poke other methods before deleting static locator class
staticClosestN = vtk.vtkIdList()
staticLocator.FindClosestNPoints(10, probePoints.GetPoint(0), staticClosestN)
for i in range (0,10):
    staticId = staticClosestN.GetId(i)
    closestId = closestN.GetId(i)
    if staticId != closestId:
        error = 1

# Okay now delete class
staticTimer.StartTimer()
del staticLocator
staticTimer.StopTimer()
StaticTime2 = staticTimer.GetElapsedTime()
totalStaticTime = StaticTime + staticOpTime + StaticTime2

print("    Delete Point Locator: {0}".format(StaticTime2))
print("    Static Point Locator (Total): {0}".format(totalStaticTime))
print("\n")

# Print out the speedups
print("Speed ups:")
if StaticTime > 0.0:
    print("    Build: {0}".format(time/StaticTime))
else:
    print("    Build: (really big)")
if staticOpTime > 0.0:
    print("    Probe: {0}".format(opTime/staticOpTime))
else:
    print("    Probe: (really big)")
if StaticTime2 > 0.0:
    print("    Delete: {0}".format(time2/StaticTime2))
else:
    print("    Delete: (really big)")
if totalStaticTime > 0.0:
    print("    Total: {0}".format(totalTime/totalStaticTime) )
else:
    print("    Total: (really big)")


# Return test results. If the assert is not true, then different results were
# generated by vtkStaticPointLocator and vtkPointLocator.
assert error == 0
