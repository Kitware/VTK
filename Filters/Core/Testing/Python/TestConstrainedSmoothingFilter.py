#!/usr/bin/env python
import vtk

# Test edge smoothing of meshes with generated smoothing stencils, and
# different constraint types.

# Control the resolution of the tests.
#res = 200
res = 40

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

# Now smooth the mesh with a filter constraint
smooth = vtk.vtkConstrainedSmoothingFilter()
smooth.SetInputConnection(plane.GetOutputPort())
smooth.SetConstraintStrategyToConstraintDistance()
smooth.SetConstraintDistance(0.1)
smooth.SetNumberOfIterations(100)
smooth.SetRelaxationFactor(.2)

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
smooth2.SetConstraintStrategyToConstraintArray()
smooth2.SetNumberOfIterations(100)
smooth2.SetRelaxationFactor(.2)

timer.SetFilter(smooth2)
smooth.Update()
ST = timer.GetElapsedWallClockTime()
print ("Smooth Edges (constraint array): ", ST)

smooth2Mapper = vtk.vtkPolyDataMapper()
smooth2Mapper.SetInputConnection(smooth2.GetOutputPort())
smooth2Mapper.ScalarVisibilityOn()

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
