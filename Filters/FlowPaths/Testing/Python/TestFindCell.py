#!/usr/bin/env python
import vtk

# Parameters for controlling the test size
dim = 40
numStreamlines = 50

# Create three volumes of different resolution and butt them together.  This
# tests vtkPointSet::FindCell() on incompatiable meshes (hanging and
# duplicate nodes).

spacing = 1.0 / (2.0*dim - 1.0)
v1 = vtk.vtkImageData()
v1.SetOrigin(0.0,0,0)
v1.SetDimensions(2*dim,2*dim,2*dim)
v1.SetSpacing(spacing,spacing,spacing)

spacing = 1.0 / (dim - 1.0)
v2 = vtk.vtkImageData()
v2.SetOrigin(1.0,0,0)
v2.SetDimensions(dim,dim,dim)
v2.SetSpacing(spacing,spacing,spacing)

spacing = 1.0 / (2.0*dim - 1.0)
v3 = vtk.vtkImageData()
v3.SetOrigin(2.0,0,0)
v3.SetDimensions(2*dim,2*dim,2*dim)
v3.SetSpacing(spacing,spacing,spacing)

# Append the volumes together to create an unstructured grid with duplicate
# and hanging points.
append = vtk.vtkAppendFilter()
append.AddInputData(v1)
append.AddInputData(v2)
append.AddInputData(v3)
append.MergePointsOff()
append.Update()

# Create a uniform vector field in the x-direction
numPts = append.GetOutput().GetNumberOfPoints()
vectors = vtk.vtkFloatArray()
vectors.SetNumberOfComponents(3)
vectors.SetNumberOfTuples(numPts);
for i in range(0,numPts):
    vectors.SetTuple3(i, 1,0,0)

# A hack but it works
append.GetOutput().GetPointData().SetVectors(vectors)

# Outline around appended unstructured grid
outline = vtk.vtkOutlineFilter()
outline.SetInputConnection(append.GetOutputPort())

outlineMapper = vtk.vtkPolyDataMapper()
outlineMapper.SetInputConnection(outline.GetOutputPort())

outlineActor = vtk.vtkActor()
outlineActor.SetMapper(outlineMapper)

# Now create streamlines from a rake of seed points
pt1 = [0.001,0.1,0.5]
pt2 = [0.001,0.9,0.5]
line = vtk.vtkLineSource()
line.SetResolution(numStreamlines-1)
line.SetPoint1(pt1)
line.SetPoint2(pt2)
line.Update()

rk4 = vtk.vtkRungeKutta4()
strategy = vtk.vtkClosestNPointsStrategy()
ivp = vtk.vtkInterpolatedVelocityField()
ivp.SetFindCellStrategy(strategy)

streamer = vtk.vtkStreamTracer()
streamer.SetInputConnection(append.GetOutputPort())
streamer.SetSourceConnection(line.GetOutputPort())
streamer.SetMaximumPropagation(10)
streamer.SetInitialIntegrationStep(.2)
streamer.SetIntegrationDirectionToForward()
streamer.SetMinimumIntegrationStep(0.01)
streamer.SetMaximumIntegrationStep(0.5)
streamer.SetTerminalSpeed(1.0e-12)
streamer.SetMaximumError(1.0e-06)
streamer.SetComputeVorticity(0)
streamer.SetIntegrator(rk4)
streamer.SetInterpolatorPrototype(ivp)
streamer.Update()
reasons = streamer.GetOutput().GetCellData().GetArray("ReasonForTermination")
print(reasons.GetValue(0))

strMapper = vtk.vtkPolyDataMapper()
strMapper.SetInputConnection(streamer.GetOutputPort())

strActor = vtk.vtkActor()
strActor.SetMapper(strMapper)

# rendering
ren = vtk.vtkRenderer()
renWin = vtk.vtkRenderWindow()
iren = vtk.vtkRenderWindowInteractor()
renWin.AddRenderer(ren)
iren.SetRenderWindow(renWin)

ren.AddActor(outlineActor)
ren.AddActor(strActor)
ren.ResetCamera()
ren.GetActiveCamera().Zoom(2)
renWin.SetSize(600, 300)
renWin.Render()

iren.Start()
