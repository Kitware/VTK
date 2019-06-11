#!/usr/bin/env python
import vtk
from vtk.util.misc import vtkGetDataRoot
VTK_DATA_ROOT = vtkGetDataRoot()

# Test vtkContour3DLinearGrid on tests, hexes, voxels.
# Also compare timing against vtkContourGrid.
# Four different pipelines are created.

# Control test size
res = 50
#res = 200
serialProcessing = 0
mergePoints = 1
interpolateAttr = 1
computeNormals = 1

# Test tetra contouring
#
# Quadric definition
quadric = vtk.vtkQuadric()
quadric.SetCoefficients([.5,1,.2,0,.1,0,0,.2,0,0])
sample = vtk.vtkSampleFunction()
sample.SetSampleDimensions(res,res,res)
sample.SetImplicitFunction(quadric)
sample.ComputeNormalsOn()
sample.Update()

# Generate tetrahedral mesh. The side effect of the clip filter
# is to produce tetrahedra.
clip = vtk.vtkClipVolume()
clip.SetInputConnection(sample.GetOutputPort())
clip.SetValue(-10.0)
clip.GenerateClippedOutputOff()
clip.Update()

# Test that filter can handle the dataset and contour array
assert(vtk.vtkContour3DLinearGrid.CanFullyProcessDataObject(clip.GetOutput(), "scalars"))

# Now contour the cells
contour = vtk.vtkContour3DLinearGrid()
contour.SetInputConnection(clip.GetOutputPort())
contour.SetValue(0, 0.5)
contour.SetValue(1, 0.9)
contour.SetMergePoints(0)
contour.SetSequentialProcessing(serialProcessing)
contour.SetInterpolateAttributes(0);
contour.SetComputeNormals(0);

contMapper = vtk.vtkPolyDataMapper()
contMapper.SetInputConnection(contour.GetOutputPort())
contMapper.ScalarVisibilityOff()

contActor = vtk.vtkActor()
contActor.SetMapper(contMapper)
contActor.GetProperty().SetColor(.8,.4,.4)

# Test voxel contouring
#
# Extract voxel cells
extract2 = vtk.vtkExtractCells()
extract2.SetInputConnection(sample.GetOutputPort())
extract2.AddCellRange(0,sample.GetOutput().GetNumberOfCells())
extract2.Update()

# Now contour the voxels
contour2 = vtk.vtkContour3DLinearGrid()
contour2.SetInputConnection(extract2.GetOutputPort())
contour2.SetValue(0, 0.5)
contour2.SetValue(1, 0.9)
contour2.SetMergePoints(mergePoints)
contour2.SetSequentialProcessing(serialProcessing)
contour2.SetInterpolateAttributes(interpolateAttr);
contour2.SetComputeNormals(computeNormals);

cont2Mapper = vtk.vtkPolyDataMapper()
cont2Mapper.SetInputConnection(contour2.GetOutputPort())
cont2Mapper.ScalarVisibilityOff()

cont2Actor = vtk.vtkActor()
cont2Actor.SetMapper(cont2Mapper)
cont2Actor.GetProperty().SetColor(.8,.4,.4)

# Test hex contouring. Have to create the geometry manually
# but can reuse the scalars from the sample function.
sgrid = vtk.vtkStructuredGrid()
sgrid.SetDimensions(res,res,res)

pts = vtk.vtkPoints()
o = sample.GetOutput().GetOrigin()
s = sample.GetOutput().GetSpacing()
for k in range(0,res):
    z = o[2] + k*s[2]
    for j in range(0,res):
        y = o[1] + j*s[1]
        for i in range(0,res):
            x = o[0] + i*s[0]
            pts.InsertNextPoint(x,y,z)
sgrid.SetPoints(pts)
sgrid.GetPointData().SetScalars(sample.GetOutput().GetPointData().GetScalars())
sgrid.GetPointData().SetVectors(sample.GetOutput().GetPointData().GetVectors())

# Extract hex cells
extract3 = vtk.vtkExtractCells()
extract3.SetInputData(sgrid)
extract3.AddCellRange(0,sample.GetOutput().GetNumberOfCells())
extract3.Update()

# Now contour the hexes
contour3 = vtk.vtkContour3DLinearGrid()
contour3.SetInputConnection(extract3.GetOutputPort())
contour3.SetValue(0, 0.5)
contour3.SetMergePoints(mergePoints)
contour3.SetSequentialProcessing(serialProcessing)
contour3.SetInterpolateAttributes(interpolateAttr);
contour3.SetComputeNormals(computeNormals);

cont3Mapper = vtk.vtkPolyDataMapper()
cont3Mapper.SetInputConnection(contour3.GetOutputPort())
cont3Mapper.ScalarVisibilityOff()

cont3Actor = vtk.vtkActor()
cont3Actor.SetMapper(cont3Mapper)
cont3Actor.GetProperty().SetColor(.8,.4,.4)

# Compare against standard contouring
contour4 = vtk.vtkContourFilter()
contour4.SetInputConnection(extract3.GetOutputPort())
contour4.SetValue(0,0.5)

cont4Mapper = vtk.vtkPolyDataMapper()
cont4Mapper.SetInputConnection(contour4.GetOutputPort())
cont4Mapper.ScalarVisibilityOff()

cont4Actor = vtk.vtkActor()
cont4Actor.SetMapper(cont4Mapper)
cont4Actor.GetProperty().SetColor(.8,.4,.4)

# Timing comparisons
timer = vtk.vtkTimerLog()
timer.StartTimer()
contour.Update()
timer.StopTimer()
time = timer.GetElapsedTime()

timer.StartTimer()
contour2.Update()
timer.StopTimer()
time2 = timer.GetElapsedTime()

timer.StartTimer()
contour3.Update()
timer.StopTimer()
time3 = timer.GetElapsedTime()

timer.StartTimer()
contour4.Update()
timer.StopTimer()
time4 = timer.GetElapsedTime()

print("Contouring comparison")
print("\tNumber of threads used: {0}".format(contour.GetNumberOfThreadsUsed()))
print("\tMerge points: {0}".format(mergePoints))
print("\tInterpolate attributes: {0}".format(interpolateAttr))
print("\tCompute Normals: {0}".format(computeNormals))

print("\tTetra contouring (fast path): {0}".format(time))
print("\t\tNumber of points: {0}".format(contour.GetOutput().GetNumberOfPoints()))
print("\t\tNumber of cells: {0}".format(contour.GetOutput().GetNumberOfCells()))

print("\tVoxel contouring: {0}".format(time2))
print("\t\tNumber of points: {0}".format(contour2.GetOutput().GetNumberOfPoints()))
print("\t\tNumber of cells: {0}".format(contour2.GetOutput().GetNumberOfCells()))

print("\tHex contouring: {0}".format(time3))
print("\t\tNumber of points: {0}".format(contour3.GetOutput().GetNumberOfPoints()))
print("\t\tNumber of cells: {0}".format(contour3.GetOutput().GetNumberOfCells()))

print("\tStandard contouring (of hexes): {0}".format(time4))
print("\t\tNumber of points: {0}".format(contour4.GetOutput().GetNumberOfPoints()))
print("\t\tNumber of cells: {0}".format(contour4.GetOutput().GetNumberOfCells()))

# Define graphics objects
renWin = vtk.vtkRenderWindow()
renWin.SetMultiSamples(0)

ren1 = vtk.vtkRenderer()
ren1.SetViewport(0,0,0.5,0.5)
ren1.SetBackground(1,1,1)
cam1 = ren1.GetActiveCamera()
ren2 = vtk.vtkRenderer()
ren2.SetViewport(0.5,0,1,0.5)
ren2.SetBackground(1,1,1)
ren2.SetActiveCamera(cam1)
ren3 = vtk.vtkRenderer()
ren3.SetViewport(0,0.5,0.5,1)
ren3.SetBackground(1,1,1)
ren3.SetActiveCamera(cam1)
ren4 = vtk.vtkRenderer()
ren4.SetViewport(0.5,0.5,1,1)
ren4.SetBackground(1,1,1)
ren4.SetActiveCamera(cam1)

renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)
renWin.AddRenderer(ren3)
renWin.AddRenderer(ren4)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(contActor)
ren2.AddActor(cont2Actor)
ren3.AddActor(cont3Actor)
ren4.AddActor(cont4Actor)

renWin.Render()
ren1.ResetCamera()
renWin.Render()

iren.Initialize()
iren.Start()
# --- end of script --
