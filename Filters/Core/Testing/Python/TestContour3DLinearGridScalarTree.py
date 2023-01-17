#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkFloatArray
from vtkmodules.vtkCommonDataModel import (
    vtkDataObject,
    vtkQuadric,
    vtkUnstructuredGrid,
)
from vtkmodules.vtkCommonExecutionModel import vtkSpanSpace
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import (
    vtkContour3DLinearGrid,
    vtkContourFilter,
    vtkExtractCells,
)
from vtkmodules.vtkImagingHybrid import vtkSampleFunction
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkPolyDataMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2

# Test vtkContour3DLinearGrid with vtkScalarTree.

# Control test size
res = 50
#res = 300
serialProcessing = 0
mergePoints = 1
interpolateAttr = 0
computeNormals = 0

#
# Quadric definition
quadric = vtkQuadric()
quadric.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])
sample = vtkSampleFunction()
sample.SetSampleDimensions(res,res,res)
sample.SetImplicitFunction(quadric)
sample.ComputeNormalsOn()
sample.Update()

#
# Extract voxel cells
extract = vtkExtractCells()
extract.SetInputConnection(sample.GetOutputPort())
extract.AddCellRange(0,sample.GetOutput().GetNumberOfCells())
extract.Update()

grid = vtkUnstructuredGrid()
grid.DeepCopy(extract.GetOutput())

# Add an array of all zeros
zeros = vtkFloatArray()
zeros.SetNumberOfTuples(grid.GetNumberOfPoints())
zeros.Fill(0.0)
zeros.SetName("zeros")
grid.GetPointData().AddArray(zeros)

# Now contour the cells, using scalar tree or not
stree = vtkSpanSpace()
stree.SetDataSet(grid)
stree.SetNumberOfCellsPerBucket(1)

contour = vtkContour3DLinearGrid()
contour.SetInputData(grid)
contour.SetValue(0, 0.5)
contour.SetValue(1, 0.75)
contour.SetMergePoints(mergePoints)
contour.SetSequentialProcessing(serialProcessing)
contour.SetInterpolateAttributes(interpolateAttr);
contour.SetComputeNormals(computeNormals);
contour.UseScalarTreeOff()

contourST = vtkContour3DLinearGrid()
contourST.SetInputData(grid)
contourST.SetValue(0, 0.5)
contourST.SetValue(1, 0.75)
contourST.SetMergePoints(mergePoints)
contourST.SetSequentialProcessing(serialProcessing)
contourST.SetInterpolateAttributes(interpolateAttr);
contourST.SetComputeNormals(computeNormals);
contourST.UseScalarTreeOn()
contourST.SetScalarTree(stree)

# Make sure we handle arrays with zero range
contourSTZero = vtkContour3DLinearGrid()
contourSTZero.SetInputData(grid)
contourSTZero.SetInputArrayToProcess(0, 0, 0, vtkDataObject.FIELD_ASSOCIATION_POINTS, "zero")
contourSTZero.SetValue(0, 0.5)
contourSTZero.SetMergePoints(mergePoints)
contourSTZero.SetInterpolateAttributes(interpolateAttr);
contourSTZero.SetComputeNormals(computeNormals);
contourSTZero.UseScalarTreeOn()
contourSTZero.Update()

assert(contourSTZero.GetOutput().GetNumberOfCells() == 0)

contMapper = vtkPolyDataMapper()
contMapper.SetInputConnection(contour.GetOutputPort())
contMapper.ScalarVisibilityOff()

contActor = vtkActor()
contActor.SetMapper(contMapper)
contActor.GetProperty().SetColor(.8,.4,.4)

# Compare against standard contouring
contour4 = vtkContourFilter()
contour4.SetInputConnection(extract.GetOutputPort())
contour4.SetValue(0,0.5)
contour4.SetValue(1, 0.75)

cont4Mapper = vtkPolyDataMapper()
cont4Mapper.SetInputConnection(contour4.GetOutputPort())
cont4Mapper.ScalarVisibilityOff()

cont4Actor = vtkActor()
cont4Actor.SetMapper(cont4Mapper)
cont4Actor.GetProperty().SetColor(.8,.4,.4)

# Timing comparisons
timer = vtkTimerLog()

timer.StartTimer()
stree.BuildTree()
timer.StopTimer()
streetime = timer.GetElapsedTime()

timer.StartTimer()
contour.Update()
timer.StopTimer()
time = timer.GetElapsedTime()

timer.StartTimer()
contourST.Update()
timer.StopTimer()
timeST = timer.GetElapsedTime()

timer.StartTimer()
contour4.Update()
timer.StopTimer()
time4 = timer.GetElapsedTime()

print("Contouring comparison")
print("\tNumber of threads used: {0}".format(contour.GetNumberOfThreadsUsed()))
print("\tMerge points: {0}".format(mergePoints))
print("\tInterpolate attributes: {0}".format(interpolateAttr))
print("\tCompute Normals: {0}".format(computeNormals))

print("\tBuild scalar tree: {0}".format(streetime))

print("\tLinear 3D contouring (No scalar tree): {0}".format(time))
print("\t\tNumber of points: {0}".format(contour.GetOutput().GetNumberOfPoints()))
print("\t\tNumber of cells: {0}".format(contour.GetOutput().GetNumberOfCells()))

print("\tLinear 3D contouring (With scalar tree): {0}".format(timeST))
print("\t\tNumber of points: {0}".format(contour.GetOutput().GetNumberOfPoints()))
print("\t\tNumber of cells: {0}".format(contour.GetOutput().GetNumberOfCells()))

print("\tStandard contouring: {0}".format(time4))
print("\t\tNumber of points: {0}".format(contour4.GetOutput().GetNumberOfPoints()))
print("\t\tNumber of cells: {0}".format(contour4.GetOutput().GetNumberOfCells()))

# Define graphics objects
renWin = vtkRenderWindow()
renWin.SetSize(400,200)
renWin.SetMultiSamples(0)

ren1 = vtkRenderer()
ren1.SetViewport(0,0,0.5,1)
ren1.SetBackground(1,1,1)
cam1 = ren1.GetActiveCamera()
ren4 = vtkRenderer()
ren4.SetViewport(0.5,0,1,1)
ren4.SetBackground(1,1,1)
ren4.SetActiveCamera(cam1)

renWin.AddRenderer(ren1)
renWin.AddRenderer(ren4)

iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(contActor)
ren4.AddActor(cont4Actor)

renWin.Render()
ren1.ResetCamera()
renWin.Render()

iren.Initialize()
iren.Start()
# --- end of script --
