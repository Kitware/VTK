#!/usr/bin/env python
import vtk

# Test edge smoothing of meshes with specified smoothing stencils, and
# user-defined smoothing stencils.

# Control the resolution of the tests.
#res = 200
res = 4

# Create a x-y plane
plane = vtk.vtkPlaneSource()
plane.SetResolution(res,res)
plane.SetOrigin(0,0,0)
plane.SetPoint1(1,0,0)
plane.SetPoint2(0,1,0)
plane.Update()

# Create a constraint array
constraints = vtk.vtkDoubleArray()
constraints.SetName("SmoothingConstraints")
numPts = plane.GetOutput().GetNumberOfPoints()
constraints.SetNumberOfTuples(numPts)
for i in range(0,numPts):
    constraints.SetTuple1(i,1000)
constraints.SetTuple1(0,0.0)
constraints.SetTuple1(res,0.0)
constraints.SetTuple1(numPts-res-1,0.0)
constraints.SetTuple1(numPts-1,0.0)

output = plane.GetOutput()
output.GetPointData().AddArray(constraints)

# Manually create smoothing stencils
stencils = vtk.vtkCellArray()
pts = [0,0,0,0]
pts = [1,5]
stencils.InsertNextCell(2,pts) #stencil for point 0
pts = [0,2,6]
stencils.InsertNextCell(3,pts) #stencil for point 1
pts = [1,3,7]
stencils.InsertNextCell(3,pts) #stencil for point 2
pts = [2,4,8]
stencils.InsertNextCell(3,pts) #stencil for point 3
pts = [3,9]
stencils.InsertNextCell(2,pts) #stencil for point 4
pts = [0,6,10]
stencils.InsertNextCell(3,pts) #stencil for point 5
pts = [1,7,11,5]
stencils.InsertNextCell(4,pts) #stencil for point 6
pts = [2,8,12,6]
stencils.InsertNextCell(4,pts) #stencil for point 7
pts = [3,9,13,7]
stencils.InsertNextCell(4,pts) #stencil for point 8
pts = [4,8,14]
stencils.InsertNextCell(3,pts) #stencil for point 9
pts = [5,11,15]
stencils.InsertNextCell(3,pts) #stencil for point 10
pts = [6,12,16,10]
stencils.InsertNextCell(4,pts) #stencil for point 11
pts = [7,13,17,11]
stencils.InsertNextCell(4,pts) #stencil for point 12
pts = [8,14,18,12]
stencils.InsertNextCell(4,pts) #stencil for point 13
pts = [9,13,19]
stencils.InsertNextCell(3,pts) #stencil for point 14
pts = [10,16,20]
stencils.InsertNextCell(3,pts) #stencil for point 15
pts = [11,17,21,15]
stencils.InsertNextCell(4,pts) #stencil for point 16
pts = [12,18,22,16]
stencils.InsertNextCell(4,pts) #stencil for point 17
pts = [13,19,23,17]
stencils.InsertNextCell(4,pts) #stencil for point 18
pts = [14,18,24]
stencils.InsertNextCell(3,pts) #stencil for point 19
pts = [15,21]
stencils.InsertNextCell(2,pts) #stencil for point 20
pts = [20,16,22]
stencils.InsertNextCell(3,pts) #stencil for point 21
pts = [21,17,23]
stencils.InsertNextCell(3,pts) #stencil for point 22
pts = [22,18,24]
stencils.InsertNextCell(3,pts) #stencil for point 23
pts = [19,23]
stencils.InsertNextCell(2,pts) #stencil for point 24

# Now smooth the mesh with a filter constraint
smooth = vtk.vtkConstrainedSmoothingFilter()
smooth.SetInputConnection(plane.GetOutputPort())
smooth.SetSmoothingStencils(stencils)
smooth.SetConstraintStrategyToConstraintDistance()
smooth.SetConstraintDistance(0.1)
smooth.SetNumberOfIterations(100)
smooth.SetRelaxationFactor(.2)
smooth.GenerateErrorScalarsOn()
smooth.GenerateErrorVectorsOff()

timer = vtk.vtkExecutionTimer()
timer.SetFilter(smooth)
smooth.Update()
ST = timer.GetElapsedWallClockTime()
print ("Smooth Edges: ", ST)

smoothMapper = vtk.vtkPolyDataMapper()
smoothMapper.SetInputConnection(smooth.GetOutputPort())
smoothMapper.ScalarVisibilityOn()

smoothActor = vtk.vtkActor()
smoothActor.SetMapper(smoothMapper)
smoothActor.GetProperty().SetInterpolationToFlat()

# Use a constraint array
smooth2 = vtk.vtkConstrainedSmoothingFilter()
smooth2.SetInputConnection(plane.GetOutputPort())
smooth2.SetSmoothingStencils(stencils)
smooth2.SetConstraintStrategyToConstraintArray()
smooth2.SetNumberOfIterations(100)
smooth2.SetRelaxationFactor(.2)
smooth2.GenerateErrorScalarsOn()
smooth2.GenerateErrorVectorsOn()

timer.SetFilter(smooth2)
smooth2.Update()
ST = timer.GetElapsedWallClockTime()
print ("Smooth Edges (constraint array): ", ST)

# Color by scalar errors if scalar visibility is enabled.
# We don't enable it here because different rendering libraries
# will shade quads differently.
smooth2Mapper = vtk.vtkPolyDataMapper()
smooth2Mapper.SetInputConnection(smooth2.GetOutputPort())
smooth2Mapper.ScalarVisibilityOff()
smooth2Mapper.SetScalarModeToUsePointFieldData()
smooth2Mapper.SelectColorArray("SmoothingErrorScalars")
smooth2Mapper.SetScalarRange(smooth2.GetOutput().GetPointData().GetArray("SmoothingErrorScalars").GetRange())

smooth2Actor = vtk.vtkActor()
smooth2Actor.SetMapper(smooth2Mapper)
smooth2Actor.GetProperty().SetInterpolationToFlat()

# Define graphics objects
renWin = vtk.vtkRenderWindow()
renWin.SetSize(600,300)

ren1 = vtk.vtkRenderer()
ren1.SetViewport(0,0,0.5,1)
ren1.SetBackground(0,0,0)

ren2 = vtk.vtkRenderer()
ren2.SetViewport(0.5,0,1,1)
ren2.SetBackground(0,0,0)

renWin.AddRenderer(ren1)
renWin.AddRenderer(ren2)

iren = vtk.vtkRenderWindowInteractor()
iren.SetRenderWindow(renWin)

ren1.AddActor(smoothActor)
ren2.AddActor(smooth2Actor)
ren1.ResetCamera()
ren2.SetActiveCamera(ren1.GetActiveCamera())
renWin.Render()

iren.Initialize()
iren.Start()
# --- end of script --
