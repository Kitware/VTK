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
sphere = vtkSphere(center=[0, 0, 0], radius=0.25)

# Create a volume with scalar data.
sampleF = vtkSampleFunction(implicit_function=sphere,
                            model_bounds=[-0.5,0.5, -0.5,0.5, -0.5,0.5],
                            sample_dimensions=[res, res, res])

# Also generate some cell data for testing
pd2cd = vtkPointDataToCellData(pass_point_data=True, process_all_arrays=False)
pd2cd.SetInputConnection(sampleF.GetOutputPort())
pd2cd.AddPointDataArray("scalars")
pd2cd.Update()

sample = (sampleF >> pd2cd).update().output

# Now extract pieces of the volume to create an
# unstructured grid.
pts = vtkPoints()
cells = vtkCellArray()
grid = vtkUnstructuredGrid()
grid.points = pts

numPts = sample.number_of_points
pts.number_of_points = numPts
grid.GetPointData().ShallowCopy(sample.GetPointData())
for pId in range(0,numPts):
    pts.SetPoint(pId,sample.GetPoint(pId))

numCells = sample.number_of_cells
genCell = vtkGenericCell()
grid.cell_data.CopyAllocate(sample.cell_data)
for cId in range(int(numCells/2),numCells):
#for cId in range(0,numCells):
    sample.GetCell(cId,genCell)
    newId = grid.InsertNextCell(genCell.cell_type,genCell.point_ids)
    grid.cell_data.CopyData(sample.cell_data,cId,newId)

# Clean the data with zero tolerance
clean0 = vtkStaticCleanUnstructuredGrid(input_data=grid, tolerance_is_absolute=True, tolerance=0.0,
                                        remove_unused_points=False)

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

assert(clean0.output.number_of_points == grid.number_of_points)
assert(clean0.output.number_of_cells == grid.number_of_cells)

# Clean the data with non-zero tolerance
clean1 = vtkStaticCleanUnstructuredGrid(input_data=grid, tolerance_is_absolute=False,
                                        tolerance=0.00001, remove_unused_points=True)

# Time execution
timer.StartTimer()
clean1.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to clean grid (non-zero tolerance): {0}".format(time))
print("\tRemaining points: {0}".format(clean1.output.number_of_points))
print("\tRemaining cells: {0}".format(clean1.output.number_of_cells))

assert(clean1.output.number_of_points == res*res*(1+int(res/2)))
assert(clean1.output.number_of_cells == grid.number_of_cells)

# Now shrink the data and clean
shrink0 = vtkShrinkFilter(input_data=sample, shrink_factor=1.0)
shrink0.Update()

# This is with a zero tolerance
shrink0 >> clean0
clean0.average_point_data = True

timer.StartTimer()
clean0.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("\nCleaning shrunk data with: {0} points and {1} cells".format(shrink0.output.number_of_points,shrink0.output.number_of_cells))
print("Time to clean grid (zero tolerance): {0}".format(time))
print("\tRemaining points: {0}".format(clean0.output.number_of_points))
print("\tRemaining cells: {0}".format(clean0.output.number_of_cells))

assert(clean0.output.number_of_points == sample.number_of_points)
assert(clean0.output.number_of_cells == sample.number_of_cells)

shrink1 = vtkShrinkFilter(input_data=sample, shrink_factor=0.999)
shrink1.Update()

shrink1 >> clean1
clean1.tolerance_is_absolute = True
clean1.absolute_tolerance = 0.01
clean1.produce_merge_map = True
clean1.average_point_data = True

timer.StartTimer()
clean1.Update()
timer.StopTimer()
time = timer.GetElapsedTime()
print("Time to clean grid (non-zero tolerance): {0}".format(time))
print("\tRemaining points: {0}".format(clean1.output.number_of_points))
print("\tRemaining cells: {0}".format(clean1.output.number_of_cells))

assert(clean1.output.number_of_points == sample.number_of_points)
assert(clean1.output.number_of_cells == sample.number_of_cells)

# Graphics stuff
# Create the RenderWindow, Renderers and both Actors
ren0 = vtkRenderer()
renWin = vtkRenderWindow()
renWin.SetMultiSamples(0)
renWin.AddRenderer(ren0)
iren = vtkRenderWindowInteractor(render_window=renWin)

# Display the clipped cells
mapper1 = vtkDataSetMapper()
clean1 >> mapper1
mapper1.scalar_range = sample.point_data.scalars.range

actor1 = vtkActor(mapper=mapper1)
actor1.property.SetInterpolationToFlat()

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
