#!/usr/bin/env python
from vtkmodules.vtkCommonCore import vtkPoints
from vtkmodules.vtkCommonDataModel import (
    vtkCellArray,
    vtkGenericCell,
    vtkSphere,
    vtkUnstructuredGrid,
)
from vtkmodules.vtkCommonSystem import vtkTimerLog
from vtkmodules.vtkFiltersCore import (
    vtkPointDataToCellData,
    vtkStaticCleanUnstructuredGrid,
)
from vtkmodules.vtkFiltersGeneral import vtkShrinkFilter
from vtkmodules.vtkImagingHybrid import vtkSampleFunction
from vtkmodules.vtkRenderingCore import (
    vtkActor,
    vtkDataSetMapper,
    vtkRenderWindow,
    vtkRenderWindowInteractor,
    vtkRenderer,
)
import vtkmodules.vtkInteractionStyle
import vtkmodules.vtkRenderingFreeType
import vtkmodules.vtkRenderingOpenGL2

# Control test size
res = 25

# Use a volume as a way to generate points and cells
# Create a synthetic source
sphere = vtkSphere()
sphere.SetCenter( 0.0,0.0,0.0)
sphere.SetRadius(0.25)

# Create a volume with scalar data.
sampleF = vtkSampleFunction()
sampleF.SetImplicitFunction(sphere)
sampleF.SetModelBounds(-0.5,0.5, -0.5,0.5, -0.5,0.5)
sampleF.SetSampleDimensions(res,res,res)

# Also generate some cell data for testing
pd2cd = vtkPointDataToCellData()
pd2cd.SetInputConnection(sampleF.GetOutputPort())
pd2cd.PassPointDataOn()
pd2cd.ProcessAllArraysOff()
pd2cd.AddPointDataArray("scalars")
pd2cd.Update()

sample = pd2cd.GetOutput()

# Now extract pieces of the volume to create an
# unstructured grid.
pts = vtkPoints()
cells = vtkCellArray()
grid = vtkUnstructuredGrid()
grid.SetPoints(pts)

numPts = sample.GetNumberOfPoints()
pts.SetNumberOfPoints(numPts)
grid.GetPointData().ShallowCopy(sample.GetPointData())
for pId in range(0,numPts):
    pts.SetPoint(pId,sample.GetPoint(pId))

numCells = sample.GetNumberOfCells()
genCell = vtkGenericCell()
grid.GetCellData().CopyAllocate(sample.GetCellData())
for cId in range(int(numCells/2),numCells):
#for cId in range(0,numCells):
    sample.GetCell(cId,genCell)
    newId = grid.InsertNextCell(genCell.GetCellType(),genCell.GetPointIds())
    grid.GetCellData().CopyData(sample.GetCellData(),cId,newId)

# Clean the data with zero tolerance
clean0 = vtkStaticCleanUnstructuredGrid()
clean0.SetInputData(grid)
clean0.ToleranceIsAbsoluteOn()
clean0.SetTolerance(0.0)
clean0.RemoveUnusedPointsOff()

# Time execution
timer = vtkTimerLog()
timer.StartTimer()
clean0.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Cleaning an unstructured grid with: {0} points and {1} cells".format(grid.GetNumberOfPoints(),grid.GetNumberOfCells()))
print("Time to clean grid (zero tolerance): {0}".format(time))
print("\tRemaining points: {0}".format(clean0.GetOutput().GetNumberOfPoints()))
print("\tRemaining cells: {0}".format(clean0.GetOutput().GetNumberOfCells()))

assert(clean0.GetOutput().GetNumberOfPoints() == grid.GetNumberOfPoints())
assert(clean0.GetOutput().GetNumberOfCells() == grid.GetNumberOfCells())

# Clean the data with non-zero tolerance
clean1 = vtkStaticCleanUnstructuredGrid()
clean1.SetInputData(grid)
clean1.ToleranceIsAbsoluteOff()
clean1.SetTolerance(0.00001)
clean1.RemoveUnusedPointsOn()

# Time execution
timer.StartTimer()
clean1.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to clean grid (non-zero tolerance): {0}".format(time))
print("\tRemaining points: {0}".format(clean1.GetOutput().GetNumberOfPoints()))
print("\tRemaining cells: {0}".format(clean1.GetOutput().GetNumberOfCells()))

assert(clean1.GetOutput().GetNumberOfPoints() == res*res*(1+int(res/2)))
assert(clean1.GetOutput().GetNumberOfCells() == grid.GetNumberOfCells())

# Now shrink the data and clean
shrink0 = vtkShrinkFilter()
shrink0.SetInputData(sample)
shrink0.SetShrinkFactor(1.0)
shrink0.Update()

# This is with a zero tolerance
clean0.SetInputConnection(shrink0.GetOutputPort())
clean0.AveragePointDataOn()

timer.StartTimer()
clean0.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("\nCleaning shrunk data with: {0} points and {1} cells".format(shrink0.GetOutput().GetNumberOfPoints(),shrink0.GetOutput().GetNumberOfCells()))
print("Time to clean grid (zero tolerance): {0}".format(time))
print("\tRemaining points: {0}".format(clean0.GetOutput().GetNumberOfPoints()))
print("\tRemaining cells: {0}".format(clean0.GetOutput().GetNumberOfCells()))

assert(clean0.GetOutput().GetNumberOfPoints() == sample.GetNumberOfPoints())
assert(clean0.GetOutput().GetNumberOfCells() == sample.GetNumberOfCells())

shrink1 = vtkShrinkFilter()
shrink1.SetInputData(sample)
shrink1.SetShrinkFactor(0.999)
shrink1.Update()

clean1.SetInputConnection(shrink1.GetOutputPort())
clean1.ToleranceIsAbsoluteOn()
clean1.SetAbsoluteTolerance(0.01)
clean1.ProduceMergeMapOn()
clean1.AveragePointDataOn()

timer.StartTimer()
clean1.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to clean grid (non-zero tolerance): {0}".format(time))
print("\tRemaining points: {0}".format(clean1.GetOutput().GetNumberOfPoints()))
print("\tRemaining cells: {0}".format(clean1.GetOutput().GetNumberOfCells()))

assert(clean1.GetOutput().GetNumberOfPoints() == sample.GetNumberOfPoints())
assert(clean1.GetOutput().GetNumberOfCells() == sample.GetNumberOfCells())

# Graphics stuff
# Create the RenderWindow, Renderers and both Actors
ren0 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren0)
iren = vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

# Display the clipped cells
mapper1 = vtkDataSetMapper()
mapper1.SetInputConnection(clean1.GetOutputPort())
mapper1.SetScalarRange(sample.GetPointData().GetScalars().GetRange())

actor1 = vtkActor()
actor1.SetMapper(mapper1)
actor1.GetProperty().SetInterpolationToFlat()

# Add the actors to the renderer, set the background and size
ren0.AddActor(actor1)

ren0.SetBackground(0,0,0)
renWin.SetSize(300,300)
camera = ren0.GetActiveCamera()
camera.SetPosition(1,1,1)
ren0.ResetCamera()
iren.Initialize()

renWin.Render()
iren.Start()
# --- end of script --
