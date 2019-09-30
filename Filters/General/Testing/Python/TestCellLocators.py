#!/usr/bin/env python
import vtk
from vtk.test import Testing

# create a test dataset
#
math = vtk.vtkMath()

# Note: the bigger the data the better vtkStaticPointLocator performs
#testSize = "large"
#testSize = "medium"
testSize = "small"

if testSize == "large":
    res = 125
    numProbes = 5000000
elif testSize == "medium":
    res = 75
    numProbes = 500000
else:
    res = 25
    numProbes = 5000

# Create an initial set of points and associated dataset
mandel = vtk.vtkImageMandelbrotSource()
mandel.SetWholeExtent(-res,res,-res,res,-res,res)
mandel.Update()
#print mandel.GetOutput()

plane = vtk.vtkPlane()
plane.SetOrigin(res+1,res+1,res+1)
plane.SetNormal(-1,-1,-1)

# Clip data to spit out unstructured tets
clipper = vtk.vtkClipDataSet()
clipper.SetInputConnection(mandel.GetOutputPort())
clipper.SetClipFunction(plane)
clipper.Update()

output = clipper.GetOutput()
numCells = output.GetNumberOfCells()
#bounds = output.GetBounds()

# Create points array which are positions to probe data with
# FindCell(), We also create an array to hold the results of this
# probe operation.
ProbeCells = vtk.vtkPoints()
ProbeCells.SetDataTypeToDouble()
ProbeCells.SetNumberOfPoints(numProbes)
math.RandomSeed(314159)
for i in range (0,numProbes):
    ProbeCells.SetPoint(i,math.Random(-3,-0.5),math.Random(-2.5,0),math.Random(-1,1))
closest = vtk.vtkIdList()
closest.SetNumberOfIds(numProbes)
treeClosest = vtk.vtkIdList()
treeClosest.SetNumberOfIds(numProbes)
staticClosest = vtk.vtkIdList()
staticClosest.SetNumberOfIds(numProbes)
bspClosest = vtk.vtkIdList()
bspClosest.SetNumberOfIds(numProbes)
obbClosest = vtk.vtkIdList()
obbClosest.SetNumberOfIds(numProbes)
dsClosest = vtk.vtkIdList()
dsClosest.SetNumberOfIds(numProbes)

genCell = vtk.vtkGenericCell()
pc = [0,0,0]
weights = [0,0,0,0,0,0,0,0,0,0,0,0]
subId = vtk.reference(0)

# Print initial statistics
print("Processing NumCells: {0}".format(numCells))
print("\n")
timer = vtk.vtkTimerLog()

#############################################################
# Time the creation and building of the static cell locator
locator2 = vtk.vtkStaticCellLocator()
locator2.SetDataSet(output)
locator2.AutomaticOn()
locator2.SetNumberOfCellsPerNode(20)
locator2.CacheCellBoundsOn()

timer.StartTimer()
locator2.BuildLocator()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Build Static Cell Locator: {0}".format(time))

# Probe the dataset with FindClosestPoint() and time it
timer.StartTimer()
for i in range (0,numProbes):
    staticClosest.SetId(i, locator2.FindCell(ProbeCells.GetPoint(i),0.001,genCell,pc,weights))
timer.StopTimer()
opTime = timer.GetElapsedTime()
print("    Find cell probing: {0}".format(opTime))

# Time the deletion of the locator. The incremental locator is quite slow due
# to fragmented memory.
timer.StartTimer()
del locator2
timer.StopTimer()
time2 = timer.GetElapsedTime()
print("    Delete Static Cell Locator: {0}".format(time2))
print("    Static Cell Locator (Total): {0}".format(time+opTime+time2))
print("\n")

#############################################################
# Time the creation and building of the standard cell locator
locator = vtk.vtkCellLocator()
locator.SetDataSet(output)
locator.SetNumberOfCellsPerBucket(25)
locator.AutomaticOn()

timer.StartTimer()
locator.BuildLocator()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Build Cell Locator: {0}".format(time))

# Probe the dataset with FindClosestPoint() and time it
timer.StartTimer()
for i in range (0,numProbes):
    closest.SetId(i, locator.FindCell(ProbeCells.GetPoint(i),0.001,genCell,pc,weights))
timer.StopTimer()
opTime = timer.GetElapsedTime()
print("    Find cell probing: {0}".format(opTime))

# Time the deletion of the locator. The standard locator is quite slow due
# to fragmented memory.
timer.StartTimer()
del locator
timer.StopTimer()
time2 = timer.GetElapsedTime()
print("    Delete Cell Locator: {0}".format(time2))
print("    Cell Locator (Total): {0}".format(time+opTime+time2))
print("\n")

#############################################################
# Time the creation and building of the cell tree locator
locator1 = vtk.vtkCellTreeLocator()
locator1.SetDataSet(output)
locator1.AutomaticOn()

timer.StartTimer()
locator1.BuildLocator()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Build Cell Tree Locator: {0}".format(time))

# Probe the dataset with FindClosestPoint() and time it
timer.StartTimer()
for i in range (0,numProbes):
    treeClosest.SetId(i, locator1.FindCell(ProbeCells.GetPoint(i),0.001,genCell,pc,weights))
timer.StopTimer()
opTime = timer.GetElapsedTime()
print("    Find cell probing: {0}".format(opTime))

# Time the deletion of the locator. The incremental locator is quite slow due
# to fragmented memory.
timer.StartTimer()
del locator1
timer.StopTimer()
time2 = timer.GetElapsedTime()
print("    Delete Cell Tree Locator: {0}".format(time2))
print("    Cell Tree Locator (Total): {0}".format(time+opTime+time2))
print("\n")

#############################################################
# Time the creation and building of the bsp tree
locator3 = vtk.vtkModifiedBSPTree()
locator3.LazyEvaluationOff()
locator3.SetDataSet(output)
locator3.AutomaticOn()

timer.StartTimer()
locator3.BuildLocator()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Build BSP Tree Locator: {0}".format(time))

# Probe the dataset with FindClosestPoint() and time it
timer.StartTimer()
for i in range (0,numProbes):
    bspClosest.SetId(i, locator3.FindCell(ProbeCells.GetPoint(i),0.001,genCell,pc,weights))
timer.StopTimer()
opTime = timer.GetElapsedTime()
print("    Find cell probing: {0}".format(opTime))

# Time the deletion of the locator. The incremental locator is quite slow due
# to fragmented memory.
timer.StartTimer()
del locator3
timer.StopTimer()
time2 = timer.GetElapsedTime()
print("    Delete BSP Tree Locator: {0}".format(time2))
print("    BSP Tree Locator (Total): {0}".format(time+opTime+time2))
print("\n")

#############################################################
# Time the creation and building of the obb tree
locator4 = vtk.vtkOBBTree()
locator4.LazyEvaluationOff()
locator4.SetDataSet(output)
locator4.AutomaticOn()

timer.StartTimer()
locator4.BuildLocator()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Build OBB Locator: {0}".format(time))

# Probe the dataset with FindClosestPoint() and time it
timer.StartTimer()
for i in range (0,numProbes):
    print(i)
    obbClosest.SetId(i, locator4.FindCell(ProbeCells.GetPoint(i),0.001,genCell,pc,weights))
timer.StopTimer()
opTime = timer.GetElapsedTime()
print("    Find cell probing: {0}".format(opTime))

# Time the deletion of the locator. The incremental locator is quite slow due
# to fragmented memory.
timer.StartTimer()
del locator4
timer.StopTimer()
time2 = timer.GetElapsedTime()
print("    Delete OBB Locator: {0}".format(time2))
print("    OBB Locator (Total): {0}".format(time+opTime+time2))
print("\n")

#############################################################
# For comparison purposes compare to FindCell()
timer.StartTimer()

output.FindCell(ProbeCells.GetPoint(0),genCell,-1,0.001,subId,pc,weights)
timer.StopTimer()
time = timer.GetElapsedTime()
print("Point Locator: {0}".format(time))

# Probe the dataset with FindClosestPoint() and time it
timer.StartTimer()
for i in range (0,numProbes):
    dsClosest.SetId(i, output.FindCell(ProbeCells.GetPoint(0),genCell,-1,0.001,subId,pc,weights))
timer.StopTimer()
opTime = timer.GetElapsedTime()
print("    Find cell probing: {0}".format(opTime))

# Time the deletion of the locator. The incremental locator is quite slow due
# to fragmented memory.
timer.StartTimer()
del output
timer.StopTimer()
time2 = timer.GetElapsedTime()
print("    Delete Point Locator: {0}".format(time2))
print("    Point Locator (Total): {0}".format(time+opTime+time2))
print("\n")
